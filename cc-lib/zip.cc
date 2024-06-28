
#include "zip.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include <cstdint>
#include <deque>
#include <string_view>
#include <memory>

#include "miniz.h"

#include "base/logging.h"
#include "base/stringprintf.h"

static constexpr size_t BUFFER_SIZE = 32768;
static constexpr size_t CHUNK_SIZE = 32768;
static_assert(BUFFER_SIZE >= TINFL_LZ_DICT_SIZE,
              "buffer size must be at least the size of the "
              "dictionary.");

static_assert((BUFFER_SIZE & (BUFFER_SIZE - 1)) == 0,
              "buffer size must be a power of 2");

#define DEBUG_ZIP 0

namespace {

#if DEBUG_ZIP
[[maybe_unused]]
static std::string DumpString(std::string_view s) {
  std::string out = StringPrintf("%d bytes:\n", (int)s.size());

  for (int p = 0; p < (int)s.size(); p += 16) {
    // Print line, first hex, then ascii
    for (int i = 0; i < 16; i++) {
      int idx = p + i;
      if (idx < (int)s.size()) {
        uint8_t c = s[idx];
        StringAppendF(&out, "%02x ", c);
      } else {
        StringAppendF(&out, " . ");
      }
    }

    StringAppendF(&out, "| ");

    for (int i = 0; i < 16; i++) {
      int idx = p + i;
      if (idx < (int)s.size()) {
        uint8_t c = s[idx];
        if (c >= 32 && c < 127) {
          StringAppendF(&out, "%c", c);
        } else {
          StringAppendF(&out, ".");
        }
      } else {
        StringAppendF(&out, " ");
      }
    }

    StringAppendF(&out, "\n");
  }

  return out;
}

static std::string DumpVector(const std::vector<uint8_t> &v) {
  return DumpString(std::string_view{(const char *)v.data(), v.size()});
}
#endif

// TODO PERF: I initially thought I could write decompressed data
// directly into this deque of vectors. But miniz seems to want to
// reuse the same circular buffer between calls. That's fine, but
// it's probably possible to simplify this stuff now. At least we
// should tune BUFFER/CHUNK/CIRC size.
template<size_t N_>
struct Vec {
  static constexpr size_t N = N_;
  std::array<uint8_t, N> arr;
  // Where the first byte would be. Never more than N.
  size_t start = 0;
  // one past last byte. Never more than N.
  size_t end = 0;

  const uint8_t *data() const {
    return arr.data() + start;
  }
  uint8_t *data() {
    return arr.data() + start;
  }

  // Amount of data available to read, starting at start.
  size_t size() const {
    DCHECK(start <= end);
    DCHECK(start <= N);
    DCHECK(end <= N);
    return end - start;
  }

  // Area in which to write new data.
  const uint8_t *space() const {
    return arr.data() + end;
  }
  uint8_t *space() {
    return arr.data() + end;
  }

  // Amount of space left to write at the end.
  size_t space_left() const {
    DCHECK(end <= N);
    return N - end;
  }
};

template<size_t N_>
struct Buf {
  static constexpr size_t N = N_;
  // A double-ended queue. The writer writes to the back, and the
  // reader reads from the front. Each vec has capacity N.

  // Get the next write destination, perhaps by allocating a new
  // block. It will have a minimum of at_least space in it.
  Vec<N> *GetDest(size_t at_least) {
    if (q.empty()) {
      q.emplace_back();
      return &q.back();
    }
    Vec<N> *v = &q.back();
    if (v->space_left() < at_least) {
      DCHECK(at_least <= N) << "There will never be enough space "
        "in a chunk if you ask for " << at_least << " but chunk "
        "size is " << N;
      q.emplace_back();
      return &q.back();
    } else {
      return v;
    }
  }

  // Get the next read source, possibly destroying empty blocks,
  // and possibly returning nullptr if there is no data. If non-null,
  // there will always be at least one byte to read.
  Vec<N> *GetSrc() {
    while (!q.empty()) {
      Vec<N> *v = &q.front();
      if (v->start == N) {
        DCHECK(v->end == N);
        q.pop_front();
      } else if (v->start == v->end) {
        // An empty block is possible if it was consumed by reading,
        // or because we reached the final block but nothing is
        // written there yet.
        if (DEBUG_ZIP) {
          printf("(%d == %d)\n", (int)v->start, (int)v->end);
        }
        q.pop_front();
      } else {
        return v;
      }
    }
    if (DEBUG_ZIP) {
      printf("(q empty)\n");
    }
    return nullptr;
  }

  // Callers need to manually adjust this.
  void AdjustSize(int64_t delta) {
    size += delta;
    CHECK(size >= 0) << size << " " << delta;
  }

  size_t WriteOutput(uint8_t *out_data, size_t out_size) {
    if (DEBUG_ZIP) {
      printf("== write output ==\n");
    }
    CheckInvariants();
    size_t done = 0;
    while (this->size > 0) {
      Vec<N> *v = GetSrc();
      if (v == nullptr) {
        if (DEBUG_ZIP) {
          printf("(null)\n");
        }
        break;
      }
      if (DEBUG_ZIP) {
        printf("Block at %p:\n", v);
      }

      DCHECK(v->start <= v->end);
      size_t chunk_size = v->end - v->start;
      // We might not have enough room for the whole chunk
      // in the output.
      size_t copy_size = std::min(chunk_size, out_size);
      if (DEBUG_ZIP) {
        printf("  copy size %d (min(%d, %d))\n",
               (int)copy_size, (int)chunk_size, (int)out_size);
      }
      memcpy(out_data, v->data(), copy_size);
      done += copy_size;
      out_data += copy_size;
      v->start += copy_size;
      DCHECK(copy_size <= out_size);
      DCHECK(v->start <= v->end);
      out_size -= copy_size;
      this->size -= copy_size;
    }
    if (DEBUG_ZIP) {
      printf("Done. Read %d\n", (int)done);
    }
    return done;
  }

  void CheckInvariants() {
#   ifndef NDEBUG
    int64_t actual_size = 0;
    for (const Vec<N> &v : q) {
      if (DEBUG_ZIP) {
        printf("(Block at %p, size %d. [%d,%d))\n",
               &v, (int)v.size(),
               (int)v.start, (int)v.end);
      }
      actual_size += v.size();
      DCHECK(v.start <= v.end);
      DCHECK(v.start <= N);
      DCHECK(v.end <= N);
    }

    if (DEBUG_ZIP) {
      printf("Actual %d vs %d\n", (int)actual_size, (int)size);
    }
    DCHECK(actual_size == size) << actual_size << " " << size;
#   endif
  }

  std::deque<Vec<N>> q;
  int64_t size = 0;
};

struct EBImpl : public ZIP::EncodeBuffer {

  // There don't seem to be any restrictions on this, so we
  // just use the same size as the decompressor.
  static constexpr size_t ENCODE_BUFFER_SIZE = BUFFER_SIZE;

  // For levels 0..10, the number of hash probes to use.
  static constexpr const int probes_for_level[11] = {
    0, 1, 6, 32, 16, 32, 128, 256, 512, 768, 1500 };

  EBImpl(int level) {
    level = std::clamp(level, 0, 10);
    // Don't want any of the other flags. Just set the number of
    // probes by the compression level.
    const int FLAGS = probes_for_level[level] & TDEFL_MAX_PROBES_MASK;

    CHECK(TDEFL_STATUS_OKAY ==
          tdefl_init(&enc,
                     // no callback. using tdefl_compress interface.
                     nullptr, nullptr,
                     FLAGS));
  }

  ~EBImpl() override { }

  void InsertVector(const std::vector<uint8_t> &v) override {
    InsertPtr(v.data(), v.size());
  }

  void InsertString(const std::string &s) override {
    InsertPtr((const uint8_t *)s.data(), s.size());
  }

  void InsertPtr(const uint8_t *data_in, size_t size_in) override {
    const uint8_t *data = data_in;
    size_t remaining = size_in;
    while (remaining > 0) {
      V *out = buf.GetDest(ENCODE_BUFFER_SIZE);
      DCHECK(V::N > out->end);
      const size_t space_left = out->space_left();
      DCHECK(space_left != 0);
      // in/out parameter
      size_t out_bytes = space_left;
      size_t in_bytes = remaining;

      uint8_t *out_space = out->space();

      if (DEBUG_ZIP) {
        printf("tdefl_compress %p for %d -> %p for %d.\n",
               data, (int)in_bytes, out_space, (int)out_bytes);
      }
      tdefl_status status =
        tdefl_compress(&enc,
                       // input buffer
                       data, &in_bytes,
                       // output buffer.
                       out_space, &out_bytes,
                       TDEFL_NO_FLUSH);

      if (DEBUG_ZIP) {
        CHECK(status == TDEFL_STATUS_OKAY) << "Bug! Only OKAY status makes "
          "sense since we don't FINISH here.";
      }

      if (DEBUG_ZIP) {
        printf("Compress %d/%d in; got %d/%d out\n",
               (int)in_bytes, (int)remaining,
               (int)out_bytes, (int)space_left);
      }

      DCHECK(in_bytes <= remaining);
      remaining -= in_bytes;
      data += in_bytes;

      DCHECK(out_bytes <= space_left) << out_bytes << " " << space_left;
      out->end += out_bytes;
      buf.AdjustSize(+out_bytes);

      buf.CheckInvariants();

    } while (remaining > 0);
  }

  void Finalize() override {

    for (;;) {
      V *out = buf.GetDest(ENCODE_BUFFER_SIZE);
      if (DEBUG_ZIP) {
        printf("For finalize, space at %p (start=%d, end=%d)\n",
               &out->arr, (int)out->start, (int)out->end);
      }
      DCHECK(V::N > out->end);
      const size_t space_left = out->space_left();
      DCHECK(space_left != 0);
      // In/out parameter.
      size_t out_bytes = space_left;
      uint8_t *out_space = out->space();

      if (DEBUG_ZIP) {
        printf("Finalize with %d space\n", (int)out_bytes);
      }
      size_t in_bytes = 0;
      tdefl_status status =
        tdefl_compress(&enc,
                       // (empty) input buffer
                       nullptr, &in_bytes,
                       // output buffer.
                       out_space, &out_bytes,
                       TDEFL_FINISH);

      if constexpr (DEBUG_ZIP) {
        printf("Finalize %d/%d in, get %d/%d out to %p\n",
               (int)0, (int)0,
               (int)out_bytes, (int)space_left, out);
        #if DEBUG_ZIP
        printf(
            "%s\n",
            DumpString(std::string_view{
                (const char *)out_space,
                out_bytes
              }).c_str());
        #endif
      }

      CHECK(in_bytes == 0);

      CHECK(status == TDEFL_STATUS_OKAY ||
            status == TDEFL_STATUS_DONE) << "Must be one of these, "
        "or something is wrong.";

      DCHECK(out_bytes <= space_left) << out_bytes << " " << space_left;
      out->end += out_bytes;
      buf.AdjustSize(+out_bytes);

      buf.CheckInvariants();

      if (status == TDEFL_STATUS_DONE) return;
    }
  }

  size_t OutputSize() const override { return buf.size; }

  std::vector<uint8_t> GetOutputVector() override {
    size_t sz = buf.size;
    std::vector<uint8_t> ret(sz);
    size_t wrote = WriteOutput(ret.data(), ret.size());
    CHECK(wrote == sz) << wrote << " " << sz;
    return ret;
  }
  std::string GetOutputString() override {
    size_t sz = buf.size;
    std::string ret;
    ret.resize(sz);

    size_t wrote = WriteOutput((uint8_t *)ret.data(), ret.size());

    CHECK(wrote == sz) << wrote << " " << sz;
    return ret;
  }

  // Write up to size bytes; return the number written.
  size_t WriteOutput(uint8_t *data, size_t size) override {
    return buf.WriteOutput(data, size);
  }


  tdefl_compressor enc;
  using V = Vec<CHUNK_SIZE>;
  Buf<CHUNK_SIZE> buf;
};

struct DBImpl : public ZIP::DecodeBuffer {
  static constexpr size_t CIRC_SIZE = BUFFER_SIZE;
  static_assert(CIRC_SIZE >= 32768);
  static_assert((CIRC_SIZE & (CIRC_SIZE - 1)) == 0,
                "must be a power of 2");
  static constexpr size_t CIRC_MASK = CIRC_SIZE - 1;

  ~DBImpl() override {
    tinfl_decompressor_free(dec);
    dec = nullptr;
  }

  DBImpl() : dec(tinfl_decompressor_alloc()) {
    CHECK(dec != nullptr);
    tinfl_init(dec);
    circ.reset(new uint8_t[CIRC_SIZE]);
    circ_pos = 0;
  }

  // Insert compressed data. New decompressed bytes may become
  // available in the output.
  void InsertVector(const std::vector<uint8_t> &v) override {
    InsertPtr(v.data(), v.size());
  }
  void InsertString(const std::string &s) override {
    InsertPtr((const uint8_t *)s.data(), s.size());
  }
  void InsertPtr(const uint8_t *data_in, size_t size_in) override {
    if (DEBUG_ZIP) {
      printf("InsertPtr on %p for %d\n", data_in, (int)size_in);
    }
    if (size_in == 0) return;

    // TODO: Figure out if I want headers etc.
    // Probably not by default?

    const uint8_t *data = data_in;
    size_t remaining = size_in;
    bool has_more_output = false;
    do {
      // Note this may be zero if we're making more calls just to
      // read output.
      size_t in_bytes = remaining;
      // Somewhat surprisingly, the "buf_size" parameter should be
      // the number of trailing bytes in the circular buffer, not
      // its full size.
      size_t out_bytes = CIRC_SIZE - circ_pos;

      if (DEBUG_ZIP) {
        printf("Decompress buffer at %p for %d -> %p (%p) size %zu\n",
               data, (int)in_bytes,
               circ.get(), circ.get() + circ_pos, (size_t)out_bytes);
      }
      tinfl_status status =
        tinfl_decompress(dec,
                         // input buffer
                         data, &in_bytes,
                         // circular buffer start and current pos.
                         circ.get(), circ.get() + circ_pos, &out_bytes,
                         TINFL_FLAG_HAS_MORE_INPUT);
      if (DEBUG_ZIP) {
        printf("Decompress %d/%d; got out %d\n",
               (int)in_bytes, (int)remaining,
               (int)out_bytes);

        #if DEBUG_ZIP
        if (false) {
        printf("Full circ buffer is:\n%s\n",
               DumpString(std::string_view{
                   (const char *)circ.get(),
                   CIRC_SIZE
                 }).c_str());
        }
        #endif
      }

      CHECK(status != TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS)
        << "Should be impossible since we're passing data with each "
        "call.";

      CHECK(status != TINFL_STATUS_BAD_PARAM) << "Bug? " <<
        StringPrintf("%p %lzu %p %p %lld\n",
                     data, (size_t)in_bytes, circ.get(), circ.get() + circ_pos,
                     out_bytes);

      CHECK(status != TINFL_STATUS_FAILED) <<
        "Flate stream is corrupt (miscellaneous).";

      CHECK(status != TINFL_STATUS_ADLER32_MISMATCH) <<
        "Flate stream is corrupt (bad checksum).";

      /*
      CHECK(status != TINFL_STATUS_DONE) <<
        "Bug: This is unexpected because we've said there's more input, "
        "but possibly I do not understand the miniz API.";
      */
      CHECK(status == TINFL_STATUS_DONE ||
            status == TINFL_STATUS_NEEDS_MORE_INPUT ||
            status == TINFL_STATUS_HAS_MORE_OUTPUT);

      DCHECK(in_bytes <= remaining);
      data += in_bytes;
      remaining -= in_bytes;

      // We have out_bytes of data, which start at
      // circ_pos, but wrap around.

      // PERF: Could do this with two memcpy calls.
      V *out = buf.GetDest(out_bytes);
      DCHECK(out_bytes <= out->space_left());
      for (size_t i = 0; i < out_bytes; i++) {
        out->arr[out->end + i] =
          circ[(circ_pos + i) & CIRC_MASK];
      }

      out->end += out_bytes;
      buf.AdjustSize(+out_bytes);
      circ_pos = (circ_pos + out_bytes) & CIRC_MASK;

      if (status == TINFL_STATUS_NEEDS_MORE_INPUT) {
        // We've read all the output. We're done if we've inserted all
        // our input.
        has_more_output = false;
      } else if (status == TINFL_STATUS_HAS_MORE_OUTPUT) {
        // Need to loop again, at least to read the pending output.
        has_more_output = true;
      } else if (status == TINFL_STATUS_DONE) {
        // The situations where this is returned are not clearly
        // documented (since I think the flate stream is not
        // delimited), but it's pretty clear that we don't have more
        // output to read.
        has_more_output = false;
      } else {
        LOG(FATAL) << "Bug: Unknown status " << status;
      }

    } while (remaining > 0 || has_more_output);
  }


  // Number of bytes that are ready.
  size_t OutputSize() const override {
    return buf.size;
  }

  // Note: These are correct, but we could be a little faster by
  // skipping the logic below that makes sure we don't exceed
  // the buffer size. We have exactly enough space by construction.
  std::vector<uint8_t> GetOutputVector() override {
    size_t sz = buf.size;
    std::vector<uint8_t> ret(sz);
    CHECK(WriteOutput(ret.data(), ret.size()) == sz);
    return ret;
  }
  std::string GetOutputString() override {
    size_t sz = buf.size;
    std::string ret;
    ret.resize(sz);
    CHECK(WriteOutput((uint8_t *)ret.data(), ret.size()) == sz);
    return ret;
  }

  // Write up to size bytes; return the number written.
  size_t WriteOutput(uint8_t *data, size_t size) override {
    return buf.WriteOutput(data, size);
  }

  // PERF: We should be able to avoid an indirection here.
  // Just nest the struct.
  tinfl_decompressor *dec = nullptr;
  using V = Vec<CHUNK_SIZE>;
  // If we don't know the output size ahead of time (we don't), then
  // miniz wants a fixed circular buffer and seemingly wants us to keep
  // reusing the same one (it keeps a member m_dist_from_out_buf_start).
  // Here is that buffer, which has size BUFFER_SIZE.
  std::unique_ptr<uint8_t[]> circ;
  // The position of the "start" of this circular buffer.
  int circ_pos = 0;
  Buf<CHUNK_SIZE> buf;
};
}

ZIP::DecodeBuffer *ZIP::DecodeBuffer::Create() {
  return new DBImpl;
}

ZIP::EncodeBuffer *ZIP::EncodeBuffer::Create(int level) {
  return new EBImpl(level);
}

ZIP::EncodeBuffer::EncodeBuffer() { }
ZIP::EncodeBuffer::~EncodeBuffer() { }

ZIP::DecodeBuffer::DecodeBuffer() { }
ZIP::DecodeBuffer::~DecodeBuffer() { }

// Intended for T = std::vector<uint8_t> or std::string
template<class T>
static T CompressPtr(const uint8_t *data, size_t size,
                     int level, int other_flags) {
  size_t out_size = 0;
  level = std::clamp(level, 0, 10);
  const int flags = EBImpl::probes_for_level[level] | other_flags;
  uint8_t *e =
    (uint8_t*)tdefl_compress_mem_to_heap(data, size, &out_size, flags);
  CHECK(e != nullptr);
  T ret;
  ret.resize(out_size);
  memcpy(ret.data(), e, out_size);
  free(e);
  return ret;
}

template<class T>
static T DecompressPtr(const uint8_t *data, size_t size,
                       int flags) {
  size_t out_size = 0;
  uint8_t *d =
    (uint8_t*)tinfl_decompress_mem_to_heap(data, size, &out_size, flags);
  if (d == nullptr) {
    CHECK(out_size == 0);
    return T();
  } else {
    CHECK(d != nullptr);
    T ret;
    ret.resize(out_size);
    memcpy(ret.data(), d, out_size);
    free(d);
    return ret;
  }
}

std::vector<uint8_t> ZIP::UnzipPtr(const uint8_t *data, size_t size) {
  return DecompressPtr<std::vector<uint8_t>>(data, size, 0);
}

std::vector<uint8_t> ZIP::UnzipVector(const std::vector<uint8_t> &v) {
  return DecompressPtr<std::vector<uint8_t>>(v.data(), v.size(), 0);
}

std::string ZIP::UnzipString(std::string_view s) {
  return DecompressPtr<std::string>(
      (const uint8_t *)s.data(), s.size(), 0);
}

std::vector<uint8_t> ZIP::ZipPtr(const uint8_t *data, size_t size,
                                 int level) {
  return CompressPtr<std::vector<uint8_t>>(data, size, level, 0);
}

std::string ZIP::ZipString(std::string_view s, int level) {
  return CompressPtr<std::string>((const uint8_t*)s.data(), s.size(), level, 0);
}

std::vector<uint8_t> ZIP::ZipVector(const std::vector<uint8_t> &v,
                                    int level) {
  return CompressPtr<std::vector<uint8_t>>(v.data(), v.size(), level, 0);
}


std::vector<uint8_t> ZIP::ZlibPtr(const uint8_t *data, size_t size,
                                  int level) {
  return CompressPtr<std::vector<uint8_t>>(data, size, level, TDEFL_WRITE_ZLIB_HEADER);
}

std::vector<uint8_t> ZIP::ZlibVector(const std::vector<uint8_t> &v,
                                     int level) {
  return CompressPtr<std::vector<uint8_t>>(v.data(), v.size(), level, TDEFL_WRITE_ZLIB_HEADER);
}

std::string ZIP::ZlibString(std::string_view s, int level) {
  return CompressPtr<std::string>(
      (const uint8_t*)s.data(), s.size(), level, TDEFL_WRITE_ZLIB_HEADER);
}


std::vector<uint8_t> ZIP::UnZlibPtr(const uint8_t *data, size_t size) {
  return DecompressPtr<std::vector<uint8_t>>(data, size, TINFL_FLAG_PARSE_ZLIB_HEADER);
}

std::vector<uint8_t> ZIP::UnZlibVector(const std::vector<uint8_t> &v) {
  return DecompressPtr<std::vector<uint8_t>>(v.data(), v.size(), TINFL_FLAG_PARSE_ZLIB_HEADER);
}

std::string ZIP::UnZlibString(std::string_view s) {
  return DecompressPtr<std::string>(
      (const uint8_t*)s.data(), s.size(), TINFL_FLAG_PARSE_ZLIB_HEADER);
}


void ZIP::CCLibHeader::SetFlags(uint32_t f) {
  flags_msb_first[3] = f & 0xFF; f >>= 8;
  flags_msb_first[2] = f & 0xFF; f >>= 8;
  flags_msb_first[1] = f & 0xFF; f >>= 8;
  flags_msb_first[0] = f & 0xFF; f >>= 8;
}
uint32 ZIP::CCLibHeader::GetFlags() const {
  uint32_t f = 0;
  f <<= 8; f |= flags_msb_first[0];
  f <<= 8; f |= flags_msb_first[1];
  f <<= 8; f |= flags_msb_first[2];
  f <<= 8; f |= flags_msb_first[3];
  return f;
}

void ZIP::CCLibHeader::SetSize(uint64_t s) {
  size_msb_first[7] = s & 0xFF; s >>= 8;
  size_msb_first[6] = s & 0xFF; s >>= 8;
  size_msb_first[5] = s & 0xFF; s >>= 8;
  size_msb_first[4] = s & 0xFF; s >>= 8;
  size_msb_first[3] = s & 0xFF; s >>= 8;
  size_msb_first[2] = s & 0xFF; s >>= 8;
  size_msb_first[1] = s & 0xFF; s >>= 8;
  size_msb_first[0] = s & 0xFF; s >>= 8;
}

uint64_t ZIP::CCLibHeader::GetSize() const {
  uint64_t s = 0;
  s <<= 8; s |= size_msb_first[0];
  s <<= 8; s |= size_msb_first[1];
  s <<= 8; s |= size_msb_first[2];
  s <<= 8; s |= size_msb_first[3];
  s <<= 8; s |= size_msb_first[4];
  s <<= 8; s |= size_msb_first[5];
  s <<= 8; s |= size_msb_first[6];
  s <<= 8; s |= size_msb_first[7];
  return s;
}

bool ZIP::CCLibHeader::HasCorrectMagic() const {
  CCLibHeader r;
  return r.magic[0] == magic[0] &&
    r.magic[1] == magic[1] &&
    r.magic[2] == magic[2] &&
    r.magic[3] == magic[3];
}

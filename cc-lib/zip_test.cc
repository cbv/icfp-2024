
#include "zip.h"

#include <cstdio>
#include <vector>
#include <string_view>
#include <cstdint>
#include <memory>
#include <string>

#include "ansi.h"
#include "arcfour.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "randutil.h"
#include "util.h"
#include "periodically.h"
#include "timer.h"

// These do whole-buffer decompression, but using the stream interface,
// to exercise that.

static std::vector<uint8_t> StreamUnzipPtr(const uint8_t *data, size_t size) {
  std::unique_ptr<ZIP::DecodeBuffer> dec(ZIP::DecodeBuffer::Create());
  CHECK(dec.get() != nullptr);

  dec->InsertPtr(data, size);

  return dec->GetOutputVector();
}

static std::vector<uint8_t> StreamUnzipVector(const std::vector<uint8_t> &v) {
  return StreamUnzipPtr(v.data(), v.size());
}

static std::string StreamUnzipString(const std::string &s) {
  std::unique_ptr<ZIP::DecodeBuffer> dec(ZIP::DecodeBuffer::Create());
  CHECK(dec.get() != nullptr);

  dec->InsertString(s);

  return dec->GetOutputString();
}


static std::vector<uint8_t> StreamZipPtr(const uint8_t *data, size_t size,
                                         int level = 7) {
  std::unique_ptr<ZIP::EncodeBuffer> enc(ZIP::EncodeBuffer::Create(level));
  CHECK(enc.get() != nullptr);

  enc->InsertPtr(data, size);
  enc->Finalize();
  return enc->GetOutputVector();
}

static std::vector<uint8_t> StreamZipVector(const std::vector<uint8_t> &v,
                                            int level = 7) {
  return StreamZipPtr(v.data(), v.size(), level);
}

static std::string StreamZipString(const std::string &s, int level) {
  std::unique_ptr<ZIP::EncodeBuffer> enc(ZIP::EncodeBuffer::Create(level));
  CHECK(enc.get() != nullptr);

  enc->InsertString(s);
  enc->Finalize();
  return enc->GetOutputString();
}


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

static std::string DumpStringDiff(std::string_view s1,
                                  std::string_view s2) {
  std::string out = StringPrintf("%d bytes vs %d bytes\n",
                                 (int)s1.size(), (int)s2.size());
# define ANSI_LEFT  ANSI_FG(210, 128, 128)
# define ANSI_RIGHT ANSI_FG(210, 160, 112)

  bool equal_prefix = true;

  auto Equal16 = [s1, s2](int p) {
      for (int i = 0; i < 16; i++) {
        const size_t idx = p + i;
        if (idx >= s1.size() || idx >= s2.size()) return false;
        if (s1[idx] != s2[idx]) return false;
      }
      return true;
    };

  for (int p = 0; p < (int)std::max(s1.size(), s2.size()); p += 16) {
    if (equal_prefix && Equal16(p)) {
      continue;
    } else if (equal_prefix) {
      if (p > 0) {
        StringAppendF(&out,
            AGREY("...") " " ABLUE("%d equal bytes") " " AGREY("...") "\n",
            p);
      }
      equal_prefix = false;
    }

    // Print line, first hex, then ascii
    for (int i = 0; i < 16; i++) {
      const size_t idx = p + i;
      const int c1 = idx < s1.size() ? (uint8_t)s1[idx] : -1;
      const int c2 = idx < s2.size() ? (uint8_t)s2[idx] : -1;
      if (c1 == c2) {
        if (c1 >= 0) {
          StringAppendF(&out, AGREY("%02x=="), (uint8_t)c1);
        } else {
          StringAppendF(&out, AGREY("::::"));
        }
      } else {
        #define L
        StringAppendF(&out, ANSI_LEFT);
        if (c1 >= 0) {
          StringAppendF(&out, "%02x", (uint8_t)c1);
        } else {
          StringAppendF(&out, "__");
        }

        StringAppendF(&out, ANSI_RIGHT);
        if (c2 >= 0) {
          StringAppendF(&out, "%02x", (uint8_t)c2);
        } else {
          StringAppendF(&out, "__");
        }
      }
    }

    #if 0
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
    #endif

    StringAppendF(&out, ANSI_RESET "\n");
  }

  return out;
}


static std::string DumpVector(const std::vector<uint8_t> &v) {
  return DumpString(std::string_view{(const char *)v.data(), v.size()});
}

static std::string DumpVectorDiff(const std::vector<uint8_t> &v1,
                                  const std::vector<uint8_t> &v2) {
  return DumpStringDiff(std::string_view{(const char *)v1.data(), v1.size()},
                        std::string_view{(const char *)v2.data(), v2.size()});
}

static void TestOneRoundTrip(const std::string &s) {
  // printf("Input string:\n%s\n", DumpString(s).c_str());
  // printf(AWHITE("Encode %d bytes:") "\n", (int)s.size());
  std::string enc = StreamZipString(s, 7);
  // printf(AWHITE("Decode %d bytes:") "\n", (int)enc.size());
  // printf("Encoded:\n%s\n", DumpString(enc).c_str());
  std::string dec = StreamUnzipString(enc);
  // printf(AWHITE("Got %d bytes.") "\n", (int)dec.size());
  if (dec != s) {

    printf("%s\n", DumpStringDiff(s, dec).c_str());

    printf("Input " AWHITE("%d") " bytes, encoded " AYELLOW("%d")
           ", decoded " APURPLE("%d") "\n",
           (int)s.size(), (int)enc.size(), (int)dec.size());

    LOG(FATAL) << "Diffs";
    LOG(FATAL) << "Round trip failed for: " << s << "\n"
      "Which encoded to:\n" << DumpString(enc) <<
      "And then decoded to:\n" << DumpString(dec);
  }
}

static void TestOneRoundTrip(const std::vector<uint8_t> &v) {
  // printf("Test vec:\n%s\n", DumpVector(v).c_str());
  std::vector<uint8_t> enc = StreamZipVector(v, 7);
  std::vector<uint8_t> dec_raw = ZIP::UnzipPtr(enc.data(), enc.size());
  std::vector<uint8_t> dec = StreamUnzipVector(enc);
  if (dec_raw != v) {
    if (dec_raw == dec) {
      printf("Streamed and raw decompression match!\n");
    }
    LOG(FATAL) << "'Raw' decompression didn't work!\n";
  }

  if (dec != v) {

    printf("%s\n", DumpVectorDiff(v, dec).c_str());
    LOG(FATAL) << "Diffs.\n";

    LOG(FATAL) << "Round trip failed for: " << DumpVector(v) << "\n"
      "Which encoded to:\n" << DumpVector(enc) <<
      "And then decoded to:\n" << DumpVector(dec);
  }
}

static void TestRoundTripRandom() {
  Periodically status_per(0.25);
  Timer timer;
  ArcFour rc("zip_test");

  static constexpr int NUM_ROUNDS = 8192;
  for (int i = 0; i < NUM_ROUNDS; i++) {
    int size = 1 + RandTo(&rc, 131072 * 4);
    // int size = 128;
    std::vector<uint8_t> v;
    v.reserve(size);

    switch (rc.Byte() & 3) {
    default:
    case 0:
      // Uniformly random.
      for (int x = 0; x < size; x++) {
        v.push_back(rc.Byte());
      }
      break;
    case 1: {
      // Runs of random length.
      uint8_t b = rc.Byte();
      while ((int)v.size() < size) {
        uint8_t x = rc.Byte();
        if ((x & 0xf) == 0) {
          b = rc.Byte();
        } else {
          v.push_back(b);
        }
      }
      break;
    }
    case 2: {
      v.push_back(rc.Byte());

      // Alternating a substring of the existing buffer,
      // then a new random byte.
      while ((int)v.size() < size) {
        int pos = RandTo(&rc, v.size());
        int len = std::min(size - v.size(),
                           RandTo(&rc, v.size() - pos));
        for (int i = 0; i < len; i++) {
          v.push_back(v[pos + i]);
        }
        v.push_back(rc.Byte());
      }

      break;
    }
    }

    TestOneRoundTrip(v);
    if (status_per.ShouldRun()) {
      printf(ANSI_UP "%s\n",
             ANSI::ProgressBar(i, NUM_ROUNDS,
                               "Random vector round-trips",
                               timer.Seconds()).c_str());
    }
  }
}

static bool RoundTripOK(const std::vector<uint8_t> &v) {
  std::vector<uint8_t> enc = StreamZipVector(v, 7);
  std::vector<uint8_t> dec_raw = ZIP::UnzipPtr(enc.data(), enc.size());
  return dec_raw == v;
}

static void ShrinkExample(std::vector<uint8_t> v) {
  printf("Shrinking example of size %d.\n", (int)v.size());
  Periodically status_per(1.0);
  Periodically save_per(60.0);
  bool dirty = true;
  auto Save = [&dirty, &v]() {
      if (dirty) {
        const std::string filename = "counterexample";
        Util::WriteFileBytes(filename, v);
        printf("Wrote " AGREEN("%s") "\n", filename.c_str());
        dirty = false;
      }
    };

  int shrunk = 0;
  // First, binary search, trimming the tail.
  size_t lb = 0;
  while (lb < v.size()) {
    std::vector<uint8_t> cpy = v;
    size_t m = (lb + v.size()) >> 1;
    cpy.resize(m);
    if (RoundTripOK(cpy)) {
      lb = m + 1;
    } else {
      v = std::move(cpy);
      dirty = true;
      shrunk++;
    }
    if (status_per.ShouldRun()) {
      printf("shrunk %d. lb %d, size %d\n", shrunk, (int)lb, (int)v.size());
    }
  }
  printf("Binary search to size %d.\n", (int)v.size());
  Save();

  int pos = 0;
  int printified = 0;
  for (;;) {
    std::vector<uint8_t> cpy = v;

    pos %= cpy.size();

    if (cpy[pos] < 33 || cpy[pos] > 126) {
      uint8_t c = 33 + (cpy[pos] % (126 - 33));
      cpy[pos] = c;
      if (!RoundTripOK(cpy)) {
        v[pos] = c;
        dirty = true;
        printified++;
      }
    }

    // Try removing a byte.
    cpy.erase(cpy.begin() + pos);
    if (!RoundTripOK(cpy)) {
      v = std::move(cpy);
      dirty = true;
      shrunk++;
    }
    pos++;

    if (status_per.ShouldRun()) {
      printf("Current size %d. Shrunk %d. Printified %d.\n",
             (int)v.size(), shrunk, printified);
    }
    if (save_per.ShouldRun()) Save();
  }
}

[[maybe_unused]]
static void FindCounterexample() {
  ArcFour rc("zip_test");

  static constexpr int NUM_ROUNDS = 4096;
  for (int i = 0; i < NUM_ROUNDS; i++) {
    int size = 1 + RandTo(&rc, 131072 * 2);
    // int size = 128;
    std::vector<uint8_t> v;
    v.reserve(size);
    for (int x = 0; x < size; x++) v.push_back(rc.Byte());

    if (!RoundTripOK(v)) {
      printf("Found counterexample with %d bytes.\n",
             (int)v.size());
      ShrinkExample(std::move(v));
      return;
    }
  }
}

static void TestRoundTripFixed() {
  // TODO: Generate random test vectors (making sure to have some that
  // are compressible) and test that s == decompress(compress(s)).

  // TestOneRoundTrip("");

  TestOneRoundTrip("hi");

  TestOneRoundTrip(
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "123456789 3.141592653589\n"
      "aaaaaaaaaaaaaa"
      "a aardvark. a albatros. a appalacian.\n"
      "0000000001111111112222222222333333333\n"
      "2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384\n");

  printf("Fixed tests " AGREEN("OK") "\n");
}

static void TestRegression1() {
  std::string s(32769, 'x');
  TestOneRoundTrip(s);
}

static void TestRegression2() {
  std::vector<uint8_t> ctr = Util::ReadFileBytes("counterexample");
  if (ctr.empty()) {
    printf("Note: Missing the file 'counterexample'!\n");
    return;
  }
  std::vector<uint8_t> enc_raw = ZIP::ZipPtr(ctr.data(), ctr.size());
  std::vector<uint8_t> enc = StreamZipPtr(ctr.data(), ctr.size());
  if (enc_raw != enc) {
    printf("Encoded differ!!\n");
    printf("%s\n", DumpVectorDiff(enc_raw, enc).c_str());
  }

  std::vector<uint8_t> dec_raw = ZIP::UnzipPtr(enc_raw.data(),
                                               enc_raw.size());
  std::vector<uint8_t> dec = ZIP::UnzipPtr(enc.data(),
                                           enc.size());
  CHECK(dec_raw == ctr);
  CHECK(dec == ctr);
  printf("Regression 2 " AGREEN("OK") "\n");
}

static void TestLong1() {
  std::string s(33333, 'x');
  for (int i = 0; i < (int)s.size(); i = i + (i >> 2) + 1) {
    s[i] = 'O';
  }
  TestOneRoundTrip(s);
  printf("Long 1 " AGREEN("OK") "\n");
}

static void TestLong2() {
  std::vector<uint8_t> s(131072 * 16, '-');
  for (int i = 0; i < (int)s.size(); i++) {
    s[i] = 'a' + (i % 26);
  }
  TestOneRoundTrip(s);

  printf("Long 2 " AGREEN("OK") "\n");
}

static void TestRegression3() {
  ArcFour rc("reg3");
  Periodically status_per(0.25);
  Timer timer;

  printf("Regression 3:\n");
  static constexpr int SIZE = 1 << 16;
  for (int64_t s = 1; s < SIZE; s += 97) {

    std::vector<uint8_t> compressible;
    for (int i = 0; i < SIZE; i++)
      compressible.push_back(RandTo(&rc, 25) + 'a');

    std::vector<uint8_t> enc = ZIP::ZipVector(compressible, 9);

    std::unique_ptr<ZIP::DecodeBuffer> db(ZIP::DecodeBuffer::Create());

    std::vector<uint8_t> dec;
    dec.reserve(compressible.size());

    uint8_t *data = enc.data();
    int64_t size_left = enc.size();
    while (size_left > 0) {
      int64_t chunk = std::min(size_left, s);
      db->InsertPtr(data, chunk);
      size_left -= chunk;
      data += chunk;

      while (db->OutputSize() > 0) {
        for (uint8_t b : db->GetOutputVector()) {
          dec.push_back(b);
        }
      }
    }

    CHECK(dec == compressible) << s;
    if (status_per.ShouldRun()) {
      printf(ANSI_UP "%s\n",
             ANSI::ProgressBar(s, SIZE, "Regression 3",
                               timer.Seconds()).c_str());
    }
  }

  printf("Regression 3 " AGREEN("OK") "\n");
}

static void TestZlib() {
  for (const std::string s : {
      "lorem ipsum dolor sit amet",
      "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzza",
      "15:05:42 15:05:43 15:05:44 15:05:45 15:05:46 15:05:47",
      "",
      "a",
      "\xFF\x01\xFF" "1234567891234566789",
    }) {
    printf("[%s]\n", s.c_str());
    std::string enc = ZIP::ZlibString(s);
    std::string dec = ZIP::UnZlibString(enc);
    CHECK(s == dec) << s;
  }
  printf("zlib round trip " AGREEN("OK") "\n");
}

int main(int argc, char **argv) {
  ANSI::Init();

  TestZlib();
  TestRegression1();
  TestLong1();
  TestLong2();
  TestRoundTripFixed();
  TestRegression2();
  TestRegression3();
  // FindCounterexample();
  TestRoundTripRandom();

  printf("OK\n");
  return 0;
}

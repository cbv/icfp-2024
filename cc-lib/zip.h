
#ifndef _CC_LIB_ZIP_H
#define _CC_LIB_ZIP_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Simple flate codec, wrapping miniz.
// The primary functions produce raw streams (RFC 1951) with no header nor
// footer, which you'd typically embed inside some other file format. It is
// often useful to know the size of the decompressed data, for example.
//
// Below I define a nonstandard "CCZ" header (to be read or written
// manually) that I use in some cc-lib code. There are also some
// convenience functions for decompressing standard zlib (RFC 1950)
// data.

struct ZIP {

  // The compression "level" can range from 0 to 9. Higher is slower,
  // but generates smaller files.

  // TODO: These should perhaps be renamed to "Flate"?

  // The simplest functions operate on the entire input at once.
  static std::vector<uint8_t> ZipVector(const std::vector<uint8_t> &v,
                                        int level = 7);
  static std::string ZipString(std::string_view s, int level = 7);
  static std::vector<uint8_t> ZipPtr(const uint8_t *data, size_t size,
                                     int level = 7);

  static std::vector<uint8_t> UnzipVector(const std::vector<uint8_t> &v);
  static std::string UnzipString(std::string_view s);
  static std::vector<uint8_t> UnzipPtr(const uint8_t *data, size_t size);

  // Streaming interfaces.

  struct EncodeBuffer {
    // Level ranges from 0 to 10; higher levels are slower but can
    // produce better compression.
    static EncodeBuffer *Create(int level = 7);
    virtual ~EncodeBuffer();

    // Insert raw data to be compressed. New compressed bytes may
    // become available in the output.
    virtual void InsertVector(const std::vector<uint8_t> &v) = 0;
    virtual void InsertPtr(const uint8_t *data, size_t size) = 0;
    virtual void InsertString(const std::string &s) = 0;

    virtual void Finalize() = 0;

    // Number of bytes that are ready.
    virtual size_t OutputSize() const = 0;
    virtual std::vector<uint8_t> GetOutputVector() = 0;
    virtual std::string GetOutputString() = 0;
    // Write up to size bytes; return the number written.
    virtual size_t WriteOutput(uint8_t *data, size_t size) = 0;

   protected:
    // Use Create.
    EncodeBuffer();
  };

  struct DecodeBuffer {
    static DecodeBuffer *Create();
    virtual ~DecodeBuffer();

    // Insert compressed data. New decompressed bytes may become
    // available in the output.
    virtual void InsertVector(const std::vector<uint8_t> &v) = 0;
    virtual void InsertPtr(const uint8_t *data, size_t size) = 0;
    virtual void InsertString(const std::string &s) = 0;

    // XXX need some way to indicate failure.

    // Number of bytes that are ready.
    virtual size_t OutputSize() const = 0;
    virtual std::vector<uint8_t> GetOutputVector() = 0;
    virtual std::string GetOutputString() = 0;
    // Write up to size bytes; return the number written.
    virtual size_t WriteOutput(uint8_t *data, size_t size) = 0;
   protected:
    // Use Create.
    DecodeBuffer();
  };

  // Custom file format. I should also support pkzip and gzip here.
  // gzip, annoyingly, stores the size mod 2^32, which is a practical
  // problem for modern files (e.g. wikipedia far exceeds this).
  struct CCLibHeader {
    // Always 16 bytes.
    uint8_t magic[4] = {'C', 'c', 'Z', 'z'};
    uint8_t flags_msb_first[4] = {};
    uint8_t size_msb_first[8] = {};

    // Flags are not used yet, so they should all be zero.
    // This could include stuff like a delta coder.
    void SetFlags(uint32_t f);
    uint32_t GetFlags() const;

    void SetSize(uint64_t s);
    uint64_t GetSize() const;

    bool HasCorrectMagic() const;

    // Just memcpy into the struct.
    void ParseHeader(uint8_t *data);
  };

  // TODO
  /*
  std::vector<uint8_t> CompressWithHeader(const uint8_t *data,
                                          size_t size,
                                          int level = 7);

  std::vector<uint8_t> DecompressWithHeader(const uint8_t *data,
                                            size_t size);
  */

  // With a ZLIB header and Adler-32 checksum at the end (RFC 1950).
  // This format is not great because the length field is only
  // 32-bit (it should work with data >4GB, but it means the length
  // is not really useful). The checksum also has some weaknesses.
  static std::vector<uint8_t> ZlibVector(const std::vector<uint8_t> &v,
                                         int level = 7);
  static std::string ZlibString(std::string_view s, int level = 7);
  static std::vector<uint8_t> ZlibPtr(const uint8_t *data, size_t size,
                                     int level = 7);

  static std::vector<uint8_t> UnZlibVector(const std::vector<uint8_t> &v);
  static std::string UnZlibString(std::string_view s);
  static std::vector<uint8_t> UnZlibPtr(const uint8_t *data, size_t size);
};

#endif

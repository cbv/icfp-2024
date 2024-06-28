
#ifndef _CC_LIB_BITBUFFER_H
#define _CC_LIB_BITBUFFER_H

#include <string>
#include <vector>
#include <cstdint>

// Writes a sequence of bits to a byte stream.
//
// The first bit in the stream is the highest bit of the first byte.
// The bytes are padded with trailing zeroes, if the number of bits
// does not divide 8.

struct BitBuffer {
#if 0
  // TODO: Some kind of reading support. This is what we used to have,
  // but I think we can make a better interface.
  //
  /* read n bits from the string s from bit offset idx.
     (high bits first)
     write that to output and return true.
     if an error occurs (such as going beyond the end of the string),
     then return false, perhaps destroying idx and output */
  static bool nbits(const std::string &s, int n, int &idx, uint32_t &output);
#endif

  /* create a new empty bit buffer */
  BitBuffer() { }

  /* Appends the n low-order bits to the bit buffer. */
  void WriteBits(int n, uint32_t thebits);
  void WriteBit(bool bit);

  /* Get the full contents of the buffer as a string,
     padded with zeroes at the end if necessary. */
  std::string GetString() const;
  std::vector<uint8_t> GetBytes() const;

  int64_t NumBits() const { return num_bits; }
  // Hint that we will want to store this many bits.
  void Reserve(int64_t bits);

  /* give the number of bytes needed to store n bits */
  static inline int64_t Ceil(int64_t bits) {
    return (bits >> 3) + !!(bits & 7);
  }

  ~BitBuffer() {}

 private:
  // Always Ceil(bytes) bytes, with trailing zero bits.
  std::vector<uint8_t> bytes;
  int64_t num_bits = 0;
};

#endif

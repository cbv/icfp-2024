#include "bitbuffer.h"

#include <string>
#include <vector>
#include <cstdint>

using namespace std;

#if 0
bool BitBuffer::nbits(const string &s, int n, int &idx, unsigned int &out) {
# define NTHBIT(x) !! (s[(x) >> 3] & (1 << (7 - ((x) & 7))))

  out = 0;

  while (n--) {
    out <<= 1;
    /* check bounds */
    if ((unsigned)(idx >> 3) >= s.length()) return false;
    out |= NTHBIT(idx);
    idx ++;
  }
  return true;

# undef NTHBIT
}
#endif

string BitBuffer::GetString() const {
  string s;
  s.reserve(bytes.size());
  for (uint8_t b : bytes) s.push_back(b);
  return s;
}

std::vector<uint8_t> BitBuffer::GetBytes() const {
  return bytes;
}

void BitBuffer::Reserve(int64_t bits) {
  bytes.reserve(Ceil(bits));
}

void BitBuffer::WriteBit(bool bit) {
  if ((num_bits & 7) == 0) {
    bytes.push_back(0x00);
  }

  if (bit) {
    bytes[num_bits >> 3] |= (1 << (7 - (num_bits & 7)));
  }
  num_bits++;
}

void BitBuffer::WriteBits(int n, unsigned int b) {
  for (int i = 0; i < n; i ++) {
    bool bit = !!(b & (1 << (n - (i + 1))));
    WriteBit(bit);
  }
}


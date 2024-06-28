
// Linear feedback shift register.
// For an n-bit word, this allows a permutation of length 2^n-1.
//
// This is not even close to being cryptographic, and also has
// bad randomness (e.g. consecutive states are highly correlated) but
// the code is very compact.

#ifndef _CC_LIB_CRYPT_LFSR_H
#define _CC_LIB_CRYPT_LFSR_H

#include <bit>
#include <cstdint>
#include <type_traits>

template<class W, W poly>
inline constexpr W LFSRNext(W state) {
  static_assert(std::is_unsigned<W>::value, "LFSR requires unsigned int");
  // constexpr int num_bits = std::bit_width((W)-1);
  const W bit = std::popcount<W>(state & poly) & 1;
  return (state << 1) | bit;
}

// Some LFSRs with maximum period.

inline constexpr uint32_t LFSRNext32(uint32_t state) {
  return LFSRNext<uint32_t, 0x8D777777>(state);
}

inline constexpr uint16_t LFSRNext16(uint16_t state) {
  // For 16, other options include 0xD008, ...
  return LFSRNext<uint16_t, 0xBDDD>(state);
}

inline constexpr uint8_t LFSRNext8(uint8_t state) {
  return LFSRNext<uint8_t, 0xB4>(state);
}

#endif

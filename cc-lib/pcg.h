
// Implementation of PCG Random number generator (see pcg-random.org).

#ifndef _CC_LIB_PCG_H
#define _CC_LIB_PCG_H

#include <cstdint>
#include <string_view>

// A single stream with 64-bit internal state.
struct PCG32 {

  // Deterministically seed the generator such that it is
  // in a state uncorrelated with the seed.
  inline explicit PCG32(uint64_t seed);
  inline explicit PCG32(std::string_view seed);

  // Advance the state and give uniformly random bits.
  inline uint64_t Rand64();
  // This is the native size.
  inline uint32_t Rand32();
  inline uint8_t Byte();

  // For serialization, rewinding, etc.
  inline uint64_t GetState() const { return state; }
  // Recreate the stream from the state. Recommended to
  // generate new streams using the constructors, as
  // PCG is pipelined such that the first element of
  // the stream is just a permutation of the state.
  static inline PCG32 FromState(uint64_t state);

  // Valid, but low quality seed (0).
  PCG32() {}

  // Value semantics.
  PCG32(const PCG32 &other) = default;
  PCG32(PCG32 &&other) = default;
  PCG32 &operator =(const PCG32 &other) = default;
  PCG32 &operator =(PCG32 &&other) = default;

private:

  uint64_t state = 0;
};


// Implementation follows.

PCG32::PCG32(uint64_t seed) : state(seed) {
  (void)Rand32();
}

uint32_t PCG32::Rand32() {
  // Based on the minimal C code by Melissa O'Neill.
  // If you consider this a derivative work:
  //   Original code was MIT (or Apache 2.0) licensed;
  //   see pcg-random.org.
  static constexpr uint64_t MULT = 6364136223846793005ULL;
  static constexpr uint64_t INC = 1442695040888963407ULL;
  const uint64_t prev = state;
  state = prev * MULT + INC;
  const uint32_t xorshifted = ((prev >> 18u) ^ prev) >> 27u;
  const uint32_t rot = prev >> 59u;
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint64_t PCG32::Rand64() {
  uint64_t ret = Rand32();
  ret <<= 32;
  ret |= Rand32();
  return ret;
}

uint8_t PCG32::Byte() {
  return Rand32() & 0xFF;
}

PCG32 PCG32::FromState(uint64_t state) {
  PCG32 pcg;
  pcg.state = state;
  return pcg;
}

// Common to want to seed with some string. We only have 64 bits of
// internal state and aren't trying to be cryptographic, so there's
// not much point in trying to get a lot of juice out of it. We just
// want something that depends on all bits of the input (and is
// sensitive to order). Internally, the PCG state is just a linear
// congruential generator (the xorshift stuff is for its output, which
// is ignored here) so this ends up being some polynomial of the
// bytes of the input (mod 2^64).
PCG32::PCG32(std::string_view seed) : state(1) {
  const uint8_t *data = (const uint8_t*)seed.data();
  int idx = 0;
  for (; idx < (int)seed.size() - 8; idx += 8) {
    // Be explicit about byte order so that we get the
    // same behavior on any platform, but this can just
    // be a load on x86.
    uint64_t word = 0;
    for (int i = 7; i >= 0; i--) {
      word <<= 8;
      word |= data[idx + i];
    }

    (void)Rand32();
    state += word;
  }

  // And any trailing bytes.
  for (; idx < (int)seed.size(); idx++) {
    (void)Rand32();
    state += data[idx];
  }

  // Always advance at least once.
  (void)Rand32();
}

#endif

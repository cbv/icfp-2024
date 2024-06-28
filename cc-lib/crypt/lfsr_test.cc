
#include <cstdint>
#include "crypt/lfsr.h"

#include "base/logging.h"
#include "timer.h"

#if 0
template<class W>
inline W LFSRNextX(W state) {
  static_assert(std::is_unsigned<W>::value, "LFSR requires unsigned int");
  CHECK(state != 0);
  constexpr int num_bits = std::bit_width((W)-1);
  constexpr uint8_t poly = 0xB4;
  const W bit = std::popcount<W>(state & poly) & 1;
  W xbit = 0;
  for (int sh = 0; sh < num_bits; sh++) {
    xbit ^= (((state & poly) >> sh) & 1);
  }
  printf("num_bits: %d state %02x bit %d xbit %d\n",
         num_bits, state, bit, xbit);
  CHECK(bit == xbit);
  return (state << 1) | (bit & 1); // (bit << (num_bits - 1));
}
#endif


static void Test8() {
  const uint8_t start_state = 0x01;
  int iters = 0;
  uint8_t state = start_state;
  // LFSRNextX<uint8_t>(start_state);
  do {
    state = LFSRNext8(state);
    iters++;
    CHECK(iters < 300);
  } while (state != start_state);
  CHECK(iters == 255) << iters;
}


static void Test16() {
  const uint16_t start_state = 0x01;
  int iters = 0;
  uint16_t state = start_state;
  do {
    state = LFSRNext16(state);
    iters++;
    CHECK(iters < 66000);
  } while (state != start_state);
  CHECK(iters == 65535) << iters;
}

static void Test32() {
  Timer timer;
  const uint32_t start_state = 0x01;
  int64_t iters = 0;
  uint32_t state = start_state;
  do {
    state = LFSRNext32(state);
    iters++;
    CHECK(iters < 0x100001000);
  } while (state != start_state);
  CHECK(iters == 0xFFFFFFFF) << iters;
  fprintf(stderr, "32 bit cycle in %.3fs\n", timer.Seconds());
}

int main(int argc, char **argv) {
  Test8();
  Test16();
  Test32();

  printf("OK\n");
  return 0;
}

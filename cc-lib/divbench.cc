
// This standalone program is for trying out some tricks
// for the initial division phase in factorize.cc; it can be ignored
// and eventually deleted!

#include <vector>
#include <utility>
#include <cstdint>
#include <cstdio>
#include <bit>

#include "timer.h"
#include "ansi.h"
#include "crypt/lfsr.h"

auto PushFactor = [](std::vector<std::pair<uint64_t, int>> *factors,
                     uint64_t b) {
    if (!factors->empty() &&
        factors->back().first == b) {
      factors->back().second++;
    } else {
      factors->emplace_back(b, 1);
    }
  };

static std::vector<std::pair<uint64_t, int>>
InternalFactorize(uint64_t x) {

  if (x <= 1) return {};

  uint64_t cur = x;

  // Factors in increasing order.
  std::vector<std::pair<uint64_t, int>> factors;

#define USE_LOCAL 0

  #if USE_LOCAL
  // local storage
  // 3^41 is the most factors possible for uint64, after eliminating 2.
  uint8_t fs[42];
  int num_fs = 0;
  auto PushLocalFactor = [&fs, &num_fs](uint8_t p) {
      fs[num_fs++] = p;
  };
  #endif

  int twos = std::countr_zero<uint64_t>(x);
  if (twos) {
    factors.emplace_back(2, twos);
    cur >>= twos;
  }

  // Try the first 32 primes. This code used to have a much longer
  // list, but it's counterproductive. With hard-coded constants,
  // we can also take advantage of division-free compiler tricks.
  //
  // PERF: Probably can pipeline faster if we don't actually divide until
  // the end of all the factors?
#if USE_LOCAL
  #define TRY(p) while (cur % p == 0) { cur /= p; PushLocalFactor(p); }
#else
  #define TRY(p) while (cur % p == 0) { cur /= p; PushFactor(&factors, p); }
#endif
  // #define TRY(p) if (cur % p == 0) { PushFactor(&factors, p); divi *= p; }
  // #define TRY(p) uint8_t c ## p; while (cur % p == 0) { cur /= p; c ## p++; } if (c ## p) PushLocalFactor(p, c ## p)

#if USE_LOCAL
  if (cur % 9 == 0) { cur /= 9; PushLocalFactor(3); PushLocalFactor(3); }
  if (cur % 15 == 0) { cur /= 15; PushLocalFactor(3); PushLocalFactor(5); }
#else
  if (cur % 9 == 0) { cur /= 9; PushFactor(&factors, 3); PushFactor(&factors, 3); }
  if (cur % 15 == 0) { cur /= 15; PushFactor(&factors, 3); PushFactor(&factors, 5); }
#endif

  TRY(3);
  TRY(5);
  TRY(7);
  TRY(11);
  TRY(13);
  TRY(17);
  TRY(19);
  TRY(23);
  TRY(29);
  TRY(31);
  TRY(37);
  TRY(41);
  TRY(43);
  TRY(47);
  TRY(53);
  TRY(59);
  TRY(61);
  TRY(67);
  TRY(71);
  TRY(73);
  TRY(79);
  TRY(83);
  TRY(89);
  TRY(97);
  TRY(101);
  TRY(103);
  TRY(107);
  TRY(109);
  TRY(113);
  TRY(127);
  TRY(131);

#if USE_LOCAL
  for (int i = 0; i < num_fs; i++) PushFactor(&factors, fs[i]);
#endif

  // if (cur == 1) return factors;
  return factors;
}

static double Bench() {
  Timer timer;
  uint32_t a = 0x12345678;
  uint32_t b = 0xACABACAB;
  uint64_t sum = 0xBEEF;
  for (int j = 0; j < 60000000; j++) {
    uint64_t input = ((uint64_t)a) << 32 | b;
    auto factors = InternalFactorize(input);
    a = LFSRNext32(LFSRNext32(a));
    b = LFSRNext32(b);
    for (const auto &[x, y] : factors) sum += x + y;
  }
  double sec = timer.Seconds();
  printf("%llu %s\n",
         sum,
         ANSI::Time(sec).c_str());
  return sec;
}

int main(int argc, char **argv) {
  AnsiInit();
  printf("Begin.\n");
  Bench();

  return 0;
}

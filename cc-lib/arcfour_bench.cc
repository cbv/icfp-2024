#include "arcfour.h"

#include "base/logging.h"
#include <stdio.h>
#include <cstdint>
#include <vector>
#include <string>

#include "timer.h"

using namespace std;
using uint8 = uint8_t;
using int64 = int64_t;

static void BenchConstruct() {
  Timer timer;
  int64 NUM_CONSTRUCT = 5000000;
  uint64_t result = 0xFADE;
  string init = "benchmarking";
  for (int i = 0; i < NUM_CONSTRUCT; i++) {
    // use a different initializer, but not an
    // empty string.
    init[1] = result & 255;
    ArcFour rc(init);
    result *= 0x31337;
    result += rc.Byte();
  }
  double seconds = timer.Seconds();
  printf("Result: %llx\n", result);
  CHECK(result == 0xd36b15846cc1c15bULL);
  printf("Construct %lld in %.3fs\n"
         "%.3f kc/sec\n", NUM_CONSTRUCT, seconds,
         NUM_CONSTRUCT / (seconds * 1000.0));
}

static uint64_t Rand64(ArcFour *rc) {
  uint64_t uu = 0ULL;
  uu = rc->Byte() | (uu << 8);
  uu = rc->Byte() | (uu << 8);
  uu = rc->Byte() | (uu << 8);
  uu = rc->Byte() | (uu << 8);
  uu = rc->Byte() | (uu << 8);
  uu = rc->Byte() | (uu << 8);
  uu = rc->Byte() | (uu << 8);
  uu = rc->Byte() | (uu << 8);
  return uu;
};

static void Bench64() {
  int64 NUM_SAMPLES = 500000000;
  uint64_t result = 0xFADE;
  ArcFour rc("bench");
  Timer timer;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    result *= 0x31337;
    result = (result >> 15) | (result << (64 - 15));
    result ^= Rand64(&rc);
  }
  double seconds = timer.Seconds();
  printf("Result: %llx\n", result);
  // CHECK(result == 0xd36b15846cc1c15bULL);
  printf("Sample %lld in %.3fs\n"
         "%.3f Msamples/sec\n", NUM_SAMPLES, seconds,
         NUM_SAMPLES / (seconds * 1000000.0));
}

int main(int argc, char **argv) {
  BenchConstruct();
  Bench64();
  printf("OK\n");
  return 0;
}


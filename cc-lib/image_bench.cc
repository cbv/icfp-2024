
#include "image.h"

#include <cstdint>

#include "base/logging.h"
#include "timer.h"
#include "crypt/lfsr.h"

using int64 = int64_t;

static uint64_t ImageHash(const ImageRGBA &image) {
  uint64_t state = image.Width() * 0x100000000 + image.Height();
  for (int y = 0; y < image.Height(); y++) {
    for (int x = 0; x < image.Width(); x++) {
      uint32_t c = image.GetPixel32(x, y);
      state = (state << 13) | (state >> (64 - 13));
      state *= 0x31337CAFE;
      state ^= c;
    }
  }
  return state;
}

static void BenchBlendPixel() {
  ImageRGBA image(250, 500);
  image.Clear32(0x000000FF);

  Timer timer;
  static constexpr int64 NUM_PIXELS = 1000000000;
  uint32_t state = 0x12345678;
  for (int64 i = 0; i < NUM_PIXELS; i++) {
    state = LFSRNext32(state);
    const uint32_t color = state;
    state = LFSRNext32(state);
    int x = state & 0xFF;
    int y = (state >> 8) & 0x1FF;
    image.BlendPixel32(x, y, color);
  }
  double sec = timer.Seconds();

  printf("BlendPixel: %lld pixels in %.3f sec =\n"
         "%.3f Mp/sec\n", NUM_PIXELS, sec,
         NUM_PIXELS / (sec * 1000000.0));

  image.Save("bench.png");
  uint64_t result = ImageHash(image);
  CHECK(result == 0x5cf6c52db1b2bd29ULL);
  printf("Result: %llx\n", result);
}

static void BenchClear32() {
  ImageRGBA image(250, 500);

  Timer timer;
  static constexpr int64 NUM_CLEARS = 1000000;
  uint32_t state = 0x12345678;
  for (int64 i = 0; i < NUM_CLEARS; i++) {
    state = LFSRNext32(state);
    image.Clear32(state);
  }

  double sec = timer.Seconds();
  printf("Clear32: %lld clears in %.3f sec =\n"
         "%.3f Kc/sec\n", NUM_CLEARS, sec,
         NUM_CLEARS / (sec * 1000.0));
}

int main(int argc, char **argv) {
  BenchBlendPixel();
  BenchClear32();

  printf("OK\n");
  return 0;
}

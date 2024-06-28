
#include "image.h"

#include "base/stringprintf.h"
#include "base/logging.h"
#include "arcfour.h"
#include "randutil.h"
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

int main(int argc, char **argv) {
  BenchBlendPixel();

  printf("OK\n");
  return 0;
}

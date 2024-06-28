
#include "hilbert-curve.h"

#include <string>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cinttypes>

#include "base/logging.h"
#include "base/stringprintf.h"
#include "arcfour.h"
#include "randutil.h"
#include "image.h"

using uint8 = uint8_t;

static void DrawLine() {
  // Square with side length 2^base
  constexpr int base = 6;
  const int side = 1 << base;
  ImageRGBA img(side * 3, side * 3);
  img.Clear32(0x000000FF);
  int oldx, oldy;
  std::tie(oldx, oldy) = HilbertCurve::To2D(base, 0);
  for (int d = 1; d < side * side; d++) {
    const auto [x, y] = HilbertCurve::To2D(base, d);
    CHECK(x >= 0 && x < side);
    CHECK(y >= 0 && y < side);
    float f = (double)d / (double)(side * side - 1);
    uint8 v = std::clamp(0, (int)round(f * 255.0), 255);
    img.BlendLine(3 * oldx + 1, 3 * oldy + 1,
                  3 * x + 1, 3 * y + 1,
                  v, 0x77, (0xFF - v), 0xFF);
    // img.SetPixel(x, y, v, 0xFF, v, 0xFF);
    oldx = x;
    oldy = y;
  }
  std::string filename = StringPrintf("lines-%d.png", base);
  img.ScaleBy(4).Save(filename);
  printf("Wrote %s\n", filename.c_str());
}


static void Brightness() {
  // Square with side length 2^base
  constexpr int base = 8;
  const int side = 1 << base;
  ImageRGBA img(side, side);
  img.Clear32(0x000000FF);
  for (int d = 0; d < side * side; d++) {
    const auto [x, y] = HilbertCurve::To2D(base, d);
    CHECK(x >= 0 && x < side);
    CHECK(y >= 0 && y < side);
    float f = (double)d / (double)(side * side - 1);
    uint8 v = std::clamp(0, (int)round(f * 255.0), 255);
    img.SetPixel(x, y, v, v, v, 0xFF);
  }
  std::string filename = StringPrintf("brightness-%d.png", base);
  img.Save(filename);
  printf("Wrote %s\n", filename.c_str());
}

// Check that the functions are inverses for all valid inputs on
// small bases.
static void Inv() {
  for (int base = 1; base < 11; base++) {
    const int side = 1 << base;
    for (int d = 0; d < side * side; d++) {
      const auto [x, y] = HilbertCurve::To2D(base, d);
      CHECK(x >= 0 && x < side);
      CHECK(y >= 0 && y < side);
      const uint64_t dd = HilbertCurve::To1D(base, x, y);
      CHECK(d == dd) << x << " " << y;
    }
  }
}

// Spot check large bases
static void Spot() {
  ArcFour rc("hilbert");

  // for (int base = 11; base <= 32; base++) {
  for (int base = 11; base <= 32; base++) {
    const uint64_t side = uint64_t{1} << base;
    printf("Base %d: %" PRIu64 " x %" PRIu64 " \n", base, side, side);

    for (int samples = 0; samples < 10000; samples++) {
      {
        CHECK(base <= 32);
        // Don't use RandTo because side * side overflows int64 (to zero)
        // for base 32. But this approach will always produce a d in
        // range.
        uint64_t xd = RandTo(&rc, side);
        uint64_t yd = RandTo(&rc, side);
        uint64_t d = yd * side + xd;
        const auto [x, y] = HilbertCurve::To2D(base, d);
        CHECK(x >= 0 && x < side);
        CHECK(y >= 0 && y < side);
        const uint64_t dd = HilbertCurve::To1D(base, x, y);
        CHECK(d == dd) << "base " << base << ": "
                       << d << " -> " << x << "," << y << " -> " << dd;
      }

      {
        uint32_t x = RandTo(&rc, side);
        uint32_t y = RandTo(&rc, side);

        uint64_t d = HilbertCurve::To1D(base, x, y);
        if (base < 32) {
          CHECK(d >= 0 && d < side * side) << "base " << base << ": "
                                           << x << "," << y << " -> "
                                           << d;
        } else {
          // 64-bit int is always less than 2^32 * 2^32, but
          // side * side would overflow to zero.
        }

        const auto [xx, yy] = HilbertCurve::To2D(base, d);
        CHECK(x == xx);
        CHECK(y == yy);
      }
    }
  }
}


int main() {
  DrawLine();
  Brightness();
  Inv();
  Spot();
  printf("OK\n");
  return 0;
}


// Discrete 1D to 2D isomorphism on squares of 2^b, using a
// Hilbert curve.

#ifndef _CC_LIB_HILBERT_CURVE_H
#define _CC_LIB_HILBERT_CURVE_H

#include <tuple>
#include <cstdint>

// Hilbert curve with dimension b.
struct HilbertCurve {
  // Convert 2D coordinates, each in [0, 2^b) to a 1D position
  // along the curve
  // each coordinate in [0, 2^b).
  static uint64_t To1D(int b, uint64_t x, uint64_t y) {
    const uint64_t n = uint64_t{1} << b;
    uint64_t d = 0;
    for (uint64_t s = n >> 1; s > 0; s >>= 1) {
      uint32_t rx = (x & s) > 0;
      uint32_t ry = (y & s) > 0;
      d += s * s * ((3 * rx) ^ ry);
      Rotate(n, &x, &y, rx, ry);
    }
    return d;
  }

  // Convert a 1D coordinate in [0, 2^b * 2^b) to a 2D position,
  // each coordinate in [0, 2^b).
  static std::tuple<uint32_t, uint32_t> To2D(int b, uint64_t d) {
    const uint64_t n = uint64_t{1} << b;
    uint64_t t = d;
    uint64_t x = 0, y = 0;
    for (uint64_t s = 1; s < n; s <<= 1) {
      uint32_t rx = 1 & (t >> 1);
      uint32_t ry = 1 & (t ^ rx);
      Rotate(s, &x, &y, rx, ry);
      x += s * rx;
      y += s * ry;
      t >>= 2;
    }
    return std::make_tuple((uint32_t)x, (uint32_t)y);
  }

private:
  // rotate/flip a quadrant appropriately
  static void Rotate(uint64_t n, uint64_t *x, uint64_t *y,
                     uint32_t rx, uint32_t ry) {
    if (ry == 0) {
      if (rx == 1) {
        *x = n - 1 - *x;
        *y = n - 1 - *y;
      }

      // Swap x and y
      const auto t = *x;
      *x = *y;
      *y = t;
    }
  }
};

#endif

// Code is adapted from Wikipedia (cc-by-sa)
// Author: "109.171.137.224", 11 June 2011
// en.wikipedia.org/w/index.php?title=Hilbert_curve&oldid=433709218

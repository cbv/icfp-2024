#ifndef _NUMBERS_H
#define _NUMBERS_H

#include <tuple>
#include <cstdint>
#include <optional>

#include "base/int128.h"
#include "base/logging.h"

#include "montgomery64.h"

// Returns (gcd, x, y)
// where we have ax * by = gcd = gcd(a, b)
std::tuple<int64_t, int64_t, int64_t>
ExtendedGCD64(int64_t a, int64_t b);

// Slower recursive version of above for reference.
std::tuple<int64_t, int64_t, int64_t>
ReferenceExtendedGCD64(int64_t a, int64_t b);

// compute a^1 mod b    for a,b coprime
inline int64_t ModularInverse64(int64_t a, int64_t b) {
  // if (a < 0 && b < 0) { a = -a; b = -b; }

  const int64_t absb = abs(b);

  const auto &[gcd, x, y] = ExtendedGCD64(a, absb);
  CHECK(gcd == 1) << "Precondition. gcd("
                  << a << ", " << b << "): " << gcd;
  // Now we have
  // ax + by = gcd = 1
  // and so
  // ax + by = 1 (mod b)
  //
  // ax = 1  (mod b)
  //
  // so x is a^1 mod b.

  if (x < 0) return x + absb;
  return x;
}

// Called with uint64_t n, so we could use that?
inline int Jacobi64(int64_t a, int64_t n) {
  CHECK(n > 0 && (n & 1) == 1) << "Precondition. n: " << n;

  int t = 1;
  a %= n;
  if (a < 0) a += n;

  while (a != 0) {

    // PERF could do this with countr_zero?
    // n is not changing in this loop.
    while ((a & 1) == 0) {
      a >>= 1;
      const int r = n % 8;
      if (r == 3 || r == 5) {
        t = -t;
      }
    }

    std::swap(n, a);
    if (a % 4 == 3 && n % 4 == 3) {
      t = -t;
    }

    a %= n;
  }

  if (n == 1) {
    return t;
  } else {
    return 0;
  }
}

// Returns x such that (x * x) mod p = xx, if it exists.
std::optional<uint64_t> SqrtModP(uint64_t xx, uint64_t p);
// As above, but doesn't compute the square root.
bool IsSquareModP(uint64_t xx, uint64_t p);

// Only for odd primes p. Already in Montgomery form.
inline bool IsSquareModP(Montgomery64 xx,
                         const MontgomeryRep64 &p) {
  // Compute Euler criteria. a^((p-1) / 2) must be 1.
  // Since p odd prime, we can just shift down by one
  // to compute (p - 1)/2.
  Montgomery64 rm = p.Pow(xx, p.Modulus() >> 1);

  if (p.Eq(rm, p.One())) return true;

  // But as a special case, the sqrt(0) is always 0.
  if (p.Eq(xx, p.Zero())) return true;

  // Otherwise there is no solution.
  return false;
}

inline int64_t DivFloor64(int64_t numer, int64_t denom) {
  // There's probably a version without %, but I verified
  // that gcc will do these both with one IDIV.
  int64_t q = numer / denom;
  int64_t r = numer % denom;
  // sign of remainder: -1, 0 or 1
  // int64_t sgn_r = (r > 0) - (r < 0);
  int64_t sgn_r = (r>>63) | ((uint64_t)-r >> 63);
  // The negated sign of the denominator. Note the denominator
  // can't be zero.
  int64_t sgn_neg_denom = denom < 0 ? 1 : -1;
  if (sgn_r == sgn_neg_denom) {
    return q - 1;
  }
  return q;

  // TODO: Benchmark these.
  //
  // An alternate version produces shorter code, but gcc generates
  // a branch for testing that r is nonzero:
  // if (r && ((r ^ denom) & (1LL << 63))) ...
  //
  // And the clearest expression generates loads of branches:
  // if ((r < 0 && denom > 0) || (r > 0 && denom < 0)) ...
}

// (a * b) % m
// Could maybe be wrong for largest a,b?
inline int64_t BasicModMult64(int64_t a, int64_t b,
                              uint64_t m) {
  // PERF: Compare UDiv128Rem from factorize. Might not handle
  // negative args though?

  // PERF: Check that this is making appropriate simplifications
  // knowing that each high word is zero.
  int128 aa(a);
  int128 bb(b);
  int128 mm(m);

  int128 rr = (aa * bb) % mm;
  return (int64_t)rr;
}

inline uint64_t Pow64(uint64_t base, int exp) {
  uint64_t res = 1;
  while (exp) {
    if (exp & 1)
      res *= base;
    exp >>= 1;
    base *= base;
  }
  return res;
}

inline uint64_t Sqrt64(uint64_t n) {
  if (n == 0) return 0;
  uint64_t r = std::sqrt((double)n);
  return r - (r * r - 1 >= n);
}

#endif

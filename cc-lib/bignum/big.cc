
#include "bignum/big.h"

#include <array>
#include <bit>
#include <cstdint>
#include <utility>
#include <vector>

// TODO: Move to big-util or something so that we don't
// need to include it by default?

using namespace std;

/* Factorization derives from the GPL factor.c, which is part of GNU
   coreutils. Copyright (C) 1986-2023 Free Software Foundation, Inc.

   (This whole file is distributed under the terms of the GPL; see
   COPYING.)

   Contributors: Paul Rubin, Jim Meyering, James Youngman, Torbjörn
   Granlund, Niels Möller.
*/

// 64-bit factoring, from ../factorization.cc. We could call the
// functions directly but I want bignum to remain standalone.
namespace {
struct Factor64 {
 private:
  // Note that gaps between primes are <= 250 all the
  // way up to 387096383. So we could have a lot more here. But
  // I saw worse results from trial division when increasing this
  // list beyond about 32 elements.
  //
  // XXX coreutils has 5000 primes, and might depend on that for the
  // correctness of the Lucas primality test?
  static constexpr std::array<uint8_t, 999> PRIME_DELTAS = {
  1,2,2,4,2,4,2,4,6,2,6,4,2,4,6,6,2,6,4,2,6,4,6,8,4,2,4,2,4,
  14,4,6,2,10,2,6,6,4,6,6,2,10,2,4,2,12,12,4,2,4,6,2,10,6,6,6,2,6,
  4,2,10,14,4,2,4,14,6,10,2,4,6,8,6,6,4,6,8,4,8,10,2,10,2,6,4,6,8,
  4,2,4,12,8,4,8,4,6,12,2,18,6,10,6,6,2,6,10,6,6,2,6,6,4,2,12,10,2,
  4,6,6,2,12,4,6,8,10,8,10,8,6,6,4,8,6,4,8,4,14,10,12,2,10,2,4,2,10,
  14,4,2,4,14,4,2,4,20,4,8,10,8,4,6,6,14,4,6,6,8,6,12,4,6,2,10,2,6,
  10,2,10,2,6,18,4,2,4,6,6,8,6,6,22,2,10,8,10,6,6,8,12,4,6,6,2,6,12,
  10,18,2,4,6,2,6,4,2,4,12,2,6,34,6,6,8,18,10,14,4,2,4,6,8,4,2,6,12,
  10,2,4,2,4,6,12,12,8,12,6,4,6,8,4,8,4,14,4,6,2,4,6,2,6,10,20,6,4,
  2,24,4,2,10,12,2,10,8,6,6,6,18,6,4,2,12,10,12,8,16,14,6,4,2,4,2,10,12,
  6,6,18,2,16,2,22,6,8,6,4,2,4,8,6,10,2,10,14,10,6,12,2,4,2,10,12,2,16,
  2,6,4,2,10,8,18,24,4,6,8,16,2,4,8,16,2,4,8,6,6,4,12,2,22,6,2,6,4,
  6,14,6,4,2,6,4,6,12,6,6,14,4,6,12,8,6,4,26,18,10,8,4,6,2,6,22,12,2,
  16,8,4,12,14,10,2,4,8,6,6,4,2,4,6,8,4,2,6,10,2,10,8,4,14,10,12,2,6,
  4,2,16,14,4,6,8,6,4,18,8,10,6,6,8,10,12,14,4,6,6,2,28,2,10,8,4,14,4,
  8,12,6,12,4,6,20,10,2,16,26,4,2,12,6,4,12,6,8,4,8,22,2,4,2,12,28,2,6,
  6,6,4,6,2,12,4,12,2,10,2,16,2,16,6,20,16,8,4,2,4,2,22,8,12,6,10,2,4,
  6,2,6,10,2,12,10,2,10,14,6,4,6,8,6,6,16,12,2,4,14,6,4,8,10,8,6,6,22,
  6,2,10,14,4,6,18,2,10,14,4,2,10,14,4,8,18,4,6,2,4,6,2,12,4,20,22,12,2,
  4,6,6,2,6,22,2,6,16,6,12,2,6,12,16,2,4,6,14,4,2,18,24,10,6,2,10,2,10,
  2,10,6,2,10,2,10,6,8,30,10,2,10,8,6,10,18,6,12,12,2,18,6,4,6,6,18,2,10,
  14,6,4,2,4,24,2,12,6,16,8,6,6,18,16,2,4,6,2,6,6,10,6,12,12,18,2,6,4,
  18,8,24,4,2,4,6,2,12,4,14,30,10,6,12,14,6,10,12,2,4,6,8,6,10,2,4,14,6,
  6,4,6,2,10,2,16,12,8,18,4,6,12,2,6,6,6,28,6,14,4,8,10,8,12,18,4,2,4,
  24,12,6,2,16,6,6,14,10,14,4,30,6,6,6,8,6,4,2,12,6,4,2,6,22,6,2,4,18,
  2,4,12,2,6,4,26,6,6,4,8,10,32,16,2,6,4,2,4,2,10,14,6,4,8,10,6,20,4,
  2,6,30,4,8,10,6,6,8,6,12,4,6,2,6,4,6,2,10,2,16,6,20,4,12,14,28,6,20,
  4,18,8,6,4,6,14,6,6,10,2,10,12,8,10,2,10,8,12,10,24,2,4,8,6,4,8,18,10,
  6,6,2,6,10,12,2,10,6,6,6,8,6,10,6,2,6,6,6,10,8,24,6,22,2,18,4,8,10,
  30,8,18,4,2,10,6,2,6,4,18,8,12,18,16,6,2,12,6,10,2,10,2,6,10,14,4,24,2,
  16,2,10,2,10,20,4,2,4,8,16,6,6,2,12,16,8,4,6,30,2,10,2,6,4,6,6,8,6,
  4,12,6,8,12,4,14,12,10,24,6,12,6,2,22,8,18,10,6,14,4,2,6,10,8,6,4,6,30,
  14,10,2,12,10,2,16,2,18,24,18,6,16,18,6,2,18,4,6,2,10,8,10,6,6,8,4,6,2,
  10,2,12,4,6,6,2,12,4,14,18,4,6,20,4,8,6,4,8,4,14,6,4,14,12,4,2,30,4,
  24,6,6,12,12,14,6,4,2,4,18,6,12,
  };

  static constexpr int NEXT_PRIME = 137;

  struct Factors {
    int num;
    uint64_t *b;
    uint8_t *e;
  };

  // Insert a factor that is known to not exist in the list. Note that
  // we use some statically-sized buffers with known limits on the
  // number of factors (that fit in 64 bits), so this can have undefined
  // behavior if it's not actually a prime factor, or is a duplicate.
  static void InsertNewFactor(Factors *factors, uint64_t b, uint64_t e) {
    factors->b[factors->num] = b;
    factors->e[factors->num] = e;
    factors->num++;
  }

  // Add the factor by finding it if it already exists (and incrementing
  // its exponent), or adding it (with an exponent of 1). See note above.
  static void PushFactor(Factors *factors,
                         uint64_t b) {
    // PERF: We could stop once we get to the prefix we know has been
    // trial factored. One clean way to do this would be to skip over
    // this region when we call the second phase.
    for (int i = factors->num - 1; i >= 0; i--) {
      if (factors->b[i] == b) {
        factors->e[i]++;
        return;
      }
    }
    InsertNewFactor(factors, b, 1);
  }

  static void NormalizeFactors(
      std::vector<std::pair<uint64_t, int>> *factors) {
    if (factors->empty()) return;
    // Sort by base.
    std::sort(factors->begin(), factors->end(),
              [](const auto &a, const auto &b) {
                return a.first < b.first;
              });

    // Now dedupe by summing.
    std::vector<std::pair<uint64_t, int>> out;
    out.reserve(factors->size());
    for (const auto &[p, e] : *factors) {
      if (!out.empty() &&
          out.back().first == p) {
        out.back().second += e;
      } else {
        out.emplace_back(p, e);
      }
    }
    *factors = std::move(out);
  }

  static void FactorizePredividedInternal(uint64_t cur, Factors *factors) {
    if (IsPrimeInternal(cur)) {
      InsertNewFactor(factors, cur, 1);
    } else {
      FactorUsingPollardRho(cur, 1, factors);
    }
  }

  static int FactorizePredivided(
      uint64_t x,
      uint64_t *bases,
      uint8_t *exponents) {

    // It would not be hard to incorporate Fermat's method too,
    // for cases that the number has factors close to its square
    // root too (this may be common?).

    Factors factors{.num = 0, .b = bases, .e = exponents};
    FactorizePredividedInternal(x, &factors);
    return factors.num;
  }

  // Assumptions from ported code.
  static_assert(sizeof(uintmax_t) == sizeof(uint64_t));
  static constexpr int W_TYPE_SIZE = 8 * sizeof (uint64_t);
  static_assert(W_TYPE_SIZE == 64);

  // XXX macros to templates etc.

  // Subtracts 128-bit words.
  // returns high, low
  static inline std::pair<uint64_t, uint64_t>
  Sub128(uint64_t ah, uint64_t al,
         uint64_t bh, uint64_t bl) {
    uint64_t carry = al < bl;
    return std::make_pair(ah - bh - carry, al - bl);
  }

  // Right-shifts a 128-bit quantity by count.
  // returns high, low
  static inline std::pair<uint64_t, uint64_t>
  RightShift128(uint64_t ah, uint64_t al, int count) {
    return std::make_pair(ah >> count,
                          (ah << (64 - count)) | (al >> count));
  }

  static inline bool GreaterEq128(uint64_t ah, uint64_t al,
                                  uint64_t bh, uint64_t bl) {
    return ah > bh || (ah == bh && al >= bl);
  }


  // Returns q, r.
  // Note we only ever use the second component, so maybe we should
  // just get rid of the quotient?
  static inline std::pair<uint64_t, uint64_t> UDiv128(uint64_t n1,
                                                      uint64_t n0,
                                                      uint64_t d) {
    assert (n1 < d);
    uint64_t d1 = d;
    uint64_t d0 = 0;
    uint64_t r1 = n1;
    uint64_t r0 = n0;
    uint64_t q = 0;
    for (unsigned int i = 64; i > 0; i--) {
      std::tie(d1, d0) = RightShift128(d1, d0, 1);
      q <<= 1;
      if (GreaterEq128(r1, r0, d1, d0)) {
        q++;
        std::tie(r1, r0) = Sub128(r1, r0, d1, d0);
      }
    }
    return std::make_pair(q, r0);
  }


  /* x B (mod n).  */
  static inline uint64_t Redcify(uint64_t r, uint64_t n) {
    return UDiv128(r, 0, n).second;
  }

  /* Requires that a < n and b <= n */
  static inline uint64_t SubMod(uint64_t a, uint64_t b, uint64_t n) {
    uint64_t t = - (uint64_t) (a < b);
    return (n & t) + a - b;
  }

  static inline uint64_t AddMod(uint64_t a, uint64_t b, uint64_t n) {
    return SubMod(a, n - b, n);
  }

  #define ll_B ((uint64_t) 1 << (64 / 2))
  #define ll_lowpart(t)  ((uint64_t) (t) & (ll_B - 1))
  #define ll_highpart(t) ((uint64_t) (t) >> (64 / 2))

  // returns w1, w0
  static inline std::pair<uint64_t, uint64_t> UMul128(uint64_t u, uint64_t v) {
    uint32_t ul = ll_lowpart(u);
    uint32_t uh = ll_highpart(u);
    uint32_t vl = ll_lowpart(v);
    uint32_t vh = ll_highpart(v);

    uint64_t x0 = (uint64_t) ul * vl;
    uint64_t x1 = (uint64_t) ul * vh;
    uint64_t x2 = (uint64_t) uh * vl;
    uint64_t x3 = (uint64_t) uh * vh;

    // This can't give carry.
    x1 += ll_highpart(x0);
    // But this indeed can.
    x1 += x2;
    // If so, add in the proper position.
    if (x1 < x2)
      x3 += ll_B;

    return std::make_pair(x3 + ll_highpart(x1),
                          (x1 << 64 / 2) + ll_lowpart(x0));
  }


  /* Entry i contains (2i+1)^(-1) mod 2^8.  */
  static constexpr unsigned char binvert_table[128] = {
    0x01, 0xAB, 0xCD, 0xB7, 0x39, 0xA3, 0xC5, 0xEF,
    0xF1, 0x1B, 0x3D, 0xA7, 0x29, 0x13, 0x35, 0xDF,
    0xE1, 0x8B, 0xAD, 0x97, 0x19, 0x83, 0xA5, 0xCF,
    0xD1, 0xFB, 0x1D, 0x87, 0x09, 0xF3, 0x15, 0xBF,
    0xC1, 0x6B, 0x8D, 0x77, 0xF9, 0x63, 0x85, 0xAF,
    0xB1, 0xDB, 0xFD, 0x67, 0xE9, 0xD3, 0xF5, 0x9F,
    0xA1, 0x4B, 0x6D, 0x57, 0xD9, 0x43, 0x65, 0x8F,
    0x91, 0xBB, 0xDD, 0x47, 0xC9, 0xB3, 0xD5, 0x7F,
    0x81, 0x2B, 0x4D, 0x37, 0xB9, 0x23, 0x45, 0x6F,
    0x71, 0x9B, 0xBD, 0x27, 0xA9, 0x93, 0xB5, 0x5F,
    0x61, 0x0B, 0x2D, 0x17, 0x99, 0x03, 0x25, 0x4F,
    0x51, 0x7B, 0x9D, 0x07, 0x89, 0x73, 0x95, 0x3F,
    0x41, 0xEB, 0x0D, 0xF7, 0x79, 0xE3, 0x05, 0x2F,
    0x31, 0x5B, 0x7D, 0xE7, 0x69, 0x53, 0x75, 0x1F,
    0x21, 0xCB, 0xED, 0xD7, 0x59, 0xC3, 0xE5, 0x0F,
    0x11, 0x3B, 0x5D, 0xC7, 0x49, 0x33, 0x55, 0xFF,
  };

  static inline uint64_t Binv(uint64_t n) {
    uint64_t inv = binvert_table[(n / 2) & 0x7F]; /*  8 */
    inv = 2 * inv - inv * inv * n;
    inv = 2 * inv - inv * inv * n;
    inv = 2 * inv - inv * inv * n;
    return inv;
  }

  /* Modular two-word multiplication, r = a * b mod m, with mi = m^(-1) mod B.
     Both a and b must be in redc form, the result will be in redc form too.

     (Redc is "montgomery form". mi stands for modular inverse.
      See https://en.wikipedia.org/wiki/Montgomery_modular_multiplication )
  */
  static inline uint64_t
  MulRedc(uint64_t a, uint64_t b, uint64_t m, uint64_t mi) {
    const auto &[rh, rl] = UMul128(a, b);
    uint64_t q = rl * mi;
    uint64_t th = UMul128(q, m).first;
    uint64_t xh = rh - th;
    if (rh < th)
      xh += m;

    return xh;
  }

  static uint64_t
  PowM(uint64_t b, uint64_t e, uint64_t n, uint64_t ni, uint64_t one) {
    uint64_t y = one;

    if (e & 1)
      y = b;

    while (e != 0) {
      b = MulRedc(b, b, n, ni);
      e >>= 1;

      if (e & 1)
        y = MulRedc(y, b, n, ni);
    }

    return y;
  }

  static inline uint64_t HighBitToMask(uint64_t x) {
    static_assert (((int64_t)-1 >> 1) < 0,
                   "In c++20 and later, right shift on signed "
                   "is an arithmetic shift.");
    return (uint64_t)((int64_t)(x) >> (W_TYPE_SIZE - 1));
  }

  static uint64_t
  GCDOdd(uint64_t a, uint64_t b) {
    if ((b & 1) == 0) {
      uint64_t t = b;
      b = a;
      a = t;
    }
    if (a == 0)
      return b;

    /* Take out least significant one bit, to make room for sign */
    b >>= 1;

    for (;;) {
      while ((a & 1) == 0)
        a >>= 1;
      a >>= 1;

      uint64_t t = a - b;
      if (t == 0)
        return (a << 1) + 1;

      uint64_t bgta = HighBitToMask(t);

      /* b <-- min (a, b) */
      b += (bgta & t);

      /* a <-- |a - b| */
      a = (t ^ bgta) - bgta;
    }
  }

  // One Miller-Rabin test. Returns true if definitely composite;
  // false if maybe prime.
  static bool DefinitelyComposite(
      uint64_t n, uint64_t ni, uint64_t b, uint64_t q,
      unsigned int k, uint64_t one) {
    uint64_t y = PowM(b, q, n, ni, one);

    /* -1, but in redc representation. */
    uint64_t nm1 = n - one;

    if (y == one || y == nm1)
      return false;

    for (unsigned int i = 1; i < k; i++) {
      // y = y^2 mod n
      y = MulRedc(y, y, n, ni);

      if (y == nm1)
        return false;
      if (y == one)
        return true;
    }
    return true;
  }

  // Deterministic primality test. Requires that n have no factors
  // less than NEXT_PRIME.
  static bool IsPrimeInternal(uint64_t n) {
    if (n <= 1)
      return false;

    /* We have already sieved out small primes.
       This also means that we don't need to check a = n as
       we consider the bases below. */
    if (n < (uint64_t)(NEXT_PRIME * NEXT_PRIME))
      return true;

    // Precomputation.
    uint64_t q = n - 1;

    // Count and remove trailing zero bits.
    const int k = std::countr_zero(q);
    q >>= k;

    // Compute modular inverse of n.
    const uint64_t ni = Binv(n);
    // Represention of 1 in redc form.
    const uint64_t one = Redcify(1, n);
    // Representation of 2 in redc form.
    uint64_t a_prim = AddMod(one, one, n);
    int a = 2;

    // Just need to check the first 12 prime bases for 64-bit ints.
    for (int i = 0; i < 12; i++) {
      if (DefinitelyComposite(n, ni, a_prim, q, k, one))
        return false;

      uint8_t delta = PRIME_DELTAS[i];

      // Establish new base.
      a += delta;

      /* The following is equivalent to a_prim = redcify (a, n).  It runs faster
         on most processors, since it avoids udiv128. */
      {
        const auto &[s1, s0] = UMul128(one, a);
        if (s1 == 0) [[likely]] {
          a_prim = s0 % n;
        } else {
          a_prim = UDiv128(s1, s0, n).second;
        }
      }
    }

    // The test above detects all 64-bit composite numbers, so this
    // must be a prime.
    return true;
  }

  static void
  FactorUsingPollardRho(uint64_t n, unsigned long int a,
                        Factors *factors) {
    // printf("pr(%llu, %lu)\n", n, a);
    uint64_t g;

    unsigned long int k = 1;
    unsigned long int l = 1;

    uint64_t P = Redcify(1, n);
    // i.e., Redcify(2)
    uint64_t x = AddMod(P, P, n);
    uint64_t z = x;
    uint64_t y = x;

    while (n != 1) {
      assert (a < n);

      uint64_t ni = Binv(n);

      for (;;) {
        do {
          x = MulRedc(x, x, n, ni);
          x = AddMod(x, a, n);

          uint64_t t = SubMod(z, x, n);
          P = MulRedc(P, t, n, ni);

          if (k % 32 == 1) {
            if (GCDOdd(P, n) != 1)
              goto factor_found;
            y = x;
          }
        } while (--k != 0);

        z = x;
        k = l;
        l = 2 * l;
        for (unsigned long int i = 0; i < k; i++) {
          x = MulRedc(x, x, n, ni);
          x = AddMod(x, a, n);
        }
        y = x;
      }

    factor_found:
      do {
        y = MulRedc(y, y, n, ni);
        y = AddMod(y, a, n);

        uint64_t t = SubMod(z, y, n);
        g = GCDOdd(t, n);
      } while (g == 1);

      if (n == g) {
        /* Found n itself as factor.  Restart with different params. */
        FactorUsingPollardRho(n, a + 1, factors);
        return;
      }

      n = n / g;

      if (IsPrimeInternal(g))
        PushFactor(factors, g);
      else
        FactorUsingPollardRho(g, a + 1, factors);

      if (IsPrimeInternal(n)) {
        PushFactor(factors, n);
        break;
      }

      x = x % n;
      z = z % n;
      y = y % n;
    }
  }

public:
  static int FactorizePreallocated(
      uint64_t x,
      uint64_t *bases,
      uint8_t *exponents) {

    // It would not be hard to incorporate Fermat's method too,
    // for cases that the number has factors close to its square
    // root too (this may be common?).

    if (x <= 1) return {};

    Factors factors{.num = 0, .b = bases, .e = exponents};

    uint64_t cur = x;

    const int twos = std::countr_zero<uint64_t>(x);
    if (twos) {
      InsertNewFactor(&factors, 2, twos);
      cur >>= twos;
    }

    // After 2, try the first 32 primes. This code used to have a much
    // longer list, but it's counterproductive. With hard-coded
    // constants, we can also take advantage of division-free compiler
    // tricks.
    //
    // PERF: Probably can pipeline more if we don't actually divide until
    // the end of all the factors?
    // #define TRY(p) while (cur % p == 0) { cur /= p; PushFactor(&factors, p); }
  #define TRY(p) do {                             \
      int e = 0;                                  \
      while (cur % p == 0) { cur /= p; e++; }     \
      if (e) InsertNewFactor(&factors, p, e);     \
    } while (0)
    // TRY(2);
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
  #undef TRY

    if (cur == 1) return factors.num;

    FactorizePredividedInternal(cur, &factors);
    return factors.num;
  }

  static bool IsPrime(uint64_t x) {
    if (x <= 1) return false;

    // Do the trial divisions so that IsPrimeInternal is correct,
    // which also quickly rejects a lot of composites.
    // PERF: Should perhaps re-tune this list with the deterministic
    // Miller-Rabin test.

  #define TRY(p) do { \
      if (x == p) return true; \
      if (x % p == 0) return false; \
  } while (0)
    TRY(2);
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
  #undef TRY

    return IsPrimeInternal(x);
  }

};
}  // namespace


// XXX could just use the deltas above everywhere; just need to
// be careful about the lengths
static constexpr array<uint16_t, 1000> PRIMES = {
  2,3,5,7,11,13,17,19,23,29,
  31,37,41,43,47,53,59,61,67,71,
  73,79,83,89,97,101,103,107,109,113,
  127,131,137,139,149,151,157,163,167,173,
  179,181,191,193,197,199,211,223,227,229,
  233,239,241,251,257,263,269,271,277,281,
  283,293,307,311,313,317,331,337,347,349,
  353,359,367,373,379,383,389,397,401,409,
  419,421,431,433,439,443,449,457,461,463,
  467,479,487,491,499,503,509,521,523,541,
  547,557,563,569,571,577,587,593,599,601,
  607,613,617,619,631,641,643,647,653,659,
  661,673,677,683,691,701,709,719,727,733,
  739,743,751,757,761,769,773,787,797,809,
  811,821,823,827,829,839,853,857,859,863,
  877,881,883,887,907,911,919,929,937,941,
  947,953,967,971,977,983,991,997,1009,1013,
  1019,1021,1031,1033,1039,1049,1051,1061,1063,1069,
  1087,1091,1093,1097,1103,1109,1117,1123,1129,1151,
  1153,1163,1171,1181,1187,1193,1201,1213,1217,1223,
  1229,1231,1237,1249,1259,1277,1279,1283,1289,1291,
  1297,1301,1303,1307,1319,1321,1327,1361,1367,1373,
  1381,1399,1409,1423,1427,1429,1433,1439,1447,1451,
  1453,1459,1471,1481,1483,1487,1489,1493,1499,1511,
  1523,1531,1543,1549,1553,1559,1567,1571,1579,1583,
  1597,1601,1607,1609,1613,1619,1621,1627,1637,1657,
  1663,1667,1669,1693,1697,1699,1709,1721,1723,1733,
  1741,1747,1753,1759,1777,1783,1787,1789,1801,1811,
  1823,1831,1847,1861,1867,1871,1873,1877,1879,1889,
  1901,1907,1913,1931,1933,1949,1951,1973,1979,1987,
  1993,1997,1999,2003,2011,2017,2027,2029,2039,2053,
  2063,2069,2081,2083,2087,2089,2099,2111,2113,2129,
  2131,2137,2141,2143,2153,2161,2179,2203,2207,2213,
  2221,2237,2239,2243,2251,2267,2269,2273,2281,2287,
  2293,2297,2309,2311,2333,2339,2341,2347,2351,2357,
  2371,2377,2381,2383,2389,2393,2399,2411,2417,2423,
  2437,2441,2447,2459,2467,2473,2477,2503,2521,2531,
  2539,2543,2549,2551,2557,2579,2591,2593,2609,2617,
  2621,2633,2647,2657,2659,2663,2671,2677,2683,2687,
  2689,2693,2699,2707,2711,2713,2719,2729,2731,2741,
  2749,2753,2767,2777,2789,2791,2797,2801,2803,2819,
  2833,2837,2843,2851,2857,2861,2879,2887,2897,2903,
  2909,2917,2927,2939,2953,2957,2963,2969,2971,2999,
  3001,3011,3019,3023,3037,3041,3049,3061,3067,3079,
  3083,3089,3109,3119,3121,3137,3163,3167,3169,3181,
  3187,3191,3203,3209,3217,3221,3229,3251,3253,3257,
  3259,3271,3299,3301,3307,3313,3319,3323,3329,3331,
  3343,3347,3359,3361,3371,3373,3389,3391,3407,3413,
  3433,3449,3457,3461,3463,3467,3469,3491,3499,3511,
  3517,3527,3529,3533,3539,3541,3547,3557,3559,3571,
  3581,3583,3593,3607,3613,3617,3623,3631,3637,3643,
  3659,3671,3673,3677,3691,3697,3701,3709,3719,3727,
  3733,3739,3761,3767,3769,3779,3793,3797,3803,3821,
  3823,3833,3847,3851,3853,3863,3877,3881,3889,3907,
  3911,3917,3919,3923,3929,3931,3943,3947,3967,3989,
  4001,4003,4007,4013,4019,4021,4027,4049,4051,4057,
  4073,4079,4091,4093,4099,4111,4127,4129,4133,4139,
  4153,4157,4159,4177,4201,4211,4217,4219,4229,4231,
  4241,4243,4253,4259,4261,4271,4273,4283,4289,4297,
  4327,4337,4339,4349,4357,4363,4373,4391,4397,4409,
  4421,4423,4441,4447,4451,4457,4463,4481,4483,4493,
  4507,4513,4517,4519,4523,4547,4549,4561,4567,4583,
  4591,4597,4603,4621,4637,4639,4643,4649,4651,4657,
  4663,4673,4679,4691,4703,4721,4723,4729,4733,4751,
  4759,4783,4787,4789,4793,4799,4801,4813,4817,4831,
  4861,4871,4877,4889,4903,4909,4919,4931,4933,4937,
  4943,4951,4957,4967,4969,4973,4987,4993,4999,5003,
  5009,5011,5021,5023,5039,5051,5059,5077,5081,5087,
  5099,5101,5107,5113,5119,5147,5153,5167,5171,5179,
  5189,5197,5209,5227,5231,5233,5237,5261,5273,5279,
  5281,5297,5303,5309,5323,5333,5347,5351,5381,5387,
  5393,5399,5407,5413,5417,5419,5431,5437,5441,5443,
  5449,5471,5477,5479,5483,5501,5503,5507,5519,5521,
  5527,5531,5557,5563,5569,5573,5581,5591,5623,5639,
  5641,5647,5651,5653,5657,5659,5669,5683,5689,5693,
  5701,5711,5717,5737,5741,5743,5749,5779,5783,5791,
  5801,5807,5813,5821,5827,5839,5843,5849,5851,5857,
  5861,5867,5869,5879,5881,5897,5903,5923,5927,5939,
  5953,5981,5987,6007,6011,6029,6037,6043,6047,6053,
  6067,6073,6079,6089,6091,6101,6113,6121,6131,6133,
  6143,6151,6163,6173,6197,6199,6203,6211,6217,6221,
  6229,6247,6257,6263,6269,6271,6277,6287,6299,6301,
  6311,6317,6323,6329,6337,6343,6353,6359,6361,6367,
  6373,6379,6389,6397,6421,6427,6449,6451,6469,6473,
  6481,6491,6521,6529,6547,6551,6553,6563,6569,6571,
  6577,6581,6599,6607,6619,6637,6653,6659,6661,6673,
  6679,6689,6691,6701,6703,6709,6719,6733,6737,6761,
  6763,6779,6781,6791,6793,6803,6823,6827,6829,6833,
  6841,6857,6863,6869,6871,6883,6899,6907,6911,6917,
  6947,6949,6959,6961,6967,6971,6977,6983,6991,6997,
  7001,7013,7019,7027,7039,7043,7057,7069,7079,7103,
  7109,7121,7127,7129,7151,7159,7177,7187,7193,7207,
  7211,7213,7219,7229,7237,7243,7247,7253,7283,7297,
  7307,7309,7321,7331,7333,7349,7351,7369,7393,7411,
  7417,7433,7451,7457,7459,7477,7481,7487,7489,7499,
  7507,7517,7523,7529,7537,7541,7547,7549,7559,7561,
  7573,7577,7583,7589,7591,7603,7607,7621,7639,7643,
  7649,7669,7673,7681,7687,7691,7699,7703,7717,7723,
  7727,7741,7753,7757,7759,7789,7793,7817,7823,7829,
  7841,7853,7867,7873,7877,7879,7883,7901,7907,7919,
};

[[maybe_unused]]
static constexpr int FIRST_OMITTED_PRIME = 7927;


#ifndef BIG_USE_GMP

// PERF: Could just use a better algorithm here. The
// Pollard-Rho implementation below would just need modular
// power (which we should offer in the interface, anyway).

std::vector<std::pair<BigInt, int>>
BigInt::PrimeFactorization(const BigInt &x, int64_t mf) {
  // Simple trial division.
  // It would not be hard to incorporate Fermat's method too,
  // for cases that the number has factors close to its square
  // root too (this may be common?).

  // Factors in increasing order.
  std::vector<std::pair<BigInt, int>> factors;

  BigInt cur = x;
  BigInt zero(0);
  BigInt two(2);

  // Illegal input.
  if (!BigInt::Greater(x, zero))
    return factors;

  BigInt max_factor(mf);
  // Without a max factor, use the starting number itself.
  if (mf < 0 || BigInt::Greater(max_factor, x))
    max_factor = x;

  // Add the factor, or increment its exponent if it is the
  // one already at the end. This requires that the factors
  // are added in ascending order (which they are).
  auto PushFactor = [&factors](const BigInt &b) {
      if (!factors.empty() &&
          BigInt::Eq(factors.back().first, b)) {
        factors.back().second++;
      } else {
        factors.push_back(make_pair(b, 1));
      }
    };

  // First, using the prime list.
  for (int i = 0; i < (int)PRIMES.size(); /* in loop */) {
    BigInt prime(PRIMES[i]);
    if (BigInt::Greater(prime, max_factor))
      break;

    const auto [q, r] = BigInt::QuotRem(cur, prime);
    if (BigInt::Eq(r, zero)) {
      cur = q;
      if (BigInt::Greater(max_factor, cur))
        max_factor = cur;
      PushFactor(prime);
      // But don't increment i, as it may appear as a
      // factor many times.
    } else {
      i++;
    }
  }

  // Once we exhausted the prime list, do the same
  // but with odd numbers up to the square root.
  BigInt divisor((int64_t)PRIMES.back());
  divisor = BigInt::Plus(divisor, two);
  for (;;) {
    if (mf >= 0 && BigInt::Greater(divisor, max_factor))
      break;

    // TODO: Would be faster to compute ceil(sqrt(cur)) each
    // time we have a new cur, right? We can then just have
    // the single max_factor as well.
    BigInt sq = BigInt::Times(divisor, divisor);
    if (BigInt::Greater(sq, cur))
      break;

    // TODO: Is it faster to skip ones with small factors?
    const auto [q, r] = BigInt::QuotRem(cur, divisor);
    if (BigInt::Eq(r, zero)) {
      cur = q;
      PushFactor(divisor);
      // But don't increment i, as it may appear as a
      // factor many times.
    } else {
      // At least we skip even ones.
      divisor = BigInt::Plus(divisor, two);
    }
  }

  // And the number itself, which we now know is prime
  // (unless we reached the max factor).
  if (!BigInt::Eq(cur, BigInt(1))) {
    PushFactor(cur);
  }

  return factors;
}

#endif

// Oops, there is BzSqrt! Benchmark to compare (and make sure
// it has the same rounding behavior.) Unless this is much
// faster, we should probably just use the library routine.
BigInt SqrtInternal(const BigInt &xx) {
  BigInt zero(0);
  BigInt one(1);
  BigInt two(2);
  BigInt four(4);

  BigInt cur = xx;
  std::vector<BigInt> stack;
  while (BigInt::Greater(cur, one)) {
    stack.push_back(cur);
    // PERF: with shifts
    cur = BigInt::Div(cur, four);
  }

  /*
  for (const BigInt &s : stack) {
    printf("Big Sqrt %s\n", s.ToString().c_str());
  }
  */

  BigInt ret = cur;
  while (!stack.empty()) {
    // PERF shift
    BigInt r2 = BigInt::Times(ret, two);
    BigInt r3 = BigInt::Plus(r2, one);
    /*
    printf("r2 = %s, r3 = %s\n",
           r2.ToString().c_str(),
           r3.ToString().c_str());
    */

    BigInt top = std::move(stack.back());
    stack.pop_back();

    if (BigInt::Less(top, BigInt::Times(r3, r3)))
      ret = std::move(r2);
    else
      ret = std::move(r3);
  }

  return ret;
}

BigInt BigInt::RandTo(const std::function<uint64_t()> &r,
                      const BigInt &radix) {
  if (BigInt::LessEq(radix, BigInt{1})) return BigInt{0};
  // Generate mask of all 1-bits.
  BigInt mask{1};
  int bits = 0;
  while (BigInt::Less(mask, radix)) {
    // PERF shift in place!
    mask = BigInt::Times(mask, BigInt{2});
    bits++;
  }
  mask = BigInt::Minus(mask, BigInt{1});

  int u64s = (bits + 63) / 64;

  for (;;) {
    // Generate a random bit string of the right length.
    BigInt s{0};
    for (int i = 0; i < u64s; i++) {
      uint64_t w = r();
      s = BigInt::Plus(BigInt::LeftShift(s, 64), BigInt::FromU64(w));
    }

    s = BigInt::BitwiseAnd(s, mask);
    if (BigInt::LessEq(s, radix)) return s;
  }
}

#ifdef BIG_USE_GMP

// assuming sorted factors array
void BigInt::InsertFactor(std::vector<std::pair<BigInt, int>> *factors,
                          mpz_t prime, unsigned int exponent) {
  // PERF binary search
  int i;
  for (i = factors->size() - 1; i >= 0; i--) {
    int cmp = mpz_cmp((*factors)[i].first.rep, prime);
    if (cmp == 0) {
      // Increase exponent of existing factor.
      (*factors)[i].second += exponent;
      return;
    }
    if (cmp < 0)
      break;
  }

  // PERF: btree or something?
  // Not found. Insert new factor.
  BigInt b;
  mpz_set(b.rep, prime);
  // Note that we have to first add 1 to i, because adding -1 to
  // the iterator would cause undefined behavior!
  factors->insert(factors->begin() + (i + 1),
                  std::make_pair(std::move(b), exponent));
}

void BigInt::InsertFactorUI(std::vector<std::pair<BigInt, int>> *factors,
                            unsigned long prime,
                            unsigned int exponent) {
  // PERF can avoid allocation when it's already present
  mpz_t pz;
  mpz_init_set_ui(pz, prime);
  InsertFactor(factors, pz, exponent);
  mpz_clear(pz);
}

void BigInt::FactorUsingDivision(mpz_t t,
                                 std::vector<std::pair<BigInt, int>> *factors) {
  unsigned long int e = mpz_scan1(t, 0);
  mpz_fdiv_q_2exp(t, t, e);
  if (e > 0) {
    InsertFactorUI(factors, 2, e);
  }

  for (int i = 1; i < (int)PRIMES.size(); /* in loop */) {
    unsigned long int p = PRIMES[i];
    if (!mpz_divisible_ui_p(t, p)) {
      i++;
      if (mpz_cmp_ui(t, p * p) < 0)
        break;
    } else {
      mpz_tdiv_q_ui(t, t, p);
      InsertFactorUI(factors, p);
    }
  }
}

static int mp_millerrabin(mpz_srcptr n, mpz_srcptr nm1, mpz_ptr x, mpz_ptr y,
                          mpz_srcptr q, unsigned long int k) {
  unsigned long int i;

  mpz_powm (y, x, q, n);

  if (mpz_cmp_ui (y, 1) == 0 || mpz_cmp (y, nm1) == 0)
    return 1;

  for (i = 1; i < k; i++) {
    mpz_powm_ui (y, y, 2, n);
    if (mpz_cmp (y, nm1) == 0)
      return 1;
    if (mpz_cmp_ui (y, 1) == 0)
      return 0;
  }
  return 0;
}

bool BigInt::MpzIsPrime(const mpz_t n) {
  int k;
  bool is_prime;
  mpz_t q, a, nm1, tmp;

  if (mpz_cmp_ui (n, 1) <= 0)
    return 0;

  /* We have already factored out small primes. */
  if (mpz_cmp_ui (n, (long) FIRST_OMITTED_PRIME * FIRST_OMITTED_PRIME) < 0)
    return 1;

  mpz_inits (q, a, nm1, tmp, NULL);

  /* Precomputation for Miller-Rabin.  */
  mpz_sub_ui (nm1, n, 1);

  /* Find q and k, where q is odd and n = 1 + 2**k * q.  */
  k = mpz_scan1 (nm1, 0);
  mpz_tdiv_q_2exp (q, nm1, k);

  mpz_set_ui (a, 2);

  /* Perform a Miller-Rabin test, which finds most composites quickly.  */
  if (!mp_millerrabin (n, nm1, a, tmp, q, k)) {
    mpz_clears (q, a, nm1, tmp, NULL);
    return false;
  }

  /* Factor n-1 for Lucas.  */
  mpz_set (tmp, nm1);

  std::vector<std::pair<BigInt, int>> factors =
    PrimeFactorizationInternal(tmp);

  /* Loop until Lucas proves our number prime, or Miller-Rabin proves our
     number composite.  */
  for (int r = 1; r < (int)PRIMES.size(); r++) {
    is_prime = true;
    for (const auto &[factor, exponent_] : factors) {
      mpz_divexact(tmp, nm1, factor.rep);
      mpz_powm(tmp, a, tmp, n);
      is_prime = mpz_cmp_ui(tmp, 1) != 0;
      if (!is_prime) break;
    }

    if (is_prime)
      goto ret1;

    mpz_set_ui (a, PRIMES[r]);

    if (!mp_millerrabin (n, nm1, a, tmp, q, k)) {
      is_prime = false;
      goto ret1;
    }
  }

  fprintf (stderr, "Lucas prime test failure.  This should not happen\n");
  abort ();

 ret1:
  mpz_clears (q, a, nm1, tmp, NULL);

  return is_prime;
}

void BigInt::FactorUsingPollardRho(
    mpz_t n, unsigned long a,
    std::vector<std::pair<BigInt, int>> *factors) {
  mpz_t x, z, y, P;
  mpz_t t, t2;
  unsigned long long k, l, i;

  mpz_inits (t, t2, NULL);
  mpz_init_set_si (y, 2);
  mpz_init_set_si (x, 2);
  mpz_init_set_si (z, 2);
  mpz_init_set_ui (P, 1);
  k = 1;
  l = 1;

  while (mpz_cmp_ui (n, 1) != 0) {
    for (;;) {
      do {
        mpz_mul (t, x, x);
        mpz_mod (x, t, n);
        mpz_add_ui (x, x, a);

        mpz_sub (t, z, x);
        mpz_mul (t2, P, t);
        mpz_mod (P, t2, n);

        if (k % 32 == 1) {
          mpz_gcd (t, P, n);
          if (mpz_cmp_ui (t, 1) != 0)
            goto factor_found;
          mpz_set (y, x);
        }
      }
      while (--k != 0);

      mpz_set (z, x);
      k = l;
      l = 2 * l;
      for (i = 0; i < k; i++) {
        mpz_mul (t, x, x);
        mpz_mod (x, t, n);
        mpz_add_ui (x, x, a);
      }
      mpz_set (y, x);
    }

  factor_found:
    do {
      mpz_mul (t, y, y);
      mpz_mod (y, t, n);
      mpz_add_ui (y, y, a);

      mpz_sub (t, z, y);
      mpz_gcd (t, t, n);
    } while (mpz_cmp_ui (t, 1) == 0);

    mpz_divexact (n, n, t); /* divide by t, before t is overwritten */

    // PERF: We could transition to the 64 bit code if it has become
    // small enough.
    if (!MpzIsPrime(t)) {
      FactorUsingPollardRho (t, a + 1, factors);
    } else {
      InsertFactor(factors, t);
    }

    if (MpzIsPrime(n)) {
      InsertFactor(factors, n);
      break;
    }

    mpz_mod (x, x, n);
    mpz_mod (z, z, n);
    mpz_mod (y, y, n);
  }

  mpz_clears (P, t2, t, z, x, y, nullptr);
}

static std::optional<uint64_t> GetU64(const mpz_t rep) {
  // Get the number of bits, ignoring sign.
  if (mpz_sizeinbase(rep, 2) > 64) {
    return std::nullopt;
  } else {
    // "buffer" where result is written
    uint64_t digit = 0;
    size_t count = 0;
    mpz_export(&digit, &count,
               // order doesn't matter, because there is just one word
               1,
               // 8 bytes
               8,
               // native endianness
               0,
               // 0 "nails" (leading bits to skip)
               0,
               rep);

    assert(count <= 1);

    if (mpz_sgn(rep) == -1) {
      return {-(int64_t)digit};
    }
    return {(int64_t)digit};
  }
}

std::vector<std::pair<BigInt, int>>
BigInt::PrimeFactorizationInternal(mpz_t x) {
  // Factors in increasing order.
  std::vector<std::pair<BigInt, int>> factors;

  if (mpz_sgn(x) == 1) [[likely]] {

    std::optional<uint64_t> u64o = GetU64(x);
    if (u64o.has_value()) {
      // If it fits in 64 bits, we can use machine words
      // and also a faster algorithm (deterministic Miller-Rabin).

      // At most 15 factors; 16 primorial is greater than 2^64.
      uint64_t b[15];
      uint8_t e[15];

      int num = Factor64::FactorizePreallocated(u64o.value(), b, e);
      for (int i = 0; i < num; i++) {
        // Note: Can't use InsertFactorUI here, as the factor may
        // be larger than unsigned int.
        BigInt p(b[i]);
        InsertFactor(&factors, p.rep, e[i]);
      }

    } else {
      // Full BigInt factorization.
      FactorUsingDivision(x, &factors);

      if (mpz_cmp_ui(x, 1) != 0) {
        if (MpzIsPrime(x))
          InsertFactor(&factors, x);
        else
          FactorUsingPollardRho(x, 1, &factors);
      }
    }
  } else {
    // 0 is allowed, but they should never be negative.
    assert(mpz_sgn(x) != -1);
  }

  return factors;
}

std::vector<std::pair<BigInt, int>>
BigInt::PrimeFactorization(const BigInt &x, int64_t max_factor_ignored) {
  mpz_t tmp;
  mpz_init(tmp);
  mpz_set(tmp, x.rep);
  auto ret = PrimeFactorizationInternal(tmp);
  mpz_clear(tmp);
  return ret;
}

bool BigInt::IsPrime(const BigInt &x) {
  std::optional<uint64_t> u64o = GetU64(x.rep);
  if (u64o.has_value()) {
    return Factor64::IsPrime(u64o.value());
  } else {
    return MpzIsPrime(x.rep);
  }
}

#endif

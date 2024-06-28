
#include "factorization.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
#include <utility>
#include <cmath>
#include <cassert>
#include <tuple>
#include <bit>

#include "base/logging.h"

using namespace std;

static uint64_t Sqrt64(uint64_t n) {
  if (n == 0) return 0;
  uint64_t r = std::sqrt((double)n);
  return r - (r * r - 1 >= n);
}

// Note that gaps between primes are <= 250 all the
// way up to 387096383. So we could have a lot more here. But
// I saw worse results from trial division when increasing this
// list beyond about 32 elements.
//
// XXX coreutils has 5000 primes, and might depend on that for the
// correctness of the Lucas primality test?
//
// XXX we only need 12 in the new version, which could just be
// inlined.
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

struct Factors {
  int num;
  uint64_t *b;
  uint8_t *e;
};

static void FactorUsingPollardRho(
    uint64_t n, unsigned long int a,
    Factors *factors);
static bool IsPrimeInternal(uint64_t n);

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

void Factorization::NormalizeFactors(
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

int Factorization::FactorizePreallocated(
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


int Factorization::FactorizePredivided(
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

uint64_t Factorization::NextPrime(uint64_t n) {
  CHECK(n < 18446744073709551557ULL) << "There are no larger representable primes.";

  if (n < 2) return 2;
  if (n == 2) return 3;

  // Presumably there are better algorithms than this!
  if (n & 1) {
    // Already odd.
    do {
      n += 2;
    } while (!IsPrime(n));

  } else {
    n++;
    while (!IsPrime(n)) {
      n += 1;
    }
  }

  return n;
}

bool Factorization::IsPrime(uint64_t x) {
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

/* This code derives from the GPL factor.c, which is part of GNU
   coreutils. Copyright (C) 1986-2023 Free Software Foundation, Inc.

   (This whole file is distributed under the terms of the GPL; see
   COPYING.)

   Contributors: Paul Rubin, Jim Meyering, James Youngman, Torbjörn
   Granlund, Niels Möller.
*/

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

// PERF maybe better to use int128 for intrinsics?
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

// PERF: Compare starting with (3*a)^2 as the initial guess
// and doing one more iteration. The advantage would be
// less cache pressure / memory latency.
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

// Computes the multiplicative inverse of n mod 2^64 (for odd n). This
// starts with an initial guess from the table above, then does enough
// iterations of Newton-Raphson to converge on the answer. Each
// iteration doubles the number of correct bits.
//
// PERF: Benchmark Dumas's algorithm. It's alleged that it gives
// better instruction-level parallelism because of shorter dependency
// chains. See https://arxiv.org/pdf/2204.04342.pdf
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

// PERF: Benchmark the IsPrimeInternalUnrolled version from
// factorize.cl. It wasn't faster on GPU, but I like the idea and it
// may very well work on CPU.

// Deterministic primality test. Requires that n have no factors
// less than NEXT_PRIME.
static bool IsPrimeInternal(uint64_t n) {
  if (n <= 1)
    return false;

  /* We have already sieved out small primes.
     This also means that we don't need to check a = n as
     we consider the bases below. */
  if (n < (uint64_t)(Factorization::NEXT_PRIME * Factorization::NEXT_PRIME))
    return true;

  // Precomputation.
  uint64_t q = n - 1;

  // Count and remove trailing zero bits.
  const int k = std::countr_zero(q);
  q >>= k;

  /*
  int k;
  for (k = 0; (q & 1) == 0; k++)
    q >>= 1;

  CHECK(k == kk && q == qq) <<
    StringPrintf("%llu: %d vs %d, %llu vs %llu\n",
                 n, k, kk, q, qq);
  int k = kk;
  q = qq;

  */

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

std::vector<std::pair<uint64_t, int>>
Factorization::Factorize(uint64_t x) {
  // At most 15 factors; 16 primorial is greater than 2^64.
  uint64_t b[15];
  uint8_t e[15];
  int num = FactorizePreallocated(x, b, e);
  std::vector<std::pair<uint64_t, int>> ret;
  ret.resize(num);
  for (int i = 0; i < num; i++) {
    ret[i] = make_pair(b[i], e[i]);
  }
  NormalizeFactors(&ret);
  return ret;
}

std::vector<std::pair<uint64_t, int>>
Factorization::ReferenceFactorize(uint64_t x) {
  uint64_t max_factor = x;

  if (x <= 1) return {};

  // Factors in increasing order.
  std::vector<std::pair<uint64_t, int>> factors;
  auto Push = [&factors](uint64_t n) {
      factors.emplace_back(n, 1);
    };

  uint64_t cur = x;

  const int twos = std::countr_zero<uint64_t>(x);
  if (twos) {
    factors.emplace_back(2, twos);
    cur >>= twos;
  }

  // After 2, try the first 32 primes. This code used to have a much
  // longer list, but it's counterproductive. With hard-coded
  // constants, we can also take advantage of division-free compiler
  // tricks.
  //
  // PERF: Probably can pipeline faster if we don't actually divide until
  // the end of all the factors?
#define TRY(p) while (cur % p == 0) { cur /= p; Push(p); }
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

  if (cur == 1) {
    NormalizeFactors(&factors);
    return factors;
  }

  // Once we exhausted the prime list, do the same
  // but with odd numbers up to the square root. Note
  // that the last prime is not actually tested above; it's
  // the initial value for this loop.
  uint64_t divisor = 137;
  uint64_t sqrtcur = Sqrt64(cur) + 1;
  for (;;) {
    // skip if a multiple of 3.
    if (divisor > max_factor)
      break;

    if (divisor > sqrtcur)
      break;

    const uint64_t rem = cur % divisor;
    if (rem == 0) {
      cur = cur / divisor;
      sqrtcur = Sqrt64(cur) + 1;
      Push(divisor);
      // But don't increment i, as it may appear as a
      // factor many times.
    } else {
      do {
        // Always skip even numbers; this is trivial.
        divisor += 2;
        // But also skip divisors with small factors. Modding by
        // small constants can be compiled into multiplication
        // tricks, so this is faster than doing another proper
        // loop.
      } while ((divisor % 3) == 0 ||
               (divisor % 5) == 0 ||
               (divisor % 7) == 0 ||
               (divisor % 11) == 0 ||
               (divisor % 13) == 0 ||
               (divisor % 17) == 0);
    }
  }

  // And the number itself, which we now know is prime
  // (unless we reached the max factor).
  if (cur != 1) {
    Push(cur);
  }

  NormalizeFactors(&factors);
  return factors;
}

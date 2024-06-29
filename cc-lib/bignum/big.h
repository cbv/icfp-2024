
// C++ wrappers for big integers and rationals by tom7 for cc-lib.

#ifndef _CC_LIB_BIGNUM_BIG_H
#define _CC_LIB_BIGNUM_BIG_H

#ifdef BIG_USE_GMP
# include <gmp.h>
#else
# include "bignum/bigz.h"
# include "bignum/bign.h"
# include "bignum/bigq.h"
#endif

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <cmath>

struct BigInt {
  static_assert(std::integral<size_t>);

  BigInt() : BigInt(uint32_t{0}) {}
  // From any integral type, but only up to 64 bits are supported.
  inline explicit BigInt(std::integral auto n);
  inline explicit BigInt(const std::string &digits);

  // Value semantics with linear-time copies (like std::vector).
  inline BigInt(const BigInt &other);
  inline BigInt &operator =(const BigInt &other);
  inline BigInt &operator =(BigInt &&other);

  inline ~BigInt();

  static inline BigInt FromU64(uint64_t u);

  // TODO: From doubles (rounding), which is useful because
  // uint64_t can't represent all large doubles.

  // Aborts if the string is not valid.
  // Bases from [2, 62] are permitted.
  inline std::string ToString(int base = 10) const;

  inline bool IsEven() const;
  inline bool IsOdd() const;

  // Returns -1, 0, or 1.
  inline static int Sign(const BigInt &a);

  inline static BigInt Negate(const BigInt &a);
  inline static BigInt Negate(BigInt &&a);
  inline static BigInt Abs(const BigInt &a);
  inline static int Compare(const BigInt &a, const BigInt &b);
  inline static bool Less(const BigInt &a, const BigInt &b);
  inline static bool Less(const BigInt &a, int64_t b);
  inline static bool LessEq(const BigInt &a, const BigInt &b);
  inline static bool LessEq(const BigInt &a, int64_t b);
  inline static bool Eq(const BigInt &a, const BigInt &b);
  inline static bool Eq(const BigInt &a, int64_t b);
  inline static bool Greater(const BigInt &a, const BigInt &b);
  inline static bool Greater(const BigInt &a, int64_t b);
  inline static bool GreaterEq(const BigInt &a, const BigInt &b);
  inline static bool GreaterEq(const BigInt &a, int64_t b);
  inline static BigInt Plus(const BigInt &a, const BigInt &b);
  inline static BigInt Plus(const BigInt &a, int64_t b);
  inline static BigInt Minus(const BigInt &a, const BigInt &b);
  inline static BigInt Minus(const BigInt &a, int64_t b);
  inline static BigInt Minus(int64_t a, const BigInt &b);
  inline static BigInt Times(const BigInt &a, const BigInt &b);
  inline static BigInt Times(const BigInt &a, int64_t b);

  // Truncates towards zero, like C.
  inline static BigInt Div(const BigInt &a, const BigInt &b);
  inline static BigInt Div(const BigInt &a, int64_t b);

  // Rounds towards negative infinity.
  inline static BigInt DivFloor(const BigInt &a, const BigInt &b);
  inline static BigInt DivFloor(const BigInt &a, int64_t b);

  // Equivalent to num % den == 0; maybe faster.
  inline static bool DivisibleBy(const BigInt &num, const BigInt &den);
  inline static bool DivisibleBy(const BigInt &num, int64_t den);
  // Returns a/b, but requires that that a % b == 0 for correctness.
  inline static BigInt DivExact(const BigInt &a, const BigInt &b);
  inline static BigInt DivExact(const BigInt &a, int64_t b);


  // TODO: Check that the behavior on negative numbers is the
  // same between the GMP and bignum implementations.

  // Ignores sign of b. Result is always in [0, |b|).
  // For the C % operator, use CMod.
  inline static BigInt Mod(const BigInt &a, const BigInt &b);
  // TODO: Could offer uint64_t Mod.

  // Modulus with C99/C++11 semantics: Division truncates towards
  // zero; modulus has the same sign as a.
  // cmod(a, b) = a - trunc(a / b) * b
  inline static BigInt CMod(const BigInt &a, const BigInt &b);
  inline static int64_t CMod(const BigInt &a, int64_t b);

  // Returns Q (a div b), R (a mod b) such that a = b * q + r
  // This is Div(a, b) and CMod(a, b); a / b and a % b in C.
  inline static std::pair<BigInt, BigInt> QuotRem(const BigInt &a,
                                                  const BigInt &b);

  inline static BigInt Pow(const BigInt &a, uint64_t exponent);

  // Integer square root, rounding towards zero.
  // Input must be non-negative.
  inline static BigInt Sqrt(const BigInt &a);
  // Returns a = floor(sqrt(aa)) and aa - a^2.
  inline static std::pair<BigInt, BigInt> SqrtRem(const BigInt &aa);
  inline static BigInt GCD(const BigInt &a, const BigInt &b);
  inline static BigInt LeftShift(const BigInt &a, uint64_t bits);
  inline static BigInt RightShift(const BigInt &a, uint64_t bits);
  inline static BigInt BitwiseAnd(const BigInt &a, const BigInt &b);
  inline static uint64_t BitwiseAnd(const BigInt &a, uint64_t b);

  inline static BigInt BitwiseXor(const BigInt &a, const BigInt &b);
  inline static BigInt BitwiseOr(const BigInt &a, const BigInt &b);
  // Return the number of trailing zeroes. For an input of zero,
  // this is zero (this differs from std::countr_zero<T>, which returns
  // the finite size of T in bits for zero).
  inline static uint64_t BitwiseCtz(const BigInt &a);

  // Only when in about -1e300 to 1e300; readily returns +/- inf
  // for large numbers.
  inline double ToDouble() const;

  // TODO: Implement with bigz too. There is a very straightforward
  // implementation.
  #ifdef BIG_USE_GMP
  // Returns (g, s, t) where g is GCD(a, b) and as + bt = g.
  // (the "Bezout coefficients".)
  inline static std::tuple<BigInt, BigInt, BigInt>
  ExtendedGCD(const BigInt &a, const BigInt &b);

  // Returns the approximate logarithm, base e.
  inline static double NaturalLog(const BigInt &a);
  inline static double LogBase2(const BigInt &a);

  // Compute the modular inverse of a mod b. Returns nullopt if
  // it does not exist.
  inline static std::optional<BigInt> ModInverse(
      const BigInt &a, const BigInt &b);

  #endif

  // Jacobi symbol (-1, 0, 1). b must be odd.
  inline static int Jacobi(const BigInt &a, const BigInt &b);

  // Generate a uniform random number in [0, radix).
  // r should return uniformly random uint64s.
  static BigInt RandTo(const std::function<uint64_t()> &r,
                       const BigInt &radix);

  inline std::optional<int64_t> ToInt() const;

  // Returns nullopt for negative numbers, or numbers larger
  // than 2^64-1.
  inline std::optional<uint64_t> ToU64() const;

  // Factors using trial division (slow!)
  // such that a0^b0 * a1^b1 * ... * an^bn = x,
  // where a0...an are primes in ascending order
  // and bi is >= 1
  //
  // If max_factor is not -1, then the final term may
  // be composite if its factors are all greater than this
  // number.
  //
  // Input must be positive.
  static std::vector<std::pair<BigInt, int>>
  PrimeFactorization(const BigInt &x, int64_t max_factor = -1);

  #if BIG_USE_GMP
  // Exact primality test.
  static bool IsPrime(const BigInt &x);
  #endif

  // Get 64 (or so) bits of the number. Will be equal for equal a, but
  // no particular value is guaranteed. Intended for hash functions.
  inline static uint64_t LowWord(const BigInt &a);

  inline void Swap(BigInt *other);


private:
  friend struct BigRat;
  #ifdef BIG_USE_GMP
  using Rep = mpz_t;
  void SetU64(uint64_t u) {
    // Need to be able to set 4 bytes at a time.
    static_assert(sizeof (unsigned long int) >= 4);
    const uint32_t hi = 0xFFFFFFFF & (u >> 32);
    const uint32_t lo = 0xFFFFFFFF & u;
    mpz_set_ui(rep, hi);
    mpz_mul_2exp(rep, rep, 32);
    mpz_add_ui(rep, rep, lo);
  }

  // XXX figure out how to hide this stuff away.
  // Could also move this to a big-util or whatever.
  static void InsertFactor(std::vector<std::pair<BigInt, int>> *, mpz_t,
                           unsigned int exponent = 1);
  static void InsertFactorUI(
      std::vector<std::pair<BigInt, int>> *, unsigned long,
      unsigned int exponent = 1);
  static void FactorUsingDivision(
      mpz_t, std::vector<std::pair<BigInt, int>> *);
  static std::vector<std::pair<BigInt, int>>
  PrimeFactorizationInternal(mpz_t x);
  static void FactorUsingPollardRho(
      mpz_t n, unsigned long a,
      std::vector<std::pair<BigInt, int>> *factors);
  static bool MpzIsPrime(const mpz_t n);

  #else
  // BigZ is a pointer to a bigz struct, which is the
  // header followed by digits.
  using Rep = BigZ;
  // Takes ownership.
  // nullptr token here is just used to distinguish from the version
  // that takes an int64 (would be ambiguous with BigInt(0)).
  explicit BigInt(Rep z, std::nullptr_t token) : rep(z) {}
  #endif

public:
  // Not recommended! And inherently not portable between
  // representations. But for example you can use this to efficiently
  // create BigInts from arrays of words using mpz_import.
  Rep &GetRep() { return rep; }
  const Rep &GetRep() const { return rep; }

private:
  Rep rep{};
};


struct BigRat {
  // Zero.
  inline BigRat() : BigRat(0LL, 1LL) {}
  inline BigRat(int64_t numer, int64_t denom);
  inline BigRat(int64_t numer);
  inline BigRat(const BigInt &numer, const BigInt &denom);
  inline BigRat(const BigInt &numer);

  inline BigRat(const BigRat &other);
  inline BigRat &operator =(const BigRat &other);
  inline BigRat &operator =(BigRat &&other);

  inline ~BigRat();

  // In base 10.
  inline std::string ToString() const;
  // The non-GMP version only works when the numerator and denominator
  // are small; readily returns nan! XXX fix it...
  inline double ToDouble() const;
  // Get the numerator and denominator.
  inline std::pair<BigInt, BigInt> Parts() const;

  inline static int Compare(const BigRat &a, const BigRat &b);
  inline static bool Eq(const BigRat &a, const BigRat &b);
  inline static BigRat Abs(const BigRat &a);
  inline static BigRat Div(const BigRat &a, const BigRat &b);
  inline static BigRat Inverse(const BigRat &a);
  inline static BigRat Times(const BigRat &a, const BigRat &b);
  inline static BigRat Negate(const BigRat &a);
  inline static BigRat Plus(const BigRat &a, const BigRat &b);
  inline static BigRat Minus(const BigRat &a, const BigRat &b);
  inline static BigRat Pow(const BigRat &a, uint64_t exponent);

  inline static BigRat ApproxDouble(double num, int64_t max_denom);

  inline void Swap(BigRat *other);

private:
  #ifdef BIG_USE_GMP
  using Rep = mpq_t;
  #else
  // TODO: This is a pointer to a struct with two BigZs (pointers),
  // so it would probably be much better to just unpack it here.
  // bigq.cc is seemingly set up to do this by redefining some
  // macros in the EXTERNAL_BIGQ_MEMORY section of the header.
  using Rep = BigQ;
  // Takes ownership.
  // Token for disambiguation as above.
  explicit BigRat(Rep q, std::nullptr_t token) : rep(q) {}
  #endif

  Rep rep{};
};

// Implementations follow. These are all light wrappers around
// the functions in the underlying representation, so inline makes
// sense.

inline BigInt BigInt::FromU64(uint64_t u) {
  uint32_t hi = (u >> 32) & 0xFFFFFFFF;
  uint32_t lo = u         & 0xFFFFFFFF;
  return BigInt::Plus(LeftShift(BigInt{hi}, 32), BigInt{lo});
}

#if BIG_USE_GMP

namespace internal {
inline bool FitsLongInt(int64_t x) {
  return (std::numeric_limits<long int>::min() <= x &&
          x <= std::numeric_limits<long int>::max());
}
}

BigInt::BigInt(std::integral auto ni) {
  // PERF: Set 32-bit quantities too.
  /*
  // Need to be able to set 4 bytes at a time.
  static_assert(sizeof (unsigned long int) >= 4);
  mpz_set_ui(rep, u);
  */

  using T = decltype(ni);
  if constexpr (std::signed_integral<T>) {
    static_assert(sizeof (T) <= sizeof (int64_t));
    const int64_t n = ni;
    mpz_init(rep);
    if (n < 0) {
      SetU64((uint64_t)-n);
      mpz_neg(rep, rep);
    } else {
      SetU64((uint64_t)n);
    }
  } else {
    static_assert(std::unsigned_integral<T>);
    static_assert(sizeof (T) <= sizeof (uint64_t));
    uint64_t u = ni;
    SetU64(u);
  }
}

BigInt::BigInt(const BigInt &other) {
  mpz_init(rep);
  mpz_set(rep, other.rep);
}

BigInt &BigInt::operator =(const BigInt &other) {
  // Self-assignment does nothing.
  if (this == &other) return *this;
  mpz_set(rep, other.rep);
  return *this;
}
BigInt &BigInt::operator =(BigInt &&other) {
  // We don't care how we leave other, but it needs to be valid (e.g. for
  // the destructor). Swap is a good way to do this.
  mpz_swap(rep, other.rep);
  return *this;
}

BigInt::BigInt(const std::string &digits) {
  mpz_init(rep);
  int res = mpz_set_str(rep, digits.c_str(), 10);
  if (0 != res) {
    printf("Invalid number [%s]\n", digits.c_str());
    assert(false);
  }
}

BigInt::~BigInt() {
  mpz_clear(rep);
}

void BigInt::Swap(BigInt *other) {
  mpz_swap(rep, other->rep);
}

uint64_t BigInt::LowWord(const BigInt &a) {
  // Zero is represented with no limbs.
  size_t limbs = mpz_size(a.rep);
  if (limbs == 0) return 0;
  // limb 0 is the least significant.
  // XXX if mp_limb_t is not 64 bits, we could get more
  // limbs here.
  return mpz_getlimbn(a.rep, 0);
}

std::string BigInt::ToString(int base) const {
  std::string s;
  // We allocate the space directly in the string to avoid
  // copying.
  // May need space for a minus sign. This function also writes
  // a nul terminating byte, but we don't want that for std::string.
  size_t min_size = mpz_sizeinbase(rep, base);
  s.resize(min_size + 2);
  mpz_get_str(s.data(), base, rep);

  // Now we have a nul-terminated string in the buffer, which is at
  // least one byte too large. We could just use strlen here but
  // we know it's at least min_size - 1 (because mpz_sizeinbase
  // can return a number 1 too large). min_size is always at least
  // 1, so starting at min_size - 1 is safe.
  for (size_t sz = min_size - 1; sz < s.size(); sz++) {
    if (s[sz] == 0) {
      s.resize(sz);
      return s;
    }
  }
  // This would mean that mpz_get_str didn't nul-terminate the string.
  assert(false);
  return s;
}

double BigInt::ToDouble() const {
  return mpz_get_d(rep);
}

int BigInt::Sign(const BigInt &a) {
  return mpz_sgn(a.rep);
}

std::optional<int64_t> BigInt::ToInt() const {
  // Get the number of bits, ignoring sign.
  if (mpz_sizeinbase(rep, 2) > 63) {
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
    assert(!(digit & 0x8000000000000000ULL));
    if (mpz_sgn(rep) == -1) {
      return {-(int64_t)digit};
    }
    return {(int64_t)digit};
  }
}

std::optional<uint64_t> BigInt::ToU64() const {
  // No negative numbers.
  if (mpz_sgn(rep) == -1)
    return std::nullopt;

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
    return {digit};
  }
}

double BigInt::NaturalLog(const BigInt &a) {
  // d is the magnitude, with absolute value in [0.5,1].
  //   a = di * 2^exponent
  // taking the log of both sides,
  //   log(a) = log(di) + log(2) * exponent
  signed long int exponent = 0;
  const double di = mpz_get_d_2exp(&exponent, a.rep);
  return std::log(di) + std::log(2.0) * (double)exponent;
}

double BigInt::LogBase2(const BigInt &a) {
  // d is the magnitude, with absolute value in [0.5,1].
  //   a = di * 2^exponent
  // taking the log of both sides,
  //   lg(a) = lg(di) + lg(2) * exponent
  //   lg(a) = log(di)/log(2) + 1 * exponent
  signed long int exponent = 0;
  const double di = mpz_get_d_2exp(&exponent, a.rep);
  return std::log(di)/std::log(2.0) + (double)exponent;
}

int BigInt::Jacobi(const BigInt &a, const BigInt &b) {
  return mpz_jacobi(a.rep, b.rep);
}

std::optional<BigInt> BigInt::ModInverse(
    const BigInt &a, const BigInt &b) {
  BigInt ret;
  if (mpz_invert(ret.rep, a.rep, b.rep)) {
    return {ret};
  } else {
    return std::nullopt;
  }
}


BigInt BigInt::BitwiseAnd(const BigInt &a, const BigInt &b) {
  BigInt ret;
  mpz_and(ret.rep, a.rep, b.rep);
  return ret;
}

uint64_t BigInt::BitwiseAnd(const BigInt &a, uint64_t b) {
  // Zero is represented without limbs.
  if (mpz_size(a.rep) == 0) return 0;
  static_assert(sizeof (mp_limb_t) == 8,
                "This code assumes 64-bit limbs, although we "
                "could easily add branches for 32-bit.");
  // Extract the low word and AND natively.
  uint64_t aa = mpz_getlimbn(a.rep, 0);
  return aa & b;
}

BigInt BigInt::BitwiseXor(const BigInt &a, const BigInt &b) {
  BigInt ret;
  mpz_xor(ret.rep, a.rep, b.rep);
  return ret;
}

BigInt BigInt::BitwiseOr(const BigInt &a, const BigInt &b) {
  BigInt ret;
  // "inclusive or"
  mpz_ior(ret.rep, a.rep, b.rep);
  return ret;
}

uint64_t BigInt::BitwiseCtz(const BigInt &a) {
  if (mpz_sgn(a.rep) == 0) return 0;
  mp_bitcnt_t zeroes = mpz_scan1(a.rep, 0);
  return zeroes;
}

bool BigInt::IsEven() const {
  return mpz_even_p(rep);
}
bool BigInt::IsOdd() const {
  return mpz_odd_p(rep);
}

BigInt BigInt::Negate(const BigInt &a) {
  BigInt ret;
  mpz_neg(ret.rep, a.rep);
  return ret;
}
BigInt BigInt::Negate(BigInt &&a) {
  mpz_neg(a.rep, a.rep);
  return a;
}

BigInt BigInt::Abs(const BigInt &a) {
  BigInt ret;
  mpz_abs(ret.rep, a.rep);
  return ret;
}
int BigInt::Compare(const BigInt &a, const BigInt &b) {
  int r = mpz_cmp(a.rep, b.rep);
  if (r < 0) return -1;
  else if (r > 0) return 1;
  else return 0;
}

bool BigInt::Less(const BigInt &a, const BigInt &b) {
  return mpz_cmp(a.rep, b.rep) < 0;
}
bool BigInt::Less(const BigInt &a, int64_t b) {
  if (internal::FitsLongInt(b)) {
    signed long int sb = b;
    return mpz_cmp_si(a.rep, sb) < 0;
  } else {
    return Less(a, BigInt(b));
  }
}

bool BigInt::LessEq(const BigInt &a, const BigInt &b) {
  return mpz_cmp(a.rep, b.rep) <= 0;
}
bool BigInt::LessEq(const BigInt &a, int64_t b) {
  if (internal::FitsLongInt(b)) {
    signed long int sb = b;
    return mpz_cmp_si(a.rep, sb) <= 0;
  } else {
    return LessEq(a, BigInt(b));
  }
}

bool BigInt::Eq(const BigInt &a, const BigInt &b) {
  return mpz_cmp(a.rep, b.rep) == 0;
}
bool BigInt::Eq(const BigInt &a, int64_t b) {
  if (internal::FitsLongInt(b)) {
    signed long int sb = b;
    return mpz_cmp_si(a.rep, sb) == 0;
  } else {
    return Eq(a, BigInt(b));
  }
}


bool BigInt::Greater(const BigInt &a, const BigInt &b) {
  return mpz_cmp(a.rep, b.rep) > 0;
}
bool BigInt::Greater(const BigInt &a, int64_t b) {
  if (internal::FitsLongInt(b)) {
    signed long int sb = b;
    return mpz_cmp_si(a.rep, sb) > 0;
  } else {
    return Greater(a, BigInt(b));
  }
}


bool BigInt::GreaterEq(const BigInt &a, const BigInt &b) {
  return mpz_cmp(a.rep, b.rep) >= 0;
}
bool BigInt::GreaterEq(const BigInt &a, int64_t b) {
  if (internal::FitsLongInt(b)) {
    signed long int sb = b;
    return mpz_cmp_si(a.rep, sb) >= 0;
  } else {
    return GreaterEq(a, BigInt(b));
  }
}

BigInt BigInt::Plus(const BigInt &a, const BigInt &b) {
  BigInt ret;
  mpz_add(ret.rep, a.rep, b.rep);
  return ret;
}

BigInt BigInt::Plus(const BigInt &a, int64_t b) {
  // PERF could also support negative b. but GMP only has
  // _ui version.
  if (b >= 0 && internal::FitsLongInt(b)) {
    signed long int sb = b;
    BigInt ret;
    mpz_add_ui(ret.rep, a.rep, sb);
    return ret;
  } else {
    return Plus(a, BigInt(b));
  }
}

BigInt BigInt::Minus(const BigInt &a, const BigInt &b) {
  BigInt ret;
  mpz_sub(ret.rep, a.rep, b.rep);
  return ret;
}

BigInt BigInt::Minus(const BigInt &a, int64_t b) {
  // PERF could also support negative b. but GMP only has
  // _ui version.
  if (b >= 0 && internal::FitsLongInt(b)) {
    signed long int sb = b;
    BigInt ret;
    mpz_sub_ui(ret.rep, a.rep, sb);
    return ret;
  } else {
    return Minus(a, BigInt(b));
  }
}

BigInt BigInt::Minus(int64_t a, const BigInt &b) {
  // PERF could also support negative b. but GMP only has
  // _ui version.
  if (a >= 0 && internal::FitsLongInt(a)) {
    signed long int sa = a;
    BigInt ret;
    mpz_ui_sub(ret.rep, sa, b.rep);
    return ret;
  } else {
    return Minus(BigInt(a), b);
  }
}

BigInt BigInt::Times(const BigInt &a, const BigInt &b) {
  BigInt ret;
  mpz_mul(ret.rep, a.rep, b.rep);
  return ret;
}

BigInt BigInt::Times(const BigInt &a, int64_t b) {
  if (internal::FitsLongInt(b)) {
    signed long int sb = b;
    BigInt ret;
    mpz_mul_si(ret.rep, a.rep, sb);
    return ret;
  } else {
    return Times(a, BigInt(b));
  }
}

BigInt BigInt::Div(const BigInt &a, const BigInt &b) {
  // truncate (round towards zero) like C
  BigInt ret;
  mpz_tdiv_q(ret.rep, a.rep, b.rep);
  return ret;
}

BigInt BigInt::Div(const BigInt &a, int64_t b) {
  if (internal::FitsLongInt(b)) {
    // alas there is no _si version, so branch on
    // the sign.
    if (b >= 0) {
      signed long int sb = b;
      BigInt ret;
      mpz_tdiv_q_ui(ret.rep, a.rep, sb);
      return ret;
    } else {
      signed long int sb = -b;
      BigInt ret;
      mpz_tdiv_q_ui(ret.rep, a.rep, sb);
      mpz_neg(ret.rep, ret.rep);
      return ret;
    }
  } else {
    return Div(a, BigInt(b));
  }
}

BigInt BigInt::DivFloor(const BigInt &a, const BigInt &b) {
  // truncate (round towards zero) like C
  BigInt ret;
  mpz_fdiv_q(ret.rep, a.rep, b.rep);
  return ret;
}

BigInt BigInt::DivFloor(const BigInt &a, int64_t b) {
  // PERF: There is mpz_fdiv_q_ui, but we can't just flip the
  // sign if it's negative, since rounding depends on the sign.
  // Maybe just handle the positive case here?
  return DivFloor(a, BigInt(b));
}


bool BigInt::DivisibleBy(const BigInt &num, const BigInt &den) {
  // (Note that GMP accepts 0 % 0, but I consider that an instance
  // of undefined behavior in this library.)
  return mpz_divisible_p(num.rep, den.rep);
}

bool BigInt::DivisibleBy(const BigInt &num, int64_t den) {
  if (internal::FitsLongInt(den)) {
    unsigned long int uden = std::abs(den);
    return mpz_divisible_ui_p(num.rep, uden);
  } else {
    return DivisibleBy(num, BigInt(den));
  }
}

BigInt BigInt::DivExact(const BigInt &a, const BigInt &b) {
  BigInt ret;
  mpz_divexact(ret.rep, a.rep, b.rep);
  return ret;
}

BigInt BigInt::DivExact(const BigInt &a, int64_t b) {
  if (internal::FitsLongInt(b)) {
    if (b >= 0) {
      BigInt ret;
      unsigned long int ub = b;
      mpz_divexact_ui(ret.rep, a.rep, ub);
      return ret;
    } else {
      unsigned long int ub = -b;
      BigInt ret;
      mpz_divexact_ui(ret.rep, a.rep, ub);
      mpz_neg(ret.rep, ret.rep);
      return ret;
    }
  } else {
    return DivExact(a, BigInt(b));
  }
}

BigInt BigInt::Mod(const BigInt &a, const BigInt &b) {
  BigInt ret;
  mpz_mod(ret.rep, a.rep, b.rep);
  return ret;
}

BigInt BigInt::CMod(const BigInt &a, const BigInt &b) {
  BigInt r;
  mpz_tdiv_r(r.rep, a.rep, b.rep);
  return r;
}

int64_t BigInt::CMod(const BigInt &a, int64_t b) {
  if (internal::FitsLongInt(b)) {
    if (b >= 0) {
      BigInt ret;
      unsigned long int ub = b;
      // PERF: Should be possible to do this without
      // allocating a rep? The return value is the
      // absolute value of the remainder.
      (void)mpz_tdiv_r_ui(ret.rep, a.rep, ub);
      auto ro = ret.ToInt();
      assert(ro.has_value());
      return ro.value();
    } else {
      // TODO: Can still use tdiv_r_ui.
    }
  }
  auto ro = CMod(a, BigInt(b)).ToInt();
  assert(ro.has_value());
  return ro.value();
}

// Returns Q (a div b), R (a mod b) such that a = b * q + r
std::pair<BigInt, BigInt> BigInt::QuotRem(const BigInt &a,
                                          const BigInt &b) {
  BigInt q, r;
  mpz_tdiv_qr(q.rep, r.rep, a.rep, b.rep);
  return std::make_pair(q, r);
}

BigInt BigInt::Pow(const BigInt &a, uint64_t exponent) {
  BigInt ret;
  mpz_pow_ui(ret.rep, a.rep, exponent);
  return ret;
}

BigInt BigInt::LeftShift(const BigInt &a, uint64_t shift) {
  if (internal::FitsLongInt(shift)) {
    mp_bitcnt_t sh = shift;
    BigInt ret;
    mpz_mul_2exp(ret.rep, a.rep, sh);
    return ret;
  } else {
    return Times(a, Pow(BigInt{2}, shift));
  }
}

BigInt BigInt::RightShift(const BigInt &a, uint64_t shift) {
  if (internal::FitsLongInt(shift)) {
    mp_bitcnt_t sh = shift;
    BigInt ret;
    mpz_fdiv_q_2exp(ret.rep, a.rep, sh);
    return ret;
  } else {
    return Div(a, Pow(BigInt{2}, shift));
  }
}


BigInt BigInt::GCD(const BigInt &a, const BigInt &b) {
  BigInt ret;
  mpz_gcd(ret.rep, a.rep, b.rep);
  return ret;
}

std::tuple<BigInt, BigInt, BigInt>
BigInt::ExtendedGCD(const BigInt &a, const BigInt &b) {
  BigInt g, s, t;
  mpz_gcdext(g.rep, s.rep, t.rep, a.rep, b.rep);
  return std::make_tuple(g, s, t);
}

BigInt BigInt::Sqrt(const BigInt &a) {
  BigInt ret;
  mpz_sqrt(ret.rep, a.rep);
  return ret;
}

std::pair<BigInt, BigInt> BigInt::SqrtRem(const BigInt &aa) {
  BigInt ret, rem;
  mpz_sqrtrem(ret.rep, rem.rep, aa.rep);
  return std::make_pair(ret, rem);
}

BigRat::BigRat(int64_t numer, int64_t denom) :
  BigRat(BigInt{numer}, BigInt{denom}) {}

BigRat::BigRat(int64_t numer) : BigRat(BigInt{numer}) {}

BigRat::BigRat(const BigInt &numer, const BigInt &denom) {
  mpq_init(rep);
  mpq_set_z(rep, numer.rep);

  Rep tmp;
  mpq_init(tmp);
  mpq_set_z(tmp, denom.rep);
  mpq_div(rep, rep, tmp);
  mpq_clear(tmp);
}

BigRat::BigRat(const BigInt &numer) {
  BigInt n{numer};
  mpq_init(rep);
  mpq_set_z(rep, numer.rep);
}

BigRat::BigRat(const BigRat &other) {
  mpq_init(rep);
  mpq_set(rep, other.rep);
}
BigRat &BigRat::operator =(const BigRat &other) {
  // Self-assignment does nothing.
  if (this == &other) return *this;
  mpq_set(rep, other.rep);
  return *this;
}
BigRat &BigRat::operator =(BigRat &&other) {
  Swap(&other);
  return *this;
}

void BigRat::Swap(BigRat *other) {
  mpq_swap(rep, other->rep);
}

BigRat::~BigRat() {
  mpq_clear(rep);
}

int BigRat::Compare(const BigRat &a, const BigRat &b) {
  const int r = mpq_cmp(a.rep, b.rep);
  if (r < 0) return -1;
  else if (r > 0) return 1;
  else return 0;
}

bool BigRat::Eq(const BigRat &a, const BigRat &b) {
  return mpq_cmp(a.rep, b.rep) == 0;
}

BigRat BigRat::Abs(const BigRat &a) {
  BigRat ret;
  mpq_abs(ret.rep, a.rep);
  return ret;
}
BigRat BigRat::Div(const BigRat &a, const BigRat &b) {
  BigRat ret;
  mpq_div(ret.rep, a.rep, b.rep);
  return ret;
}
BigRat BigRat::Inverse(const BigRat &a) {
  BigRat ret;
  mpq_inv(ret.rep, a.rep);
  return ret;
}
BigRat BigRat::Times(const BigRat &a, const BigRat &b) {
  BigRat ret;
  mpq_mul(ret.rep, a.rep, b.rep);
  return ret;
}
BigRat BigRat::Negate(const BigRat &a) {
  BigRat ret;
  mpq_neg(ret.rep, a.rep);
  return ret;
}
BigRat BigRat::Plus(const BigRat &a, const BigRat &b) {
  BigRat ret;
  mpq_add(ret.rep, a.rep, b.rep);
  return ret;
}
BigRat BigRat::Minus(const BigRat &a, const BigRat &b) {
  BigRat ret;
  mpq_sub(ret.rep, a.rep, b.rep);
  return ret;
}

std::string BigRat::ToString() const {
  const auto &[numer, denom] = Parts();
  std::string ns = numer.ToString();
  if (mpz_cmp_ui(denom.rep, 1)) {
    // for n/1
    return ns;
  } else {
    std::string ds = denom.ToString();
    return ns + "/" + ds;
  }
}

std::pair<BigInt, BigInt> BigRat::Parts() const {
  BigInt numer, denom;
  mpz_set(numer.rep, mpq_numref(rep));
  mpz_set(denom.rep, mpq_denref(rep));
  return std::make_pair(numer, denom);
}

BigRat BigRat::ApproxDouble(double num, int64_t max_denom) {
  // XXX implement max_denom somehow?
  BigRat ret;
  mpq_set_d(ret.rep, num);
  return ret;
}

double BigRat::ToDouble() const {
  return mpq_get_d(rep);
}

#else
// No GMP. Using portable big*.h.


BigInt::BigInt(std::integral auto ni) {
  // PERF: Set 32-bit quantities too.
  /*
  // Need to be able to set 4 bytes at a time.
  static_assert(sizeof (unsigned long int) >= 4);
  mpz_set_ui(rep, u);
  */

  using T = decltype(ni);
  if constexpr (std::signed_integral<T>) {
    static_assert(sizeof (T) <= sizeof (int64_t));
    const int64_t n = ni;
    rep = BzFromInteger(n);
  } else {
    static_assert(std::unsigned_integral<T>);
    static_assert(sizeof (T) <= sizeof (uint64_t));
    const uint64_t u = ni;
    rep = BzFromUnsignedInteger(u);
  }
}

BigInt::BigInt(const BigInt &other) : rep(BzCopy(other.rep)) { }
BigInt &BigInt::operator =(const BigInt &other) {
  // Self-assignment does nothing.
  if (this == &other) return *this;
  BzFree(rep);
  rep = BzCopy(other.rep);
  return *this;
}
BigInt &BigInt::operator =(BigInt &&other) {
  // We don't care how we leave other, but it needs to be valid (e.g. for
  // the destructor). Swap is a good way to do this.
  Swap(&other);
  return *this;
}

BigInt::~BigInt() {
  BzFree(rep);
  rep = nullptr;
}

void BigInt::Swap(BigInt *other) {
  std::swap(rep, other->rep);
}

BigInt::BigInt(const std::string &digits) {
  rep = BzFromStringLen(digits.c_str(), digits.size(), 10, BZ_UNTIL_END);
}

std::string BigInt::ToString(int base) const {
  // Allocates a buffer.
  // Third argument forces a + sign for positive; not used here.
  BzChar *buf = BzToString(rep, base, 0);
  std::string ret{buf};
  BzFreeString(buf);
  return ret;
}

int BigInt::Sign(const BigInt &a) {
  switch (BzGetSign(a.rep)) {
  case BZ_MINUS: return -1;
  default:
  case BZ_ZERO: return 0;
  case BZ_PLUS: return 1;
  }
}

std::optional<int64_t> BigInt::ToInt() const {
  if (BzNumDigits(rep) > (BigNumLength)1) {
    return std::nullopt;
  } else {
    uint64_t digit = BzGetDigit(rep, 0);
    // Would overflow int64. (This may be a bug in BzToInteger?)
    if (digit & 0x8000000000000000ULL)
      return std::nullopt;
    if (BzGetSign(rep) == BZ_MINUS) {
      return {-(int64_t)digit};
    }
    return {(int64_t)digit};
  }
}
std::optional<uint64_t> BigInt::ToU64() const {
  if (BzGetSign(rep) == BZ_MINUS ||
      BzNumDigits(rep) > (BigNumLength)1) {
    return std::nullopt;
  } else {
    const uint64_t digit = BzGetDigit(rep, 0);
    // Would overflow int64. (This may be a bug in BzToInteger?)
    return {(uint64_t)digit};
  }
}

double BigInt::ToDouble() const {
  std::optional<int64_t> io = ToInt();
  if (io.has_value()) return (double)io.value();
  // There is no doubt a more accurate and faster way to do this!

  const int size = BzGetSize(rep);

  double d = 0.0;
  // From big end to little
  for (int idx = size - 1; idx >= 0; idx--) {
    uint64_t udigit = BzGetDigit(rep, idx);
    double ddigit = udigit;
    // printf("d %.17g | u %llu | dd %.17g\n", d, udigit, ddigit);
    for (size_t e = 0; e < sizeof (BigNumDigit); e++)
      d *= 256.0;
    d += ddigit;
  }

  if (BzGetSign(rep) == BZ_MINUS) d = -d;
  return d;
}

uint64_t BigInt::LowWord(const BigInt &a) {
  if (BzNumDigits(a.rep) == 0) return uint64_t{0};
  else return BzGetDigit(a.rep, 0);
}

bool BigInt::IsEven() const { return BzIsEven(rep); }
bool BigInt::IsOdd() const { return BzIsOdd(rep); }

int BigInt::Jacobi(const BigInt &a_input,
                   const BigInt &n_input) {
  // FIXME: Buggy! Test segfaults.

  // Preconditions.
  assert(Greater(n_input, 0));
  assert(n_input.IsOdd());

  int t = 1;

  BigInt a = CMod(a_input, n_input);
  BigInt n = n_input;

  if (Less(a, 0)) a = Plus(a, n);

  while (!Eq(a, 0)) {

    while (a.IsOdd()) {
      a = RightShift(a, 1);
      const uint64_t r = BitwiseAnd(n, 7);
      if (r == 3 || r == 5) {
        t = -t;
      }
    }

    std::swap(n, a);
    if (BitwiseAnd(a, 3) == 3 &&
        BitwiseAnd(n, 3) == 3) {
      t = -t;
    }

    a = CMod(a, n);
  }

  if (Eq(n, 1)) {
    return t;
  } else {
    return 0;
  }
}


BigInt BigInt::Negate(const BigInt &a) {
  return BigInt{BzNegate(a.rep), nullptr};
}
BigInt BigInt::Negate(BigInt &&a) {
  // PERF any way to negate in place?
  return BigInt{BzNegate(a.rep), nullptr};
}

BigInt BigInt::Abs(const BigInt &a) {
  return BigInt{BzAbs(a.rep), nullptr};
}
int BigInt::Compare(const BigInt &a, const BigInt &b) {
  switch (BzCompare(a.rep, b.rep)) {
  case BZ_LT: return -1;
  case BZ_EQ: return 0;
  default:
  case BZ_GT: return 1;
  }
}
bool BigInt::Less(const BigInt &a, const BigInt &b) {
  return BzCompare(a.rep, b.rep) == BZ_LT;
}
bool BigInt::Less(const BigInt &a, int64_t b) {
  return BzCompare(a.rep, BigInt{b}.rep) == BZ_LT;
}

bool BigInt::LessEq(const BigInt &a, const BigInt &b) {
  auto cmp = BzCompare(a.rep, b.rep);
  return cmp == BZ_LT || cmp == BZ_EQ;
}
bool BigInt::LessEq(const BigInt &a, int64_t b) {
  auto cmp = BzCompare(a.rep, BigInt{b}.rep);
  return cmp == BZ_LT || cmp == BZ_EQ;
}

bool BigInt::Eq(const BigInt &a, const BigInt &b) {
  return BzCompare(a.rep, b.rep) == BZ_EQ;
}
bool BigInt::Eq(const BigInt &a, int64_t b) {
  return BzCompare(a.rep, BigInt{b}.rep) == BZ_EQ;
}

bool BigInt::Greater(const BigInt &a, const BigInt &b) {
  return BzCompare(a.rep, b.rep) == BZ_GT;
}
bool BigInt::Greater(const BigInt &a, int64_t b) {
  return BzCompare(a.rep, BigInt{b}.rep) == BZ_GT;
}

bool BigInt::GreaterEq(const BigInt &a, const BigInt &b) {
  auto cmp = BzCompare(a.rep, b.rep);
  return cmp == BZ_GT || cmp == BZ_EQ;
}
bool BigInt::GreaterEq(const BigInt &a, int64_t b) {
  auto cmp = BzCompare(a.rep, BigInt{b}.rep);
  return cmp == BZ_GT || cmp == BZ_EQ;
}

BigInt BigInt::Plus(const BigInt &a, const BigInt &b) {
  return BigInt{BzAdd(a.rep, b.rep), nullptr};
}
BigInt BigInt::Plus(const BigInt &a, int64_t b) {
  return BigInt{BzAdd(a.rep, BigInt{b}.rep), nullptr};
}
BigInt BigInt::Minus(const BigInt &a, const BigInt &b) {
  return BigInt{BzSubtract(a.rep, b.rep), nullptr};
}
BigInt BigInt::Minus(const BigInt &a, int64_t b) {
  return BigInt{BzSubtract(a.rep, BigInt{b}.rep), nullptr};
}
BigInt BigInt::Minus(int64_t a, const BigInt &b) {
  return BigInt{BzSubtract(BigInt{a}.rep, b.rep), nullptr};
}

BigInt BigInt::Times(const BigInt &a, const BigInt &b) {
  return BigInt{BzMultiply(a.rep, b.rep), nullptr};
}
BigInt BigInt::Times(const BigInt &a, int64_t b) {
  return BigInt{BzMultiply(a.rep, BigInt{b}.rep), nullptr};
}

BigInt BigInt::Div(const BigInt &a, const BigInt &b) {
  // In BzTruncate is truncating division, like C.
  return BigInt{BzTruncate(a.rep, b.rep), nullptr};
}
BigInt BigInt::Div(const BigInt &a, int64_t b) {
  return BigInt{BzTruncate(a.rep, BigInt{b}.rep), nullptr};
}

BigInt BigInt::DivFloor(const BigInt &a, const BigInt &b) {
  return BigInt{BzFloor(a.rep, b.rep), nullptr};
}
BigInt BigInt::DivFloor(const BigInt &a, int64_t b) {
  return BigInt{BzFloor(a.rep, BigInt{b}.rep), nullptr};
}


BigInt BigInt::DivExact(const BigInt &a, const BigInt &b) {
  // Not using the precondition here; same as division.
  return BigInt{BzTruncate(a.rep, b.rep), nullptr};
}
BigInt BigInt::DivExact(const BigInt &a, int64_t b) {
  // Not using the precondition here; same as division.
  return BigInt{BzTruncate(a.rep, BigInt{b}.rep), nullptr};
}

bool BigInt::DivisibleBy(const BigInt &num, const BigInt &den) {
  return BzCompare(CMod(num, den).rep, BigInt{0}.rep) == BZ_EQ;
}
bool BigInt::DivisibleBy(const BigInt &num, int64_t den) {
  return BzCompare(CMod(num, BigInt{den}).rep, BigInt{0}.rep) == BZ_EQ;
}

// TODO: Clarify mod vs rem?
BigInt BigInt::Mod(const BigInt &a, const BigInt &b) {
  return BigInt{BzMod(a.rep, b.rep), nullptr};
}

// Returns Q (a div b), R (a mod b) such that a = b * q + r
std::pair<BigInt, BigInt> BigInt::QuotRem(const BigInt &a,
                                          const BigInt &b) {
  BigZ r = BzRem(a.rep, b.rep);
  BigZ q = BzTruncate(a.rep, b.rep);
  return std::make_pair(BigInt{q, nullptr}, BigInt{r, nullptr});
}

BigInt BigInt::CMod(const BigInt &a, const BigInt &b) {
  return BigInt{BzRem(a.rep, b.rep), nullptr};
}

int64_t BigInt::CMod(const BigInt &a, int64_t b) {
  auto ro = BigInt{BzRem(a.rep, BigInt{b}.rep), nullptr}.ToInt();
  assert(ro.has_value());
  return ro.value();
}


BigInt BigInt::Pow(const BigInt &a, uint64_t exponent) {
  return BigInt{BzPow(a.rep, exponent), nullptr};
}

BigInt BigInt::LeftShift(const BigInt &a, uint64_t bits) {
  return Times(a, Pow(BigInt{2}, bits));
}

BigInt BigInt::RightShift(const BigInt &a, uint64_t bits) {
  // There is BzAsh which I assume is Arithmetic Shift?
  return BigInt{BzFloor(a.rep, Pow(BigInt{2}, bits).rep), nullptr};
}

BigInt BigInt::BitwiseAnd(const BigInt &a, const BigInt &b) {
  return BigInt{BzAnd(a.rep, b.rep), nullptr};
}

uint64_t BigInt::BitwiseAnd(const BigInt &a, uint64_t b) {
  // PERF: extract a as int (just need low word), then do native and.
  // return BigInt{BzAnd(a.rep, BigInt{b}.rep), nullptr};
  auto ao = BigInt{BzAnd(a.rep, BigInt{b}.rep), nullptr}.ToU64();
  // Result must fit in 64 bits, since we anded with a 64-bit number.
  assert(ao.has_value());
  return ao.value();
}

BigInt BigInt::BitwiseXor(const BigInt &a, const BigInt &b) {
  return BigInt{BzXor(a.rep, b.rep), nullptr};
}

BigInt BigInt::BitwiseOr(const BigInt &a, const BigInt &b) {
  return BigInt{BzOr(a.rep, b.rep), nullptr};
}


uint64_t BigInt::BitwiseCtz(const BigInt &a) {
  if (BzGetSign(a.rep) == BZ_ZERO) return 0;
  // PERF: This could be faster by testing a limb at a time first.
  uint64_t bit = 0;
  while (!BzTestBit(bit, a.rep)) bit++;
  return bit;
}

BigInt BigInt::GCD(const BigInt &a, const BigInt &b) {
  return BigInt{BzGcd(a.rep, b.rep), nullptr};
}

BigInt SqrtInternal(const BigInt &a);
BigInt BigInt::Sqrt(const BigInt &a) {
  // PERF: Bench bignum's native sqrt.
  return SqrtInternal(a);
}

std::pair<BigInt, BigInt> BigInt::SqrtRem(const BigInt &aa) {
  BigInt a = Sqrt(aa);
  BigInt aa1 = Times(a, a);
  return std::make_pair(a, Minus(aa, aa1));
}

BigRat::BigRat(int64_t numer, int64_t denom) {
  // PERF This could avoid creating intermediate BigZ with
  // a new function inside BigQ.
  BigInt n{numer}, d{denom};
  rep = BqCreate(n.rep, d.rep);
}
BigRat::BigRat(int64_t numer) : BigRat(numer, int64_t{1}) {}

BigRat::BigRat(const BigInt &numer, const BigInt &denom)
  : rep(BqCreate(numer.rep, denom.rep)) {}

BigRat::BigRat(const BigInt &numer) : BigRat(numer, BigInt(1)) {}

// PERF: Should have BqCopy so that we don't need to re-normalize.
BigRat::BigRat(const BigRat &other) :
  rep(BqCreate(
           BqGetNumerator(other.rep),
           BqGetDenominator(other.rep))) {
}
BigRat &BigRat::operator =(const BigRat &other) {
  // Self-assignment does nothing.
  if (this == &other) return *this;
  BqDelete(rep);
  rep = BqCreate(BqGetNumerator(other.rep),
                  BqGetDenominator(other.rep));
  return *this;
}
BigRat &BigRat::operator =(BigRat &&other) {
  Swap(&other);
  return *this;
}

void BigRat::Swap(BigRat *other) {
  std::swap(rep, other->rep);
}

BigRat::~BigRat() {
  BqDelete(rep);
  rep = nullptr;
}

int BigRat::Compare(const BigRat &a, const BigRat &b) {
  switch (BqCompare(a.rep, b.rep)) {
  case BQ_LT: return -1;
  case BQ_EQ: return 0;
  default:
  case BQ_GT: return 1;
  }
}

bool BigRat::Eq(const BigRat &a, const BigRat &b) {
  return BqCompare(a.rep, b.rep) == BQ_EQ;
}

BigRat BigRat::Abs(const BigRat &a) {
  return BigRat{BqAbs(a.rep), nullptr};
}
BigRat BigRat::Div(const BigRat &a, const BigRat &b) {
  return BigRat{BqDiv(a.rep, b.rep), nullptr};
}
BigRat BigRat::Inverse(const BigRat &a) {
  return BigRat{BqInverse(a.rep), nullptr};
}
BigRat BigRat::Times(const BigRat &a, const BigRat &b) {
  return BigRat{BqMultiply(a.rep, b.rep), nullptr};
}
BigRat BigRat::Negate(const BigRat &a) {
  return BigRat{BqNegate(a.rep), nullptr};
}
BigRat BigRat::Plus(const BigRat &a, const BigRat &b) {
  Rep res = BqAdd(a.rep, b.rep);
  return BigRat{res, nullptr};
}
BigRat BigRat::Minus(const BigRat &a, const BigRat &b) {
  return BigRat{BqSubtract(a.rep, b.rep), nullptr};
}

std::string BigRat::ToString() const {
  // No forced +
  BzChar *buf = BqToString(rep, 0);
  std::string ret{buf};
  BzFreeString(buf);
  return ret;
}

std::pair<BigInt, BigInt> BigRat::Parts() const {
  return std::make_pair(BigInt(BzCopy(BqGetNumerator(rep)), nullptr),
                        BigInt(BzCopy(BqGetDenominator(rep)), nullptr));
}

BigRat BigRat::ApproxDouble(double num, int64_t max_denom) {
  return BigRat{BqFromDouble(num, max_denom), nullptr};
}

double BigRat::ToDouble() const {
  // TODO: Should be able to make this work as long as the ratio
  // is representable. GMP does it.
  return BqToDouble(rep);
}

#endif

// Common / derived implementations.

BigRat BigRat::Pow(const BigRat &a, uint64_t exponent) {
  const auto &[numer, denom] = a.Parts();
  BigInt nn(BigInt::Pow(numer, exponent));
  BigInt dd(BigInt::Pow(denom, exponent));
  return BigRat(nn, dd);
}


#endif

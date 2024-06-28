
#include "bignum/big.h"
#ifndef BIG_USE_GMP
#include "bignum/bign.h"
#endif

#include <bit>
#include <cstdint>
#include <array>
#include <vector>
#include <utility>

#include "base/logging.h"
#include "base/stringprintf.h"
#include "timer.h"

using int64 = int64_t;
using namespace std;

static constexpr bool RUN_BENCHMARKS = false;

static void TestToString() {
  for (int i = -100000; i < 100000; i++) {
    BigInt bi(i);
    string s = bi.ToString();
    CHECK(!s.empty());
    CHECK(BigInt::Eq(BigInt(s), bi));
    // We do some resizing tricks in the GMP version, which
    // can inadvertently leave nul bytes.
    for (char c : s) CHECK(c != 0) << i;
  }
}

static void TestSign() {
  CHECK(BigInt::Sign(BigInt(-3)) == -1);
  CHECK(BigInt::Sign(BigInt(0)) == 0);
  CHECK(BigInt::Sign(BigInt("19823749283749817")) == 1);
  CHECK(BigInt::Sign(BigInt("-999999999999999999999")) == -1);
}

static void CopyAndAssign() {
  BigInt a{1234};
  BigInt b{5678};
  BigInt c = a;
  a = b;
  CHECK(BigInt::Eq(a, b));
  CHECK(!BigInt::Eq(a, c));
  {
    BigInt z{8888};
    a = z;
  }
  CHECK(a.ToInt() == 8888);

  string s = "190237849028374901872390876190872349817230948719023874190827349817239048712903847190283740918273490817230948798767676767676738347482712341";
  BigInt big(s);
  CHECK(big.ToString() == s);
  big = a;
  CHECK(big.ToInt() == 8888);
  CHECK(big.ToU64() == 8888u);
}

static void TestToU64() {

  {
    auto uo = BigInt(0).ToU64();
    CHECK(uo.has_value() && uo.value() == 0);
  }

  CHECK(!BigInt(-1).ToU64().has_value());

  {
    auto uo =
      BigInt::Minus(
          BigInt::LeftShift(BigInt(1), 64),
          1).ToU64();
    CHECK(uo.has_value() && uo.value() == (uint64_t)-1);
  }

}

static void LowWord() {
  BigInt a{1234};
  BigInt b{5678};
  // Technically this can be anything, but it would probably be a bug if
  // small integers don't get different values.
  CHECK(BigInt::LowWord(a) != BigInt::LowWord(b));

  string s = "190237849028374901872390876190872349817230948719023874190827349817239048712903847190283740918273490817230948798767676767676738347482712341";
  BigInt big(s);
  CHECK(BigInt::LowWord(a) != BigInt::LowWord(big));
}

static void TestToDouble() {
  BigInt a{1234};
  BigInt b{-5678};

  CHECK((BigInt(0)).ToDouble() == 0.0);
  CHECK(a.ToDouble() == 1234.0);
  CHECK(b.ToDouble() == -5678.0);

  {
    BigInt big("7000000000000000000000000000"
               "000000000000000000000000000000000");
    double d = big.ToDouble();
    CHECK(d > 0.0);
    CHECK(std::isfinite(d));
    CHECK(d > 7e59 && d < 7e61) << StringPrintf("%11g\n", d);
  }

  {
    BigInt big("-7000000000000000000000000000"
               "000000000000000000000000000000000");
    double d = big.ToDouble();
    CHECK(d < 0.0);
    CHECK(std::isfinite(d));
    CHECK(d < -7e59 && d > -7e61) << StringPrintf("%11g\n", d);
  }

  // TODO: Test infinite cases
}


static void TestMod() {
  CHECK(BigInt::Eq(BigInt::Mod(BigInt{3}, BigInt{5}), BigInt{3}));
  CHECK(BigInt::Eq(BigInt::Mod(BigInt{7}, BigInt{5}), BigInt{2}));
  CHECK(BigInt::Eq(BigInt::Mod(BigInt{10}, BigInt{5}), BigInt{0}));
  CHECK(BigInt::Eq(BigInt::Mod(BigInt{-1}, BigInt{5}), BigInt{4}));
}

static void TestEq() {
  BigInt a{1234};
  BigInt b{5678};

  CHECK(BigInt::Eq(BigInt::Times(a, b), 7006652));
}

static void TestPow() {
  BigRat q(11,15);

  BigRat qqq = BigRat::Times(q, BigRat::Times(q, q));
  BigRat qcubed = BigRat::Pow(q, 3);
  printf("%s vs %s\n", qqq.ToString().c_str(),
         qcubed.ToString().c_str());
  CHECK(BigRat::Eq(qqq, qcubed));
}

// TODO: Test/document behavior on negative inputs
static void TestQuotRem() {
  BigInt a(37);
  BigInt b(5);

  const auto [q, r] = BigInt::QuotRem(a, b);
  CHECK(BigRat::Eq(q, BigInt(7)));
  CHECK(BigRat::Eq(r, BigInt(2)));
}

static void TestPrimeFactors() {
  printf("Prime factors..\n");
  auto FTOS = [](const std::vector<std::pair<BigInt, int>> &fs) {
      string s;
      for (const auto &[b, i] : fs) {
        StringAppendF(&s, "%s^%d ", b.ToString().c_str(), i);
      }
      return s;
    };

  BigInt bi31337(31337);
  {
    std::vector<std::pair<BigInt, int>> factors =
      BigInt::PrimeFactorization(bi31337);

    CHECK(factors.size() == 1);
    CHECK(factors[0].second == 1);
    CHECK(BigInt::Eq(factors[0].first, bi31337));
  }

  {
    BigInt x(31337 * 71);
    std::vector<std::pair<BigInt, int>> factors =
      BigInt::PrimeFactorization(x);

    CHECK(factors.size() == 2) << FTOS(factors);
    CHECK(BigInt::Eq(factors[0].first, BigInt(71)));
    CHECK(factors[0].second == 1) << factors[0].second;
    CHECK(BigInt::Eq(factors[1].first, bi31337));
    CHECK(factors[1].second == 1) << factors[0].second;
  }

  {
    BigInt bi31337sq(31337 * 31337);
    std::vector<std::pair<BigInt, int>> factors =
      BigInt::PrimeFactorization(bi31337sq);

    CHECK(factors.size() == 1) << FTOS(factors);
    CHECK(BigInt::Eq(factors[0].first, bi31337));
    CHECK(factors[0].second == 2) << factors[0].second;
  }

  // This will never finish without a better algorithm
  // in the non-gmp case.
  #ifdef BIG_USE_GMP
  {
    // Largest 64-bit prime.
    BigInt p("18446744073709551557");
    std::vector<std::pair<BigInt, int>> factors =
      BigInt::PrimeFactorization(p);

    CHECK(BigInt::IsPrime(p));

    CHECK(factors.size() == 1) << FTOS(factors);
    CHECK(BigInt::Eq(factors[0].first, p)) << factors[0].first.ToString();
    CHECK(factors[0].second == 1) << factors[0].second;
  }

  {
    BigInt x("11682658198262752314377738154934272");
    std::vector<std::pair<BigInt, int>> factors =
      BigInt::PrimeFactorization(x);

    CHECK(factors.size() == 3) << FTOS(factors);
    CHECK(BigInt::Eq(factors[0].first, 2));
    CHECK(factors[0].second == 18);

    CHECK(BigInt::Eq(factors[1].first, 3));
    CHECK(factors[1].second == 1);

    CHECK(BigInt::Eq(factors[2].first,
                     BigInt("14855268094714803459647799371")));
    CHECK(factors[2].second == 1);
  }
#endif

  {
    BigInt x(1);
    // Must all be distinct and prime
    const array f = {
      2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 37, 41, 43, 47, 419,
      541, 547, 31337,
    };
    for (int factor : f) {
      x = BigInt::Times(x, BigInt(factor));
    }

    std::vector<std::pair<BigInt, int>> factors =
      BigInt::PrimeFactorization(x);

    CHECK(factors.size() == f.size());
    for (int i = 0; i < (int)f.size(); i++) {
      CHECK(factors[i].second == 1);
      #if BIG_USE_GMP
      CHECK(BigInt::IsPrime(factors[i].first));
      #endif
      CHECK(BigInt::Eq(factors[i].first, BigInt(f[i])));
    }
  }

  #ifndef BIG_USE_GMP
  // XXX max_factor is not supported in gmp factorer.
  {
    // 100-digit prime; trial factoring will not succeed!
    BigInt p1("207472224677348520782169522210760858748099647"
              "472111729275299258991219668475054965831008441"
              "6732550077");
    BigInt p2("31337");

    BigInt x = BigInt::Times(p1, p2);

    printf("100-digit:\n");
    // Importantly, we set a max factor (greater than p2, and
    // greater than the largest value in the primes list).
    std::vector<std::pair<BigInt, int>> factors =
      BigInt::PrimeFactorization(x, 40000);

    CHECK(factors.size() == 2);
    CHECK(factors[0].second == 1);
    CHECK(factors[1].second == 1);
    CHECK(BigInt::Eq(factors[0].first, p2));
    CHECK(BigInt::Eq(factors[1].first, p1));
  }
  #endif

  // Primorials
  #if BIG_USE_GMP
  // TODO: Enable this when we have IsPrime.
  {
    BigInt prim(1);
    for (uint64_t n = 2; n <= 128; n++) {
      if (!BigInt::IsPrime(BigInt(n)))
        continue;

      prim = BigInt::Times(prim, n);

      std::vector<std::pair<BigInt, int>> factors =
        BigInt::PrimeFactorization(prim);

      BigInt product(1);
      for (const auto &[p, e] : factors) {
        CHECK(e == 1) << n;
        for (int r = 0; r < e; r++) {
          product = BigInt::Times(product, p);
        }
      }

      CHECK(BigInt::Eq(product, prim)) << n;
    }
  }
  #endif

  printf("Prime factorization OK.\n");
}


static void BenchDiv2() {
  double total_sec = 0.0;
  const int iters = 100;
  const BigInt two(2);
  for (int i = 0; i < iters; i++) {
    BigInt x = BigInt::Pow(two, 30000);
    Timer t;

    for (;;) {
      const auto [q, r] = BigInt::QuotRem(x, two);
      if (BigInt::Eq(r, 0)) {
        x = q;
      } else {
        break;
      }
    }

    CHECK(BigInt::Eq(x, 1));

    total_sec += t.Seconds();
    if (i % 10 == 0) {
      printf("%d/%d\n", i, iters);
    }
  }

  printf("%d iters in %.5fs = %.3f/s\n",
         iters, total_sec, iters / total_sec);
}

static void TestPi() {
  printf("----\n");
  {
    BigInt i{int64_t{1234567}};
    BigInt j{int64_t{33}};
    BigInt k = BigInt::Times(i, j);
    BigInt m("102030405060708090987654321");

    printf("Integer: %s %s %s\n%s\n",
           i.ToString().c_str(),
           j.ToString().c_str(),
           k.ToString().c_str(),
           m.ToString().c_str());
    fflush(stdout);
  }

  BigRat sum;
  #if BIG_USE_GMP
  static constexpr int LIMIT = 10000;
  #else
  static constexpr int LIMIT = 1000;
  #endif

  for (int i = 0; i < LIMIT; i++) {
    // + 1/1, - 1/3, + 1/5
    BigRat term{(i & 1) ? -1 : 1,
        i * 2 + 1};
    sum = BigRat::Plus(sum, term);
    if (i < 50) {
      BigRat tpi = BigRat::Times(sum, BigRat{4,1});
      printf("Approx pi: %s = %f\n",
             tpi.ToString().c_str(),
             tpi.ToDouble());
      fflush(stdout);
    } else if (i % 1000 == 0) {
      printf("%d...\n", i);
      fflush(stdout);
    }
  }

  BigRat res = BigRat::Times(sum, BigRat(4, 1));
  printf("Final approx pi: %s\n",
         res.ToString().c_str());
  fflush(stdout);


  // This sequence converges REALLY slow!
  BigRat pi_lb(314, 100);
  BigRat pi_ub(315, 100);

  CHECK(BigRat::Compare(pi_lb, pi_ub) == -1);
  CHECK(BigRat::Compare(pi_lb, res) == -1);
  CHECK(BigRat::Compare(res, pi_ub) == -1);
}

static void TestLeadingZero() {
  // TODO: Test this through BigNum interface. Ideally
  // the test should not care about the implementation.
#ifndef BIG_USE_GMP
  CHECK(BnnNumLeadingZeroBitsInDigit(1ULL) == 63);
  CHECK(BnnNumLeadingZeroBitsInDigit(0b1000ULL) == 60);
  CHECK(BnnNumLeadingZeroBitsInDigit(~0ULL) == 0);
  CHECK(BnnNumLeadingZeroBitsInDigit(0ULL) == 64);

  CHECK(std::countl_zero<BigNumDigit>(1ULL) == 63);
  CHECK(std::countl_zero<BigNumDigit>(0b1000ULL) == 60);
  CHECK(std::countl_zero<BigNumDigit>(~0ULL) == 0);
  CHECK(std::countl_zero<BigNumDigit>(0ULL) == 64);
#endif
}

static void TestToInt() {
# define ROUNDTRIP(x) do {                                              \
  BigInt bi((int64_t)(x));                                              \
  std::optional<int64_t> io = bi.ToInt();                               \
  CHECK(io.has_value()) << StringPrintf("%llx", x) << "("               \
                        << bi.ToString(16) << ")";                      \
  CHECK((x) == io.value()) << StringPrintf("%llx", x) << " vs "         \
                           << bi.ToString(16);                          \
} while (0)

  ROUNDTRIP(0);
  ROUNDTRIP(1);
  ROUNDTRIP(-1);
  ROUNDTRIP(0x7FFFFFFEL);
  ROUNDTRIP(0x7FFFFFFFL);
  ROUNDTRIP(0x80000000LL);
  ROUNDTRIP(0x80000000LL);
  ROUNDTRIP(0x7FFFFFFFFFFFFFFELL);
  ROUNDTRIP(0x7FFFFFFFFFFFFFFFLL);

# define NOROUNDTRIP(bi) do {                                     \
    std::optional<int64_t> io = (bi).ToInt();                     \
    CHECK(!io.has_value()) << #bi << " =\n" << \
      bi.ToString() << " " << io.value();     \
  } while (false)
  NOROUNDTRIP(BigInt::Plus(BigInt(int64_t{0x7FFFFFFFFFFFFFFF}),
                           BigInt(1)));
  NOROUNDTRIP(BigInt::Minus(
                  BigInt::Negate(BigInt(int64_t{0x7FFFFFFFFFFFFFFF})),
                  BigInt(1)));
  NOROUNDTRIP(BigInt::Times(BigInt(int64_t{0x7FFFFFFFFFFFFFFF}),
                            BigInt(10000)));

# undef ROUNDTRIP
# undef NOROUNDTRIP
}

static uint64_t Sqrt64Nuprl(uint64_t xx) {
  // printf("SqrtNuprl(%llu)\n", xx);
  if (xx <= 1) return xx;
  // z = xx / 4
  uint64_t z = xx >> 2;
  uint64_t r2 = 2 * Sqrt64Nuprl(z);
  uint64_t r3 = r2 + 1;
  // printf("r2 = %llu, r3 = %llu\n", r2, r3);
  return (xx < r3 * r3) ? r2 : r3;
}

static void TestSqrt() {
  for (const uint64_t u : std::initializer_list<uint64_t>{
        1234567, 0x7FFFFFFFFFFFFFFFULL, 121,
        0, 1, 2, 3, 4, 5, 6, 9999999999999ULL, 31337 * 31337ULL}) {
    BigInt ub(u);

    // Sqare root of squares should be equal
    BigInt uu = BigInt::Times(ub, ub);
    CHECK(BigInt::Eq(BigInt::Sqrt(uu), ub)) << u;

    {
      const auto [v, vrem] = BigInt::SqrtRem(uu);
      CHECK(BigInt::Eq(v, ub));
      CHECK(BigInt::Eq(vrem, BigInt{0}));
    }

    uint64_t us64 = Sqrt64Nuprl(u);
    BigInt usb = BigInt::Sqrt(ub);
    CHECK(BigInt::Eq(usb, BigInt(us64))) << u << " " << us64;
    const auto [vsb, vrem] = BigInt::SqrtRem(ub);
    CHECK(BigInt::Eq(vsb, BigInt(us64)));
    BigInt back = BigInt::Plus(BigInt::Times(vsb, vsb), vrem);
    CHECK(BigInt::Eq(back, ub));
  }
}

static void TestGCD() {
  BigInt g = BigInt::GCD(BigInt(8), BigInt(12));
  CHECK(BigInt::Eq(g, BigInt(4)));
}

static void TestLog() {
#if BIG_USE_GMP
  BigInt x = BigInt::LeftShift(BigInt(1), 301);
  double y = BigInt::LogBase2(x);
  CHECK(y >= 300.99 && y <= 301.01) << y;

  double z = BigInt::NaturalLog(x);
  CHECK(z >= 208.6 && z <= 208.7) << z;
#else
  printf("Warning: LogBase2 not implemented\n");
#endif
}

static void TestShift() {
  {
    BigInt a{"12398471982735675717171221"};
    BigInt b = BigInt::LeftShift(a, 18);
    BigInt c = BigInt::Times(a, BigInt{262144});
    CHECK(BigInt::Eq(b, c));
  }

  {
    BigInt a{"1293472907097860173485720741"};
    BigInt c = BigInt::RightShift(a, 51);
    CHECK(BigInt::Eq(c, BigInt("574417361275"))) << c.ToString();
  }
}

static void TestAnd() {
  {
    BigInt a{"19827348723"};
    BigInt b{"123908472983749187767123045885812"};
    BigInt c = BigInt::BitwiseAnd(a, b);
    CHECK(BigInt::Eq(c, BigInt("227083376"))) << c.ToString();
  }

  {
    BigInt a{"-19827348723"};
    BigInt b{"123908472983749187767123045885812"};
    BigInt c = BigInt::BitwiseAnd(a, b);
    CHECK(BigInt::Eq(c, BigInt("123908472983749187767122818802436")))
      << c.ToString();
  }

  {
    BigInt a("11122233344487293847298734827");
    uint64_t b = 0xFEFFFFFFFFFEDCBAULL;
    uint64_t c = BigInt::BitwiseAnd(a, b);
    CHECK(c == 0xbecb0d895d7858aa) << c;
  }

  {
    BigInt z(0);
    CHECK(0 == BigInt::BitwiseAnd(z, 1234));
  }

}

static void TestXor() {
  CHECK(BigInt::Eq(BigInt{0},
                   BigInt::BitwiseXor(BigInt{12345}, BigInt{12345})));
  CHECK(BigInt::Eq(BigInt{5},
                   BigInt::BitwiseXor(BigInt{4}, BigInt{1})));
}

static void TestOr() {
  CHECK(BigInt::Eq(BigInt{12345},
                   BigInt::BitwiseOr(BigInt{12345}, BigInt{12345})));
  CHECK(BigInt::Eq(BigInt{5}, BigInt::BitwiseOr(BigInt{4}, BigInt{1})));
  CHECK(BigInt::Eq(BigInt{5}, BigInt::BitwiseOr(BigInt{5}, BigInt{1})));
}

static void TestDivExact() {
  BigInt a{"23984727341"};
  BigInt b{"12737177354116809923874293874113"};
  BigInt c = BigInt::Times(a, b);
  BigInt d = BigInt::DivExact(c, a);
  BigInt e = BigInt::DivExact(c, b);
  CHECK(BigInt::Eq(b, d));
  CHECK(BigInt::Eq(a, e));
}

static void TestDivFloor() {
  // No rounding.
  CHECK(BigInt::Eq(
            BigInt::DivFloor(BigInt(-4), BigInt(-2)),
            2));
  CHECK(BigInt::Eq(
            BigInt::DivFloor(BigInt(40), BigInt(-20)),
            -2));
  CHECK(BigInt::Eq(
            BigInt::DivFloor(BigInt(-400), 200),
            -2));
  CHECK(BigInt::Eq(
            BigInt::DivFloor(BigInt(-4000), -2000),
            2));

  // Round towards negative infinity.
  // This is the same for positive numbers.
  CHECK(BigInt::Eq(
            BigInt::Div(BigInt(1234567), 175),
            BigInt::DivFloor(BigInt(1234567), 175)));

  CHECK(BigInt::Eq(
            BigInt::DivFloor(BigInt(10), -3),
            -4));

  CHECK(BigInt::Eq(
            BigInt::DivFloor(BigInt(-10001),
                             BigInt(10)),
            -1001));
}

static void TestJacobi() {
  #if BIG_USE_GMP
  // XXX It's broken outside of GMP?

  CHECK(BigInt::Jacobi(BigInt(11), BigInt(17)) == -1);
  CHECK(BigInt::Jacobi(BigInt(1), BigInt(1)) == 1);
  CHECK(BigInt::Jacobi(BigInt(6), BigInt(15)) == 0);

  CHECK(BigInt::Jacobi(BigInt(30), BigInt(59)) == -1);
  #endif
}

static void TestModRem() {
  BigInt neg1{-1};
  BigInt four{4};

  CHECK(BigInt::Eq(BigInt::Mod(neg1, four), 3));
  const auto [q, r] = BigInt::QuotRem(neg1, four);
  CHECK(BigInt::Eq(q, 0)) << q.ToString();
  CHECK(BigInt::Eq(r, -1)) << r.ToString();
}

static void TestDiv() {
  // Check that Div has the same rounding behavior as C++11.
  for (int x = -14; x < 7; x++) {
    for (int y = -7; y < 14; y++) {
      if (y != 0) {
        int z = x / y;
        BigInt bz = BigInt::Div(BigInt(x), BigInt(y));
        BigInt bq = BigInt::QuotRem(BigInt(x), BigInt(y)).first;

        CHECK(BigInt::Eq(bz, z)) <<
          StringPrintf("%d / %d = %d (got %s)\n",
                       x, y, z, bz.ToString().c_str());

        CHECK(BigInt::Eq(bq, z)) <<
          StringPrintf("%d / %d = %d (got %s)\n",
                       x, y, z, bq.ToString().c_str());
      }
    }
  }
}

static void TestCMod() {
  // Check that CMod has the same rounding behavior as C++11.
  for (int x = -14; x < 7; x++) {
    for (int y = -7; y < 14; y++) {
      if (y != 0) {
        int z = x % y;
        BigInt bz = BigInt::CMod(BigInt(x), BigInt(y));
        BigInt br = BigInt::QuotRem(BigInt(x), BigInt(y)).second;

        CHECK(BigInt::Eq(bz, z)) <<
          StringPrintf("%d %% %d = %d (got %s)\n",
                       x, y, z, bz.ToString().c_str());
        CHECK(BigInt::Eq(br, z)) <<
          StringPrintf("%d %% %d = %d (got %s)\n",
                       x, y, z, br.ToString().c_str());

        // Also check int64 version.
        int64_t ibz = BigInt::CMod(BigInt(x), (int64_t)y);
        CHECK(BigInt::Eq(bz, ibz));
      }
    }
  }
}

#define CHECK_HAS_EQ(aopt, b) do {                        \
    std::optional<BigInt> aopt_ = (aopt);                 \
    BigInt b_ = (b);                                      \
    CHECK(aopt_.has_value()) << #aopt << " vs " << #b;    \
    CHECK(BigInt::Eq(aopt_.value(), b_)) <<               \
      #aopt << " vs " << #b << "\n"                       \
      "Got: " << aopt_.value().ToString() << "\n"         \
      "Want: " << b_.ToString();                          \
  } while (0)

static void TestInvert() {
#if BIG_USE_GMP
  for (const char *bs : {
      "1", "2", "3", "4", "5", "100", "10001", "31337",
      // 2^64
      "18446744073709551616",
      // 2^64 - 1
      "18446744073709551615",
      // 2^64 + 1
      "18446744073709551617",
      // 2^127
      "170141183460469231731687303715884105728"}) {
    const BigInt modulus(bs);

    if (BigInt::Eq(modulus, 1)) {
      // Degenerate case.
      CHECK_HAS_EQ(BigInt::ModInverse(BigInt(1), modulus),
                   BigInt(0));
    } else {
      // Otherwise, modular inverse of 1 always exists and is 1.
      CHECK_HAS_EQ(BigInt::ModInverse(BigInt(1), modulus),
                   BigInt(1));
    }

    if (BigInt::Less(modulus, BigInt(100000)) &&
        BigInt::IsPrime(modulus)) {

      for (BigInt a(1); BigInt::Less(a, modulus); a = BigInt::Plus(a, 1)) {
        auto ao = BigInt::ModInverse(a, modulus);
        CHECK(ao.has_value()) << "Every nonzero value has an inverse "
          "mod a prime p: " << a.ToString() << "^-1 mod " <<
          modulus.ToString();
        BigInt inv = ao.value();
        CHECK(BigInt::Greater(inv, 0) &&
              BigInt::Less(inv, modulus)) << inv.ToString();
        BigInt product = BigInt::CMod(BigInt::Times(inv, a), modulus);
        CHECK(BigInt::Eq(product, 1)) << product.ToString();
      }

    } else {

      for (const char *as : {
          "2", "3", "5", "6", "11", "27", "51",
          "120", "121",
          "15232", "90210",
          // 2^64 - 1
          "18446744073709551615"}) {
        const BigInt a(as);

        if (BigInt::Less(a, modulus)) {
          std::optional<BigInt> ao = BigInt::ModInverse(a, modulus);
          if (ao.has_value()) {
            BigInt inv = ao.value();
            CHECK(BigInt::Greater(inv, 0) &&
                  BigInt::Less(inv, modulus)) << inv.ToString();
            BigInt product = BigInt::CMod(BigInt::Times(inv, a), modulus);
            CHECK(BigInt::Eq(product, 1)) << product.ToString();
          }
        }
      }
    }
  }

  printf("Modular inverse OK.\n");
#else
  printf("Warning: Modular inverse not implemented.\n");
#endif
}

static void TestSwap() {
  BigInt a("11223344556677889900");
  BigInt b("55555555555555555555");

  std::swap(a, b);
  CHECK(a.ToString() == "55555555555555555555");
  CHECK(b.ToString() == "11223344556677889900");
}

static void TestCtz() {
  CHECK(BigInt::BitwiseCtz(BigInt(0)) == 0);
  CHECK(BigInt::BitwiseCtz(BigInt(-1)) == 0);
  CHECK(BigInt::BitwiseCtz(BigInt(1)) == 0);
  CHECK(BigInt::BitwiseCtz(BigInt(2)) == 1);
  CHECK(BigInt::BitwiseCtz(BigInt(-2)) == 1);

  CHECK(BigInt::BitwiseCtz(
            BigInt::Times(BigInt::Pow(BigInt{2}, 389), 33)) == 389);
}

static void TestDivisibleBy() {
  BigInt threes("3333333333333333333333333333333333333333333333");
  CHECK(BigInt::DivisibleBy(threes, 3));
  CHECK(BigInt::DivisibleBy(threes, BigInt(3)));

  BigInt p(31337);
  BigInt xp = BigInt::Times(threes, p);
  CHECK(!BigInt::DivisibleBy(p, 3));
  CHECK(!BigInt::DivisibleBy(p, BigInt(3)));
  CHECK(BigInt::DivisibleBy(xp, 3));
  CHECK(BigInt::DivisibleBy(xp, BigInt(3)));

  CHECK(BigInt::DivisibleBy(xp, xp));
  CHECK(BigInt::DivisibleBy(xp, threes));
  CHECK(BigInt::DivisibleBy(p, 1));
}

int main(int argc, char **argv) {
  printf("Start.\n");
  fflush(stdout);

  CopyAndAssign();
  TestToString();
  TestSign();
  TestToU64();

  TestDiv();
  TestCMod();
  TestModRem();

  TestEq();
  LowWord();
  TestMod();
  TestToInt();
  TestGCD();

  TestLog();

  TestDivisibleBy();
  TestDivExact();
  TestDivFloor();

  TestShift();
  TestAnd();
  TestXor();
  TestOr();
  TestCtz();

  TestLeadingZero();

  TestPow();
  TestQuotRem();

  TestPrimeFactors();

  TestPi();

  // Slow
  if (RUN_BENCHMARKS) {
    BenchDiv2();
  }

  TestSqrt();
  TestJacobi();
  TestInvert();

  TestSwap();
  TestToDouble();

  printf("OK\n");
}

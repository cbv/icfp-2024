#include "factorization.h"

#include <bit>
#include <cmath>
#include <initializer_list>
#include <cstdint>
#include <vector>
#include <utility>
#include <array>

#include "base/logging.h"
#include "base/stringprintf.h"
#include "timer.h"
#include "crypt/lfsr.h"
#include "ansi.h"
#include "arcfour.h"
#include "randutil.h"

using namespace std;

string FTOS(const std::vector<std::pair<uint64_t, int>> &fs) {
  string s;
  for (const auto &[b, i] : fs) {
    StringAppendF(&s, "%llu^%d ", b, i);
  }
  return s;
}

#if 0
static void OptimizeLimit() {
  uint64_t sum = 0xBEEF;
  double best_sec = 9999999.0;
  for (int limit = 1; limit < 10000; limit += 10) {
    Timer timer;
    {
      uint32_t a = 0x12345678;
      uint32_t b = 0xACABACAB;
      for (int j = 0; j < 100000; j++) {
        uint64_t input = ((uint64_t)a) << 32 | b;
        auto factors =
          Factorization::Factorize(input, limit);
        a = LFSRNext32(LFSRNext32(a));
        b = LFSRNext32(b);
        for (const auto &[x, y] : factors) sum += x + y;
      }
    }
    double sec = timer.Seconds();
    printf("%d: %s%s\n", limit,
           ANSI::Time(sec).c_str(), sec < best_sec ? APURPLE(" *") : "");
    if (sec < best_sec) best_sec = sec;
  }

  printf("%llu\n", sum);
}
#endif

// TODO: Benchmark this. From factor.c, gpl
[[maybe_unused]]
static uint64_t coreutils_isqrt (uint64_t n) {
  if (n == 0)
    return 0;

  int c = std::countl_zero<uint64_t>(n);

  /* Make x > sqrt(n).  This will be invariant through the loop.  */
  uint64_t x = (uint64_t) 1 << ((64 + 1 - c) / 2);

  for (;;) {
    uint64_t y = (x + n / x) / 2;
    if (y >= x)
      return x;

    x = y;
  }
}


// https://www.nuprl.org/MathLibrary/integer_sqrt/
[[maybe_unused]]
static uint64_t Sqrt64Nuprl(uint64_t xx) {
  if (xx <= 1) return xx;
  // z = xx / 4
  uint64_t z = xx >> 2;
  uint64_t r2 = 2 * Sqrt64Nuprl(z);
  uint64_t r3 = r2 + 1;
  return (xx < r3 * r3) ? r2 : r3;
}

[[maybe_unused]]
static uint64_t Sqrt64Embedded(uint64_t a) {
  uint64_t rem = 0;
  uint64_t root = 0;

  for (int i = 0; i < 32; i++) {
    root <<= 1;
    rem <<= 2;
    rem += a >> 62;
    a <<= 2;

    if (root < rem) {
      root++;
      rem -= root;
      root++;
    }
  }

  return root >> 1;
}

[[maybe_unused]]
static uint64_t Sqrt64BitGuess(const uint64_t n) {
  unsigned char shift = std::bit_width<uint64_t>(n);
  shift += shift & 1; // round up to next multiple of 2

  uint64_t result = 0;

  do {
    shift -= 2;
    result <<= 1; // make space for the next guessed bit
    result |= 1;  // guess that the next bit is 1
    result ^= result * result > (n >> shift); // revert if guess too high
  } while (shift != 0);

  return result;
}

[[maybe_unused]]
static uint64_t Sqrt64Double(uint64_t n) {
  if (n == 0) return 0;
  uint64_t r = std::sqrt((double)n);
  return r - (r * r - 1 >= n);
}

[[maybe_unused]]
static uint64_t Sqrt64Tglas(uint64_t n) {
  if (n == 0) return 0;
  uint64_t r = 0;
  for (uint64_t b = uint64_t{1} << ((std::bit_width(n) - 1) >> 1);
       b != 0;
       b >>= 1) {
    const uint64_t k = (b + 2 * r) * b;
    r |= (n >= k) * b;
    n -= (n >= k) * k;
  }
  return r;
}

#define Sqrt64 Sqrt64Double

static void TestSqrt() {
  for (uint64_t x : std::initializer_list<uint64_t>{
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        182734987, 298375827, 190238482, 19023904, 11111111,
        923742, 27271917, 127, 1, 589589581, 55555555,
        0x0FFFFFFFFFFFFFF, 0x0FFFFFFFFFFFFFE, 0x0FFFFFFFFFFFFFD,
        65537, 10002331, 100004389, 1000004777, 10000007777,
        65537ULL * 10002331ULL,
        10002331ULL * 100004389ULL,
        100004389ULL * 1000004777ULL,
        1000004777ULL * 10000007777ULL,
        10000055547037150728ULL,
        10000055547037150727ULL,
        10000055547037150726ULL,
        10000055547037150725ULL,
        10000055547037150724ULL,
        18446744073709551557ULL,
        29387401978, 2374287337, 9391919, 4474741071,
        18374, 1927340972, 29292922, 131072, 7182818}) {
    uint64_t root = Sqrt64(x);
    // What we want is the floor.
    CHECK(root * root <= x) << root << " " << x;

    // Result should fit in 32 bits. But we can't actually square
    // 2^32-1 because it would overflow. However, since that root+1
    // squared would be larger than any 64-bit number, it's correct
    // if we get it here and passed the previous check.
    CHECK(root <= 4294967295);
    if (root != 4294967295) {
      CHECK((root + 1ULL) * (root + 1ULL) > x)
        << root << " " << x << "(" << (root + 1ULL) << ")";
    }
  }
}

static void TestFactorials() {
  uint64_t fact = 1;
  for (uint64_t n = 2; n < 20; n++) {
    CHECK(fact * n > fact);
    fact *= n;

    std::vector<std::pair<uint64_t, int>> factors =
      Factorization::Factorize(fact);

    uint64_t product = 1;
    for (const auto &[p, e] : factors) {
      for (int r = 0; r < e; r++) product *= p;
    }

    CHECK(product == fact) << n;
  }
}

// 614889782588491410 has 15 distinct factors; the
// maximum for a 64-bit number:
// 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23 * 29 *
//   31 * 37 * 41 * 43 * 47
static void TestPrimorials() {
  uint64_t prim = 1;
  for (uint64_t n = 2; n <= 47; n++) {
    if (!Factorization::IsPrime(n))
      continue;

    CHECK(prim * n > prim) << n;
    prim *= n;

    std::vector<std::pair<uint64_t, int>> factors =
      Factorization::Factorize(prim);

    uint64_t product = 1;
    for (const auto &[p, e] : factors) {
      CHECK(e == 1) << n;
      for (int r = 0; r < e; r++) product *= p;
    }

    CHECK(product == prim) << n;
  }
}


static void TestPrimeFactors() {

  {
    std::vector<std::pair<uint64_t, int>> factors =
      Factorization::Factorize(7);

    CHECK(factors.size() == 1);
    CHECK(factors[0].second == 1);
    CHECK(factors[0].first == 7);
  }

  {
    std::vector<std::pair<uint64_t, int>> factors =
      Factorization::Factorize(31337);

    CHECK(factors.size() == 1);
    CHECK(factors[0].second == 1);
    CHECK(factors[0].first == 31337);
  }

  {
    uint64_t x = 31337 * 71;
    std::vector<std::pair<uint64_t, int>> factors =
      Factorization::Factorize(x);

    CHECK(factors.size() == 2) << FTOS(factors);
    CHECK(factors[0].first == 71);
    CHECK(factors[0].second == 1) << factors[0].second;
    CHECK(factors[1].first == 31337);
    CHECK(factors[1].second == 1) << factors[0].second;
  }

  {
    uint64_t sq31337 = 31337 * 31337;
    std::vector<std::pair<uint64_t, int>> factors =
      Factorization::Factorize(sq31337);

    CHECK(factors.size() == 1) << FTOS(factors);
    CHECK(factors[0].first == 31337);
    CHECK(factors[0].second == 2) << factors[0].second;
  }

  for (int n = 2; n < 100000; n++) {
    auto v = Factorization::Factorize(n);
    auto r = Factorization::ReferenceFactorize(n);
    CHECK(v == r) << "For " << n << ":\nGot:  " << FTOS(v) <<
      "\nWant: " << FTOS(r);
  }

  // Verify that the output is valid, but we don't check primality
  // of the factors.
  auto TestOne = [](uint64_t n) {
      if (n == 0) return;
      auto factors = Factorization::Factorize(n);
      uint64_t product = 1;
      for (int i = 1; i < (int)factors.size(); i++) {
        CHECK(factors[i - 1].first < factors[i].first &&
              factors[i].second >= 1) << n << "\n" << FTOS(factors);
      }

      for (const auto &[b, e] : factors)
        for (int i = 0; i < e; i++)
          product *= b;

      CHECK(n == product) << n << " != " << product << "\n"
                          << FTOS(factors);
    };

  // Test a bunch of arbitrary 64-bit numbers.
  Timer lfsr_timer;
  {
    uint32_t a = 0x12345678;
    uint32_t b = 0xACABACAB;
    for (int j = 0; j < 500000; j++) {
      uint64_t input = ((uint64_t)a) << 32 | b;
      TestOne(input);
      a = LFSRNext32(LFSRNext32(a));
      b = LFSRNext32(b);
    }
  }
  double lfsr_sec = lfsr_timer.Seconds();
  printf("500k via LFSR OK (%s)\n", ANSI::Time(lfsr_sec).c_str());

  // Carmichael numbers > 100k.
  for (uint64_t n : {
        101101, 115921, 126217, 162401, 172081, 188461, 252601, 278545,
        294409, 314821, 334153, 340561, 399001, 410041, 449065,
        488881, 512461}) {
    TestOne(n);
    CHECK(!Factorization::IsPrime(n));
  }

  CHECK(Factorization::IsPrime(0x7FFFFFFF));

  // Must all be distinct and prime. Careful about overflow!
  for (const auto &f : std::vector<std::vector<int>>{
      {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 37, 41, 43, 47, 419},
      {7907, 7919},
      {7919, 31337},
      {7919},
      {7919, 7927},
      // Miller-Rabin needs base of 11 to reject
      {151, 751, 28351},
      // Special cases for deterministic test
      {193},
      {407521},
      {299210837},
      {2, 3, 299210837},
      // primorial(15), which has the most distinct factors without
      // overflowing
      {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47},
    }) {

    for (int factor : f) {
      CHECK(Factorization::IsPrime(factor)) << factor;
    }

    auto Expected = [&f]() {
        string out;
        for (int factor : f) StringAppendF(&out, "%d, ", factor);
        return out;
      };

    uint64_t x = 1;
    for (uint64_t factor : f) {
      uint64_t xf = x * factor;
      CHECK(xf / x == factor) << "overflow";
      x = xf;
    }

    if (f.size() > 1) {
      CHECK(!Factorization::IsPrime(x));
    }

    std::vector<std::pair<uint64_t, int>> factors =
      Factorization::Factorize(x);

    CHECK(factors.size() == f.size()) << x << " got: " << FTOS(factors)
                                      << "\nBut wanted: " << Expected();
    for (int i = 0; i < (int)f.size(); i++) {
      CHECK(factors[i].second == 1);
      CHECK((int)factors[i].first == f[i]) << x << " got: " << FTOS(factors)
                                           << "\nBut wanted: " << Expected();
    }
  }

#if 0
// If we add the max_factor arg back in; consider something here.
  {
    // 100-digit prime; trial factoring will not succeed!
    uint64_t p1("207472224677348520782169522210760858748099647"
                "472111729275299258991219668475054965831008441"
                "6732550077");
    uint64_t p2("31337");

    uint64_t x = uint64_t::Times(p1, p2);

    // Importantly, we set a max factor (greater than p2, and
    // greater than the largest value in the primes list).
    std::vector<std::pair<uint64_t, int>> factors =
      Factorization::Factorize(x, 40000);

    CHECK(factors.size() == 2);
    CHECK(factors[0].second == 1);
    CHECK(factors[1].second == 1);
    CHECK(uint64_t::Eq(factors[0].first, p2));
    CHECK(uint64_t::Eq(factors[1].first, p1));
  }
#endif

  printf("TestPrimeFactors " AGREEN("OK") "\n");
}

static void BenchSqrt() {
  Timer timer;
  uint64_t result = 0;

  for (int i = 0; i < 2000; i++) {
    uint32_t a = 0xDEAD;
    uint32_t b = 0xDECEA5ED;
    for (int j = 0; j < 100000; j++) {
      uint64_t input = ((uint64_t)a) << 32 | b;
      result += Sqrt64(input);
      a = LFSRNext32(LFSRNext32(a));
      b = LFSRNext32(b);
    }
  }
  double seconds = timer.Seconds();
  printf("Result %llx\n", result);
  printf("Sqrts %.3f seconds.\n", seconds);
  printf("-------------------\n");
}

static void BenchFactorize() {
  Timer timer;
  uint64_t result = 0;

  for (int i = 0; i < 100; i++) {
    for (uint64_t n = 2; n < 100000; n++) {
      auto factors = Factorization::Factorize(n);
      for (const auto &[p, e] : factors) result += p * e;
    }
  }
  printf("Factored 100k one hundred times, %.3fs\n", timer.Seconds());

  // Now some larger ones.
  for (uint64_t num :
         std::initializer_list<uint64_t>{
         182734987, 298375827, 190238482, 19023904, 11111111,
           923742, 27271917, 127, 1, 589589581, 55555555,
           0x0FFFFFFFFFFFFFF, 0x0FFFFFFFFFFFFFE, 0x0FFFFFFFFFFFFFD,
           // primes
           65537, 10002331, 100004389, 1000004777, 10000007777,
           65537ULL * 10002331ULL,
           10002331ULL * 100004389ULL,
           100004389ULL * 1000004777ULL,
           // big, slow.
           1000004777ULL * 10000007777ULL,
           10000055547037150728ULL,
           10000055547037150727ULL,
           10000055547037150726ULL,
           10000055547037150725ULL,
           10000055547037150724ULL,
           // largest 64-bit prime; this is pretty much the worst
           // case for the trial division algorithm.
           18446744073709551557ULL,
           29387401978, 2374287337, 9391919, 4474741071,
           18374, 1927340972, 29292922, 131072, 7182818}) {
    auto factors = Factorization::Factorize(num);
    printf("%llu: %s\n", num, FTOS(factors).c_str());
    for (const auto &[p, e] : factors) result += p * e;
  }
  double seconds = timer.Seconds();
  printf("Result %llx\n", result);
  printf("Factored the list in %.3f seconds.\n", seconds);
}

static constexpr array<uint32_t, 1000> PRIMES = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
  31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
  73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
  127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
  179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
  233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
  283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
  353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
  419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
  467, 479, 487, 491, 499, 503, 509, 521, 523, 541,
  547, 557, 563, 569, 571, 577, 587, 593, 599, 601,
  607, 613, 617, 619, 631, 641, 643, 647, 653, 659,
  661, 673, 677, 683, 691, 701, 709, 719, 727, 733,
  739, 743, 751, 757, 761, 769, 773, 787, 797, 809,
  811, 821, 823, 827, 829, 839, 853, 857, 859, 863,
  877, 881, 883, 887, 907, 911, 919, 929, 937, 941,
  947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013,
  1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069,
  1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151,
  1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223,
  1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291,
  1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373,
  1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451,
  1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511,
  1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583,
  1597, 1601, 1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657,
  1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 1733,
  1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811,
  1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889,
  1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 1987,
  1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053,
  2063, 2069, 2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129,
  2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203, 2207, 2213,
  2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287,
  2293, 2297, 2309, 2311, 2333, 2339, 2341, 2347, 2351, 2357,
  2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423,
  2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531,
  2539, 2543, 2549, 2551, 2557, 2579, 2591, 2593, 2609, 2617,
  2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 2687,
  2689, 2693, 2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741,
  2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801, 2803, 2819,
  2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903,
  2909, 2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999,
  3001, 3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079,
  3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181,
  3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257,
  3259, 3271, 3299, 3301, 3307, 3313, 3319, 3323, 3329, 3331,
  3343, 3347, 3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413,
  3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511,
  3517, 3527, 3529, 3533, 3539, 3541, 3547, 3557, 3559, 3571,
  3581, 3583, 3593, 3607, 3613, 3617, 3623, 3631, 3637, 3643,
  3659, 3671, 3673, 3677, 3691, 3697, 3701, 3709, 3719, 3727,
  3733, 3739, 3761, 3767, 3769, 3779, 3793, 3797, 3803, 3821,
  3823, 3833, 3847, 3851, 3853, 3863, 3877, 3881, 3889, 3907,
  3911, 3917, 3919, 3923, 3929, 3931, 3943, 3947, 3967, 3989,
  4001, 4003, 4007, 4013, 4019, 4021, 4027, 4049, 4051, 4057,
  4073, 4079, 4091, 4093, 4099, 4111, 4127, 4129, 4133, 4139,
  4153, 4157, 4159, 4177, 4201, 4211, 4217, 4219, 4229, 4231,
  4241, 4243, 4253, 4259, 4261, 4271, 4273, 4283, 4289, 4297,
  4327, 4337, 4339, 4349, 4357, 4363, 4373, 4391, 4397, 4409,
  4421, 4423, 4441, 4447, 4451, 4457, 4463, 4481, 4483, 4493,
  4507, 4513, 4517, 4519, 4523, 4547, 4549, 4561, 4567, 4583,
  4591, 4597, 4603, 4621, 4637, 4639, 4643, 4649, 4651, 4657,
  4663, 4673, 4679, 4691, 4703, 4721, 4723, 4729, 4733, 4751,
  4759, 4783, 4787, 4789, 4793, 4799, 4801, 4813, 4817, 4831,
  4861, 4871, 4877, 4889, 4903, 4909, 4919, 4931, 4933, 4937,
  4943, 4951, 4957, 4967, 4969, 4973, 4987, 4993, 4999, 5003,
  5009, 5011, 5021, 5023, 5039, 5051, 5059, 5077, 5081, 5087,
  5099, 5101, 5107, 5113, 5119, 5147, 5153, 5167, 5171, 5179,
  5189, 5197, 5209, 5227, 5231, 5233, 5237, 5261, 5273, 5279,
  5281, 5297, 5303, 5309, 5323, 5333, 5347, 5351, 5381, 5387,
  5393, 5399, 5407, 5413, 5417, 5419, 5431, 5437, 5441, 5443,
  5449, 5471, 5477, 5479, 5483, 5501, 5503, 5507, 5519, 5521,
  5527, 5531, 5557, 5563, 5569, 5573, 5581, 5591, 5623, 5639,
  5641, 5647, 5651, 5653, 5657, 5659, 5669, 5683, 5689, 5693,
  5701, 5711, 5717, 5737, 5741, 5743, 5749, 5779, 5783, 5791,
  5801, 5807, 5813, 5821, 5827, 5839, 5843, 5849, 5851, 5857,
  5861, 5867, 5869, 5879, 5881, 5897, 5903, 5923, 5927, 5939,
  5953, 5981, 5987, 6007, 6011, 6029, 6037, 6043, 6047, 6053,
  6067, 6073, 6079, 6089, 6091, 6101, 6113, 6121, 6131, 6133,
  6143, 6151, 6163, 6173, 6197, 6199, 6203, 6211, 6217, 6221,
  6229, 6247, 6257, 6263, 6269, 6271, 6277, 6287, 6299, 6301,
  6311, 6317, 6323, 6329, 6337, 6343, 6353, 6359, 6361, 6367,
  6373, 6379, 6389, 6397, 6421, 6427, 6449, 6451, 6469, 6473,
  6481, 6491, 6521, 6529, 6547, 6551, 6553, 6563, 6569, 6571,
  6577, 6581, 6599, 6607, 6619, 6637, 6653, 6659, 6661, 6673,
  6679, 6689, 6691, 6701, 6703, 6709, 6719, 6733, 6737, 6761,
  6763, 6779, 6781, 6791, 6793, 6803, 6823, 6827, 6829, 6833,
  6841, 6857, 6863, 6869, 6871, 6883, 6899, 6907, 6911, 6917,
  6947, 6949, 6959, 6961, 6967, 6971, 6977, 6983, 6991, 6997,
  7001, 7013, 7019, 7027, 7039, 7043, 7057, 7069, 7079, 7103,
  7109, 7121, 7127, 7129, 7151, 7159, 7177, 7187, 7193, 7207,
  7211, 7213, 7219, 7229, 7237, 7243, 7247, 7253, 7283, 7297,
  7307, 7309, 7321, 7331, 7333, 7349, 7351, 7369, 7393, 7411,
  7417, 7433, 7451, 7457, 7459, 7477, 7481, 7487, 7489, 7499,
  7507, 7517, 7523, 7529, 7537, 7541, 7547, 7549, 7559, 7561,
  7573, 7577, 7583, 7589, 7591, 7603, 7607, 7621, 7639, 7643,
  7649, 7669, 7673, 7681, 7687, 7691, 7699, 7703, 7717, 7723,
  7727, 7741, 7753, 7757, 7759, 7789, 7793, 7817, 7823, 7829,
  7841, 7853, 7867, 7873, 7877, 7879, 7883, 7901, 7907, 7919,
};
static constexpr int LAST_PRIME = PRIMES[PRIMES.size() - 1];

static void TestIsPrime() {
  // 2^42 - 11
  CHECK(Factorization::IsPrime(4398046511093ULL));
  CHECK(!Factorization::IsPrime(2330708273ULL * 9868769ULL));

  int next_prime_index = 0;
  for (int n = 0; n <= LAST_PRIME; n++) {
    const int p = PRIMES[next_prime_index];
    if (n == p) {
      CHECK(Factorization::IsPrime(p)) << p;
      next_prime_index++;
    } else {
      CHECK(!Factorization::IsPrime(n)) << n;
    }
  }

  printf("TestIsPrime " AGREEN("OK") "\n");
}

template<size_t N>
[[maybe_unused]]
static void PrintGaps(const std::array<uint32_t, N> &primes) {
  CHECK(primes[0] == 2);
  for (int i = 1; i < N; i++) {
    int gap = primes[i] - primes[i - 1];
    CHECK(gap > 0 && gap < 256);
    printf("%d,", gap);
    if (i % 29 == 0) printf("\n");
  }
}


[[maybe_unused]]
static void ProfileFactorize() {
  ArcFour rc("profile");
  Timer timer;
  uint64_t result = 0;

  static constexpr int TIMES = 1000000;
  for (int i = 0; i < TIMES; i++) {
    const uint64_t num = Rand64(&rc) & 0xFFFFFFFFFF;
    auto factors = Factorization::Factorize(num);
    for (const auto &[p, e] : factors) result += p * e;
  }

  printf("[%llx], Factored %d numbers, %.3fs\n",
         result, TIMES, timer.Seconds());
}

static void TestNextPrime() {
  CHECK(Factorization::NextPrime(2) == 3);
  CHECK(Factorization::NextPrime(1) == 2);
  CHECK(Factorization::NextPrime(0) == 2);
  CHECK(Factorization::NextPrime(3) == 5);
  CHECK(Factorization::NextPrime(4) == 5);
  CHECK(Factorization::NextPrime(8) == 11);
  CHECK(Factorization::NextPrime(2147483646) == 2147483647);
  CHECK(Factorization::NextPrime(31333) == 31337);
}

int main(int argc, char **argv) {
  ANSI::Init();
  printf("Test factorization...\n");

  // PrintGaps(PRIMES);
  // return 0;

  // OptimizeLimit();
  // return 0;

  TestSqrt();
  TestPrimeFactors();
  TestIsPrime();

  BenchSqrt();

  BenchFactorize();

  TestNextPrime();

  TestFactorials();
  TestPrimorials();

  // ProfileFactorize();

  printf("OK\n");
}

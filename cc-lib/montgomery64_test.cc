
#include "montgomery64.h"

#include <cstdint>
#include <initializer_list>
#include <unordered_set>

#include "base/logging.h"
#include "base/int128.h"
#include "arcfour.h"
#include "randutil.h"
#include "ansi.h"

static inline uint64_t PlusMod(uint64_t a, uint64_t b, uint64_t m) {
  uint128_t aa(a);
  uint128_t bb(b);
  uint128_t mm(m);
  uint128_t rr = (aa + bb) % mm;
  CHECK(Uint128High64(rr) == 0);
  return Uint128Low64(rr);
}

static inline uint64_t SubMod(uint64_t a, uint64_t b, uint64_t m) {
  int128_t aa(a);
  int128_t bb(b);
  int128_t mm(m);
  int128_t rr = (aa - bb) % mm;
  if (rr < 0) {
    rr += m;
  }
  CHECK(rr >= 0);
  uint128_t uu = (uint128_t)rr;
  CHECK(Uint128High64(uu) == 0);
  uint64_t u = (uint64_t)Uint128Low64(uu);
  // printf("%llu - %llu mod %llu = %llu\n", a, b, m, u);
  return u;
}

static inline uint64_t MultMod(uint64_t a, uint64_t b,
                               uint64_t m) {
  uint128 aa(a);
  uint128 bb(b);
  uint128 mm(m);

  uint128 rr = (aa * bb) % mm;
  CHECK(Uint128High64(rr) == 0);
  return Uint128Low64(rr);
}

// Linear time!
static inline uint64_t SimpleModPow(uint64_t b, uint64_t e,
                                    uint64_t m) {
  uint64_t res = 1;
  for (uint64_t i = 0; i < e; i++) {
    res *= b;
    res %= m;
  }
  return res;
}

static void TestOne() {
  for (const uint64_t m : std::initializer_list<uint64_t>{
      3, 5, 7, 9, 11, 15, 121, 31337, 131073, 23847198347561,
    }) {
    CHECK(m > 1 && (m & 1) == 1) << m;
    MontgomeryRep64 rep(m);

    // printf("[%llu] one: %llu mont -> %llu reg\n", m, rep.One().x,
    // rep.ToInt(rep.One()));
    CHECK(rep.ToInt(rep.One()) == 1);
    CHECK(rep.One().x == rep.ToMontgomery(1).x);
  }
}

static void TestExp() {
  {
    const uint64_t m = 12345678901ULL;
    const MontgomeryRep64 rep(m);

    const Montgomery64 a = rep.Pow(rep.ToMontgomery(2), 80);
    uint64_t ma = rep.ToInt(a);
    uint64_t sa = SimpleModPow(2, 80, m);
    CHECK(ma == sa) << ma << " " << sa;

    const Montgomery64 b = rep.Pow(rep.ToMontgomery(1110238741), 12345);
    CHECK(rep.ToInt(b) == SimpleModPow(1110238741, 12345, m));
  }

  {
    const uint64_t m = 173;
    const uint64_t u = 169;
    const MontgomeryRep64 rep(m);

    const uint64_t base = (u * 2) % m;

    Montgomery64 a = rep.Pow(rep.ToMontgomery(base), 21);
    uint64_t ma = rep.ToInt(a);
    uint64_t sa = SimpleModPow(base, 21, m);
    CHECK(ma == sa) << ma << " " << sa;
  }

  // Could test more...
}

static void TestNth() {
  for (uint64_t m : std::initializer_list<uint64_t>{
      3, 7, 11, 21, 19, 65, 121, 173}) {

    const MontgomeryRep64 rep(m);

    std::unordered_set<uint64_t> found;
    for (uint64_t i = 0; i < m; i++) {
      Montgomery64 r = rep.Nth(i);

      uint64_t ri = rep.ToInt(r);
      CHECK(!found.contains(ri));
      found.insert(ri);
    }

    // Then it must be a permutation.
    CHECK(found.size() == m);
  }
}

static void TestBasic() {
  for (uint64_t m : std::initializer_list<uint64_t>{
      7, 11, 21, 19, 65, 121, 173, 31337, 131073, 999998999,
      10000001000001,
      // 2^62 -1, +1,
      4611686018427387903, 4611686018427387905,
    }) {
    CHECK((m & 1) == 1) << "Modulus must be odd: " << m;

    const MontgomeryRep64 rep(m);

    for (uint64_t a : std::initializer_list<uint64_t>{
        0, 1, 2, 3, 4, 5, 7, 12, 64, 120, 31337, 131072, 999999999,
        10000000000001,
        // 2^62 -1, +0, +1
        4611686018427387903, 4611686018427387904, 4611686018427387905,
      }) {

      const uint64_t amod = a % m;
      const Montgomery64 am = rep.ToMontgomery(amod);
      CHECK(rep.ToInt(am) == amod) << a << " mod " << m;

      // Inverse
      const uint64_t neg_a = (m - amod) % m;
      CHECK(neg_a >= 0 && neg_a < m);
      const Montgomery64 negam = rep.Negate(am);
      CHECK(rep.Eq(rep.ToMontgomery(neg_a), negam));
      CHECK(rep.Eq(rep.Zero(), rep.Add(negam, am)));

      for (uint64_t b : std::initializer_list<uint64_t>{
          0, 1, 2, 7, 11, 16, 19, 64, 120, 169, 31337, 131072, 999998999,
          10000001000001,
          // 2^62 -1, +0, +1
          4611686018427387903, 4611686018427387904, 4611686018427387905,
        }) {

        const uint64_t bmod = b % m;
        const Montgomery64 bm = rep.ToMontgomery(bmod);
        CHECK(rep.ToInt(bm) == bmod) << b << " mod " << m
                                     << "\nGot:  " << rep.ToInt(bm)
                                     << "\nWant: " << bmod;

        // Plus
        const Montgomery64 aplusb = rep.Add(am, bm);
        CHECK(rep.ToInt(aplusb) == PlusMod(amod, bmod, m)) <<
          amod << " + " << bmod << " mod " << m
               << "\nam: " << am.x << " bm: " << bm.x
               << "\nGot:  " << rep.ToInt(aplusb)
               << "\nWant: " << PlusMod(amod, bmod, m);


        const Montgomery64 aminusb = rep.Sub(am, bm);
        CHECK(rep.ToInt(aminusb) == SubMod(amod, bmod, m)) <<
          amod << " - " << bmod << " mod " << m;

        const Montgomery64 bminusa = rep.Sub(bm, am);
        CHECK(rep.ToInt(bminusa) == SubMod(bmod, amod, m));

        const Montgomery64 atimesb = rep.Mult(am, bm);
        CHECK(rep.ToInt(atimesb) == MultMod(amod, bmod, m));

      }
    }
  }
}

static void TestRandom() {
  ArcFour rc("test");

  static constexpr int ITERS = 10000000;
  for (int i = 0; i < ITERS; i++) {
    // TODO: Should be able to make this work for full 64 bits,
    // but we don't actually need that yet.
    constexpr uint64_t BITS63 = 0x7fffffffffffffffULL;
    // const uint64_t m = (Rand64(&rc) & BITS63) | 0b1;
    const uint64_t m = (Rand64(&rc) & 0xFF) | 0b1;
    if (m == 1) continue;

    const uint64_t amod = (Rand64(&rc) & BITS63) % m;
    const uint64_t bmod = (Rand64(&rc) & BITS63) % m;

    const MontgomeryRep64 rep(m);

    const Montgomery64 am = rep.ToMontgomery(amod);
    CHECK(rep.ToInt(am) == amod) << amod << " mod " << m;

    const Montgomery64 bm = rep.ToMontgomery(bmod);
    CHECK(rep.ToInt(bm) == bmod) << bmod << " mod " << m;

    const Montgomery64 aplusb = rep.Add(am, bm);
    CHECK(rep.ToInt(aplusb) == PlusMod(amod, bmod, m));

    const Montgomery64 aminusb = rep.Sub(am, bm);
    CHECK(rep.ToInt(aminusb) == SubMod(amod, bmod, m));

    const Montgomery64 bminusa = rep.Sub(bm, am);
    CHECK(rep.ToInt(bminusa) == SubMod(bmod, amod, m));

    const Montgomery64 atimesb = rep.Mult(am, bm);
    CHECK(rep.ToInt(atimesb) == MultMod(amod, bmod, m));

    for (Montgomery64 v : {am, bm, aplusb, aminusb, bminusa, atimesb}) {
      CHECK(v.x < m) << v.x << " vs " << m;
    }
  }
}

int main(int argc, char **argv) {
  ANSI::Init();

  TestOne();
  TestNth();
  TestBasic();
  TestExp();
  TestRandom();

  printf("OK");
  return 0;
}

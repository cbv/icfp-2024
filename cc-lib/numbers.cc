#include "numbers.h"

#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include <bit>
#include <tuple>
#include <cstdint>

#include "base/logging.h"
#include "montgomery64.h"

static constexpr bool VERBOSE = false;
static constexpr bool SELF_CHECK = false;

// PERF: Can do this with a loop, plus bit tricks to avoid
// division.
std::tuple<int64_t, int64_t, int64_t>
ReferenceExtendedGCD64Internal(int64_t a, int64_t b) {
  if (a == 0) return std::make_tuple(b, 0, 1);
  const auto &[gcd, x1, y1] = ReferenceExtendedGCD64Internal(b % a, a);
  return std::make_tuple(gcd, y1 - (b / a) * x1, x1);
}

std::tuple<int64_t, int64_t, int64_t>
ReferenceExtendedGCD64(int64_t a, int64_t b) {
  const auto &[gcd, x, y] = ReferenceExtendedGCD64Internal(a, b);
  if (gcd < 0) return std::make_tuple(-gcd, -x, -y);
  else return std::make_tuple(gcd, x, y);
}

std::tuple<int64_t, int64_t, int64_t>
OldExtendedGCD64Internal(int64_t a, int64_t b) {
  if (VERBOSE)
    printf("gcd(%lld, %lld)\n", a, b);

  a = abs(a);
  b = abs(b);

  if (a == 0) return std::make_tuple(b, 0, 1);
  if (b == 0) return std::make_tuple(a, 1, 0);


  // Remove common factors of 2.
  const int r = std::countr_zero<uint64_t>(a | b);
  a >>= r;
  b >>= r;

  int64_t alpha = a;
  int64_t beta = b;

  if (VERBOSE) {
    printf("Alpha: %lld, Beta: %lld\n", alpha, beta);
  }

  int64_t u = 1, v = 0, s = 0, t = 1;

  if (VERBOSE) {
    printf("2Loop %lld = %lld alpha + %lld beta | "
           "%lld = %lld alpha + %lld beta\n",
           a, u, v, b, s, t);
  }

  if (SELF_CHECK) {
    CHECK(a == u * alpha + v * beta) << a << " = "
                                     << u * alpha << " + "
                                     << v * beta << " = "
                                     << (u * alpha) + (v * beta);
    CHECK(b == s * alpha + t * beta);
  }

  int azero = std::countr_zero<uint64_t>(a);
  printf("azero: %d\n", azero);
  if (azero > 0) {

    int uvzero = std::countr_zero<uint64_t>(u | v);

    // shift away all the zeroes in a.
    a >>= azero;

    int all_zero = std::min(azero, uvzero);
    u >>= all_zero;
    v >>= all_zero;

    int rzero = azero - all_zero;
    if (VERBOSE)
      printf("azero %d uvzero %d all_zero %d rzero %d\n",
             azero, uvzero, all_zero, rzero);

    for (int i = 0; i < rzero; i++) {
      // PERF: The first time through, we know we will
      // enter the top branch.
      if ((u | v) & 1) {
        u += beta;
        v -= alpha;
      }

      u >>= 1;
      v >>= 1;
    }
  }

  while (a != b) {
    if (VERBOSE) {
      printf("Loop %lld = %lld alpha + %lld beta | "
             "%lld = %lld alpha + %lld beta\n",
             a, u, v, b, s, t);
    }

    if (SELF_CHECK) {
      CHECK(a == u * alpha + v * beta) << a << " = "
                                       << u * alpha << " + "
                                       << v * beta << " = "
                                       << (u * alpha) + (v * beta);
      CHECK(b == s * alpha + t * beta);

      CHECK((a & 1) == 1);
    }

    // Loop invariant.
    // PERF: I think that this loop could still be improved.
    // I explicitly skip some of the tests that gcc couldn't
    // figure out, but it still generates explicit move
    // instructions for the swap; we could just have six
    // states and track that manually.
    // if ((a & 1) == 0) __builtin_unreachable();

    // one:
    if ((b & 1) == 0) {
    one_even:
      if (SELF_CHECK) { CHECK((b & 1) == 0); }

      b >>= 1;
      if (((s | t) & 1) == 0) {
        s >>= 1;
        t >>= 1;
      } else {
        s = (s + beta) >> 1;
        t = (t - alpha) >> 1;
      }

      // could cause a to equal b
      continue;
    }

    // two:
    if (b < a) {
      // printf("Swap.\n");
      std::swap(a, b);
      std::swap(s, u);
      std::swap(t, v);

      // we know a is odd, and now a < b, so we
      // go to case three.

      goto three;
    }

  three:
    b -= a;
    s -= u;
    t -= v;
    if (SELF_CHECK) {
      // we would only have b == a here if b was 2a.
      // but this is impossible since b was odd.
      CHECK(b != a);
      // but since we had odd - odd, we b is now even.
      CHECK((b & 1) == 0);
    }
    // so we know we enter that branch next.

    goto one_even;
  }

  return std::make_tuple(a << r, s, t);
}

std::tuple<int64_t, int64_t, int64_t>
OldExtendedGCD64(int64_t a, int64_t b) {
  const auto &[gcd, x, y] = OldExtendedGCD64Internal(a, b);
  if (SELF_CHECK) {
    CHECK(gcd >= 0);
  }
  // Negate coefficients if they start negative.
  return std::make_tuple(gcd, a < 0 ? -x : x, b < 0 ? -y : y);
}

// This routine is adapted from GMP. GPL License; see bottom and
// COPYING.

// Zerotab was for fast calculation of trailing zeroes; unnecessary.

// TODO: Benchmark these
#define GCDEXT_1_BINARY_METHOD 1

// returns 0 or 0xFF..FF.
// In c++11, signed right shift is arithmetic.
#define LIMB_HIGHBIT_TO_MASK(n) ((int64_t(n)) >> 63)

static
uint64_t
mpn_gcdext_1(int64_t *sp, int64_t *tp,
             uint64_t u, uint64_t v) {
  /* Maintain

     U = t1 u + t0 v
     V = s1 u + s0 v

     where U, V are the inputs (without any shared power of two),
     and the matrix has determinant +/- 2^{shift}.
  */
  uint64_t s0 = 1;
  uint64_t t0 = 0;
  uint64_t s1 = 0;
  uint64_t t1 = 1;
  uint64_t ug;
  uint64_t vg;
  uint64_t ugh;
  uint64_t vgh;
#if GCDEXT_1_BINARY_METHOD == 2
  uint64_t det_sign;
#endif

  CHECK(u > 0);
  CHECK(v > 0);

  int zero_bits = std::countr_zero(u | v);
  // count_trailing_zeros (zero_bits, u | v);
  u >>= zero_bits;
  v >>= zero_bits;

  int shift = 0;
  if ((u & 1) == 0) {
    shift = std::countr_zero(u);
    // count_trailing_zeros (shift, u);
    u >>= shift;
    t1 <<= shift;
  } else if ((v & 1) == 0) {
    shift = std::countr_zero(v);
    // count_trailing_zeros (shift, v);
    v >>= shift;
    s0 <<= shift;
  }

#if GCDEXT_1_BINARY_METHOD == 1
  while (u != v) {
    if (u > v) {
      u -= v;

      int count = std::countr_zero(u);
      // count_trailing_zeros (count, u);
      u >>= count;

      t0 += t1; t1 <<= count;
      s0 += s1; s1 <<= count;
      shift += count;
    } else {
      v -= u;

      int count = std::countr_zero(v);
      // count_trailing_zeros (count, v);
      v >>= count;

      t1 += t0; t0 <<= count;
      s1 += s0; s0 <<= count;
      shift += count;
    }
  }
#else
# if GCDEXT_1_BINARY_METHOD == 2
  u >>= 1;
  v >>= 1;

  det_sign = 0;

  while (u != v) {
    uint64_t d = u - v;
    uint64_t vgtu = LIMB_HIGHBIT_TO_MASK(d);
    uint64_t sx;
    uint64_t tx;

    /* When v <= u (vgtu == 0), the updates are:

       (u; v)   <-- ( (u - v) >> count; v)    (det = +(1<<count) for corr. M factor)
       (t1, t0) <-- (t1 << count, t0 + t1)

       and when v > 0, the updates are

       (u; v)   <-- ( (v - u) >> count; u)    (det = -(1<<count))
       (t1, t0) <-- (t0 << count, t0 + t1)

       and similarly for s1, s0
    */

    /* v <-- min (u, v) */
    v += (vgtu & d);

    /* u <-- |u - v| */
    u = (d ^ vgtu) - vgtu;

    /* Number of trailing zeros is the same no matter if we look at
       d or u, but using d gives more parallelism. */
    int count = std::countr_zero(d);
    // count_trailing_zeros (count, d);

    det_sign ^= vgtu;

    tx = vgtu & (t0 - t1);
    sx = vgtu & (s0 - s1);
    t0 += t1;
    s0 += s1;
    t1 += tx;
    s1 += sx;

    count++;
    u >>= count;
    t1 <<= count;
    s1 <<= count;
    shift += count;
  }
  u = (u << 1) + 1;

# else /* GCDEXT_1_BINARY_METHOD == 2 */
#  error Unknown GCDEXT_1_BINARY_METHOD
# endif
#endif

  /* Now u = v = g = gcd (u,v). Compute U/g and V/g */
  ug = t0 + t1;
  vg = s0 + s1;

  ugh = ug/2 + (ug & 1);
  vgh = vg/2 + (vg & 1);

  /* Now (+/-2)^{shift} g = s0 U - t0 V. Get rid of the power of two, using
     s0 U - t0 V = (s0 + V/g) U - (t0 + U/g) V. */
  for (int i = 0; i < shift; i++) {
    uint64_t mask = - ( (s0 | t0) & 1);

    s0 /= 2;
    t0 /= 2;
    s0 += mask & vgh;
    t0 += mask & ugh;
  }

  /* FIXME: Try simplifying this condition. */
  if ( (s0 > 1 && 2*s0 >= vg) || (t0 > 1 && 2*t0 >= ug) ) {
    s0 -= vg;
    t0 -= ug;
  }

#if GCDEXT_1_BINARY_METHOD == 2
  /* Conditional negation. */
  s0 = (s0 ^ det_sign) - det_sign;
  t0 = (t0 ^ det_sign) - det_sign;
#endif

  *sp = s0;
  *tp = -t0;

  return u << zero_bits;
}

std::tuple<int64_t, int64_t, int64_t>
ExtendedGCD64(int64_t a, int64_t b) {
  if (a == 0) return std::make_tuple(b, 0, 1);
  if (b == 0) return std::make_tuple(a, 1, 0);

  uint64_t ua = abs(a);
  uint64_t ub = abs(b);

  int64_t s = 0, t = 0;

  uint64_t gcd = mpn_gcdext_1(&s, &t, ua, ub);
  return std::make_tuple(gcd,
                         a < 0 ? -s : s,
                         b < 0 ? -t : t);
}

std::optional<uint64_t> SqrtModP(uint64_t base, uint64_t prime) {

  static constexpr bool VERBOSE = false;
  static constexpr bool SELF_CHECK = false;

  if (VERBOSE) {
    printf("Sqrt(%llu) mod %llu\n", base, prime);
  }

  if (prime == 2) {
    // Both 0^2 = 0 and 1^1 = 1.
    return {base};
  }

  MontgomeryRep64 rep(prime);

  Montgomery64 basem = rep.ToMontgomery(base);

  // Compute Euler criteria. a^(p-1) / 2 must be 1.
  if (VERBOSE) {
    printf("%llu^%llu mod %llu\n", base, (prime - 1) >> 1, prime);
  }
  Montgomery64 rm = rep.Pow(basem, (prime - 1) >> 1);
  uint64_t r = rep.ToInt(rm);
  if (r != 1) {
    if (VERBOSE) {
      printf("Euler criteria is %llu\n", r);
    }
    // But as a special case, the sqrt(0) is always 0.
    if (base == 0) return {0};
    // Otherwise there is no solution.
    return std::nullopt;
  }

  if ((prime & 3) == 3) [[unlikely]] {
    // prime mod 4 = 3
    // subtractdivide(&Q, -1, 4);   // Q <- (prime+1)/4.

    if (VERBOSE) {
      printf("Prime & 3 == 3\n");
    }
    // Empirically, this branch isn't entered. It must fail
    // the residue test or something?

    Montgomery64 rm = rep.Pow(basem, (prime + 1) >> 2);
    uint64_t r = rep.ToInt(rm);
    return {r};

  } else {

    Montgomery64 to_convert;

    // Convert discriminant to Montgomery notation.
    // u
    const Montgomery64 aux6 = basem;

    if (SELF_CHECK) {
      CHECK(base == rep.ToInt(aux6)) << base << " " << rep.ToInt(aux6);
    }

    if (VERBOSE)
    printf("aux5 %llu = aux6 %llu\n",
           base,
           rep.ToInt(aux6));

    if ((prime & 7) == 5) {
      if (VERBOSE)
      printf("prime & 7 == 5.\n");

      // prime mod 8 = 5: use Atkin's method for modular square roots.
      // Step 1. v <- (2u)^((p-5)/8) mod p
      // Step 2. i <- (2uv^2) mod p
      // Step 3. square root of u <- uv (i-1)
      // Step 1.
      // Q <- (prime-5)/8.
      CHECK(prime >= 5);
      uint64_t q = (prime - 5) >> 3;

      if (VERBOSE)
      printf("q: %llu\n", q);

      // 2u
      const Montgomery64 aux7 = rep.Add(aux6, aux6);
      const Montgomery64 aux8 = rep.Pow(aux7, q);
      // At this moment aux7 is v in Montgomery notation.

      if (VERBOSE)
      printf("before step2: aux7 %llu aux8 %llu\n",
             rep.ToInt(aux7), rep.ToInt(aux8));

      // Step 2.
      // v^2
      Montgomery64 aux9 = rep.Mult(aux8, aux8);

      // i
      aux9 = rep.Mult(aux7, aux9);

      if (VERBOSE)
        printf("aux9 before step3: %llu\n", rep.ToInt(aux9));

      // Step 3.
      // i-1
      aux9 = rep.Sub(aux9, rep.One());
      // v*(i-1)
      aux9 = rep.Mult(aux8, aux9);
      // u*v*(i-1)
      aux9 = rep.Mult(aux6, aux9);
      to_convert = aux9;
    } else {

      if (VERBOSE)
      printf("prime & 7 == %llu\n", prime & 7);

      // prime = 1 (mod 8). Use Shanks' method for modular square roots.
      // Step 1. Select e >= 3, q odd such that p = 2^e * q + 1.
      // Step 2. Choose x at random in the range 1 < x < p such that
      //           jacobi (x, p) = -1.
      // Step 3. Set z <- x^q (mod p).
      // Step 4. Set y <- z, r <- e, x <- a^((q-1)/2) mod p,
      //           v <- ax mod p, w <- vx mod p.
      // Step 5. If w = 1, the computation ends with +/-v as the square root.
      // Step 6. Find the smallest value of k such that w^(2^k) = 1 (mod p)
      // Step 7. Set d <- y^(2^(r-k-1)) mod p, y <- d^2 mod p,
      //           r <- k, v <- dv mod p, w <- wy mod p.
      // Step 8. Go to step 5.

      // Step 1.

      uint64_t qq = prime - 1;
      CHECK(qq != 0);
      int e = std::countr_zero(qq);
      qq >>= e;

      // Step 2.
      int x = 1;

      do {
        x++;
      } while (Jacobi64(x, prime) >= 0);

      if (VERBOSE)
      printf("  x: %d  qq: %llu\n", x, qq);

      // Step 3.
      // Get z <- x^q (mod p) in Montgomery notation.
      // z
      Montgomery64 aux5 = rep.Pow(rep.ToMontgomery(x), qq);

      // Step 4.
      // y
      int r = e;

      CHECK(qq > 0);
      uint64_t kk1 = (qq - 1) >> 1;

      // x
      Montgomery64 aux7 = rep.Pow(aux6, kk1);
      // v
      Montgomery64 aux8 = rep.Mult(aux6, aux7);
      // w
      Montgomery64 aux9 = rep.Mult(aux8, aux7);

      if (VERBOSE)
      printf("  Z %llu aux7 %llu aux8 %llu aux9 %llu\n",
             rep.ToInt(aux5),
             rep.ToInt(aux7),
             rep.ToInt(aux8),
             rep.ToInt(aux9));

      // Step 5
      while (!rep.Eq(aux9, rep.One())) {
        // memcmp(aux9, params->R1.data(), NumberLengthBytes) != 0

        if (VERBOSE)
          printf("  Start loop aux9=%llu\n", rep.ToInt(aux9));

        // Step 6
        int k = 0;
        Montgomery64 aux10 = aux9;
        // (void)memcpy(aux10, aux9, NumberLengthBytes);
        do {
          k++;
          aux10 = rep.Mult(aux10, aux10);
        } while (!rep.Eq(aux10, rep.One()));
        // memcmp(aux10, params->R1.data(), NumberLengthBytes) != 0)

        if (VERBOSE)
        printf(
            "    r %d k %d aux5 %llu aux10 %llu\n",
            r, k, rep.ToInt(aux5), rep.ToInt(aux10));

        // Step 7
        // d
        aux10 = aux5;
        // (void)memcpy(aux10, aux5, NumberLengthBytes);
        for (int ctr = 0; ctr < (r - k - 1); ctr++) {
          aux10 = rep.Mult(aux10, aux10);
        }
        if (VERBOSE)
        printf(
            "    aux10 %llu\n", rep.ToInt(aux10));

        // y
        aux5 = rep.Mult(aux10, aux10);
        r = k;
        // v
        aux8 = rep.Mult(aux8, aux10);
        // w
        aux9 = rep.Mult(aux9, aux5);

        if (VERBOSE)
        printf(
            "    aux5 %llu aux8 %llu aux9 %llu aux10 %llu\n",
            rep.ToInt(aux5), rep.ToInt(aux8),
            rep.ToInt(aux9), rep.ToInt(aux10));
      }
      to_convert = aux8;
    }

    // CHECK(to_convert == aux8 || to_convert == aux9);

    // Convert from Montgomery to standard notation.

    // Port note: Alpertron does this by multiplying by 1 to
    // get the call to Reduce. We skip the multiplication.
    uint64_t res = rep.ToInt(to_convert);
    if (VERBOSE)
    printf("  returning %llu\n", res);
    return {res};
  }
}

bool IsSquareModP(uint64_t base, uint64_t prime) {
  static constexpr bool VERBOSE = false;
  static constexpr bool SELF_CHECK = false;

  if (VERBOSE) {
    printf("IsSquare(%llu) mod %llu\n", base, prime);
  }

  if (SELF_CHECK) {
    CHECK(base < prime);
  }

  if (prime == 2) {
    // Both 0^2 = 0 and 1^1 = 1.
    return true;
  }

  MontgomeryRep64 rep(prime);
  Montgomery64 basem = rep.ToMontgomery(base);

  return IsSquareModP(basem, rep);
}

/*

Portions derived from the GNU MP Library.

Copyright 1996, 1998, 2000-2005, 2008, 2009 Free Software Foundation, Inc.

The GNU MP Library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The GNU MP Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the GNU MP Library.  If not,
see https://www.gnu.org/licenses/.  */

/*
  Portions derived from Alpertron Calculators.
  Copyright 2017-2021 Dario Alejandro Alpern

  GNU Public License. See COPYING for further details.

  Alpertron Calculators is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Alpertron Calculators is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Alpertron Calculators. If not, see <http://www.gnu.org/licenses/>.
*/

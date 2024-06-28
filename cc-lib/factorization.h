
#ifndef _CC_LIB_FACTORIZATION_H
#define _CC_LIB_FACTORIZATION_H

#include <vector>
#include <utility>
#include <cstdint>

struct Factorization {
  // Prime factorization with trial division (not fast). Input must be > 1.
  // Output is pairs of [prime, exponent] in sorted order (by prime).
  static std::vector<std::pair<uint64_t, int>> Factorize(uint64_t n);

  // Writes to parallel arrays bases,exponents. Returns the number of
  // elements written. There is always at least one factor, and the
  // largest number of factors is 15 (because primorial(16), the product
  // of the first 16 primes, is greater than 2^64).
  static int FactorizePreallocated(
      uint64_t n,
      uint64_t *bases,
      uint8_t *exponents);

  // First prime not in the list of trial divisions.
  static constexpr int NEXT_PRIME = 137;

  // For advanced uses: As with the Preallocated version, but n must
  // be >= NEXT_PRIME and not contain any factors < NEXT_PRIME.
  static int FactorizePredivided(
      uint64_t n,
      uint64_t *bases,
      uint8_t *exponents);

  // Make sure bases are unique (by adding their exponents), and sort
  // in ascending order. Note that the factorization functions here
  // already produce unique factors.
  static void NormalizeFactors(std::vector<std::pair<uint64_t, int>> *factors);

  // Exact primality test (Lucas).
  static bool IsPrime(uint64_t n);

  // Return n or the next number larger than n that is prime.
  // n must be < 18446744073709551557, the largest 64-bit prime.
  // Not fast! Currently just running IsPrime on odd numbers.
  static uint64_t NextPrime(uint64_t n);

  // Simpler, slower reference version. Generally just useful for testing.
  static std::vector<std::pair<uint64_t, int>> ReferenceFactorize(uint64_t n);
};

#endif

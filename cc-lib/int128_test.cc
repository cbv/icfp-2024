
#include "base/int128.h"

#include <cstdio>

#include "ansi.h"
#include "base/logging.h"

static void Basic() {

  // 2^70 * 3 * 7^2
  uint128 u = 1;
  u <<= 70;
  u *= 3;
  u *= 7 * 7;

  CHECK((uint64_t)u == 0);
  CHECK((uint64_t)(u >> 64) == 9408);

  CHECK(u % 7 == 0);
  CHECK(u % 3 == 0);
  CHECK(u % 2 == 0);
  CHECK(u % 49 == 0);
  CHECK(u % 21 == 0);
  CHECK(u % 42 == 0);

  u++;

  CHECK(u % 7 == 1);
  CHECK(u % 42 == 1);
  CHECK(u % 3 == 1);
}

static void Signed() {
  uint128_t x = 14;
  int128_t y = (int128_t)x;
  CHECK(y > 0);
}

static void SignedMod() {
  int128_t x = 131072;
  int128_t y = 42;

  int128_t z = x % y;
  CHECK(z == (int128_t)32);
}

int main(int argc, char **argv) {
  ANSI::Init();

  Basic();
  Signed();
  SignedMod();

  printf("OK\n");
  return 0;
}

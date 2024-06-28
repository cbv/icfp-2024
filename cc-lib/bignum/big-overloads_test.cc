
#include "bignum/big.h"
#include "bignum/big-overloads.h"

#include <cstdio>

#include "timer.h"
#include "ansi.h"
#include "base/logging.h"

static void BenchNegate() {
  Timer timer;
  BigInt x = BigInt{1} << 4000000;
  for (int i = 0; i < 10000; i++) {
    x = 2 * -std::move(x) * BigInt{i + 1};
    // if (i % 1000 == 0) printf("%d/%d\n", i, 10000);
  }
  double took = timer.Seconds();
  printf("BenchNegate: done in %s\n",
         ANSI::Time(took).c_str());
  fflush(stdout);
}

static void TestShift() {
  int xi = -1000;

  BigInt x(-1000);

  for (int s = 0; s < 12; s++) {
    int xis = xi >> s;
    BigInt xs = x >> s;
    CHECK(xs == xis) << "at " << s << " want " << xis
                     << " got " << xs.ToString();
  }
}

static void TestAssigningOps() {

  // TODO more! I have screwed these up with copy-paste.

  {
    BigInt x(41);
    x /= 2;
    CHECK(x == 20);
  }

  {
    BigInt x(44);
    x /= BigInt{2};
    CHECK(x == 22);
  }

  {
    BigInt x(42);
    x &= BigInt{3};
    CHECK(x == 2);
  }

  {
    BigInt x(12345);
    const BigInt five(5);
    x %= five;
    CHECK(x == 0);
  }

}

static void TestBinaryOps() {
  {
    BigInt x(13310);
    CHECK((x & 717) == 716);
    CHECK((717 & x) == 716);
  }

  {
    BigInt x(13310);
    CHECK(x % 10 == 0);
    CHECK(x % 2 == 0);
  }

  {
    BigInt x(13317);
    CHECK(x % 10 == 7);
    CHECK(x % 2 == 1);
  }

}

int main(int argc, char **argv) {
  ANSI::Init();
  printf("Start.\n");
  fflush(stdout);

  BenchNegate();
  TestShift();

  TestAssigningOps();
  TestBinaryOps();

  printf("OK\n");
}

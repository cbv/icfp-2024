
#include "functional-map.h"

#include <string>

#include "base/logging.h"

static void SimpleTest() {
  using FM = FunctionalMap<std::string, int>;
  const FM empty;
  CHECK(empty.FindPtr("hi") == nullptr);

  {
    const FM fm1 = empty.Insert("hi", 3);
    CHECK(empty.FindPtr("hi") == nullptr);
    CHECK(fm1.Contains("hi") && *fm1.FindPtr("hi") == 3);
  }

  const FM fm2 = empty.Insert("hi", 4);
  const FM fm3 = fm2.Insert("hi", 5);
  const FM fm4 = fm3.Insert("bye", 6);
  const FM fm5 = fm4.Insert("hi", 7);
  const FM fm6 = fm5.Insert("hi", 8);
  const FM fm7 = fm6.Insert("hi", 9);
  const FM fm8 = fm7.Insert("bye", 10);
  const FM fm9 = fm8.Insert("hi", 11);
  const FM fm10 = fm9.Insert("hi", 12);
  const FM fm11 = fm10.Insert("hi", 13);

  CHECK(empty.FindPtr("hi") == nullptr);
  CHECK(empty.FindPtr("bye") == nullptr);
  CHECK(fm2.FindPtr("bye") == nullptr);

  CHECK(fm11.Contains("bye") && *fm11.FindPtr("bye") == 10);
  CHECK(fm10.Contains("bye") && *fm10.FindPtr("bye") == 10);
  CHECK(fm8.Contains("bye") && *fm8.FindPtr("bye") == 10);
  CHECK(fm7.Contains("bye") && *fm7.FindPtr("bye") == 6);

  CHECK(fm3.Contains("hi") && *fm3.FindPtr("hi") == 5);
  CHECK(fm11.Contains("hi") && *fm11.FindPtr("hi") == 13);
}

static void Deep() {
  using FM = FunctionalMap<int, int>;

  FM fm;
  for (int i = 0; i < 10000; i++) {
    fm = fm.Insert((i & 1) ? -i : i, i * 10000);
  }

  {
    const int *f = fm.FindPtr(0);
    CHECK(f != nullptr && *f == 0);
  }

  {
    const int *f = fm.FindPtr(9000);
    CHECK(f != nullptr && *f == 9000 * 10000);
  }
}

int main(int argc, char **argv) {

  SimpleTest();
  Deep();

  printf("OK\n");
  return 0;
}

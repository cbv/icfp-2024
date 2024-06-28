#include "tuple-util.h"

#include <list>
#include <tuple>
#include <cstdlib>

#include "base/logging.h"
#include "base/stringprintf.h"

using namespace std;


static void TestMapTuple() {
    auto t2 = MapTuple([](int i) { return StringPrintf("%d", i); },
                     std::make_tuple(5, 6, 7));

  const auto &[a, b, c] = t2;
  CHECK(a == "5");
  CHECK(b == "6");
  CHECK(c == "7");
}

int main(int argc, char **argv) {
  TestMapTuple();

  printf("OK\n");
  return 0;
}

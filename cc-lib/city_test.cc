
#include "city/city.h"
#include "base/logging.h"
#include "base/stringprintf.h"

using namespace std;

#define CHECK_EQ64(a, b) CHECK(a == b) << StringPrintf("%llx", a) \
  << " vs " << StringPrintf("%llx", b);

static void TestKnown() {
  CHECK_EQ64(CityHash64("archaeopteryx"), 0xbf8577469841d551ULL);
  CHECK_EQ64(CityHash64WithSeed("archaeopteryx", 0x12345678abababULL),
             0xc70acdb8b87d82a0ULL);
}

int main(int argc, char **argv) {

  TestKnown();

  printf("OK\n");
  return 0;
}

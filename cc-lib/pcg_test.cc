
#include "pcg.h"

#include <string>

#include "base/logging.h"

static void Basic() {
  {
    PCG32 pcg1(0);
    CHECK(pcg1.Rand32() != 0);
  }

  {
    PCG32 pcg1(0);
    PCG32 pcg2(1);
    CHECK(pcg1.Rand32() != pcg2.Rand32());
  }
}

static void StringInit() {
  PCG32 pcg1("initialized somehow");
  PCG32 pcg2("niitialized somehow");
  PCG32 pcg3("initialized someohw");
  PCG32 pcg4("initialized somehox");
  PCG32 pcg5("jnitialized somehox");

  uint32_t r1 = pcg1.Rand32();
  uint32_t r2 = pcg2.Rand32();
  uint32_t r3 = pcg3.Rand32();
  uint32_t r4 = pcg4.Rand32();
  uint32_t r5 = pcg5.Rand32();

  CHECK(r1 != r2);
  CHECK(r1 != r3);
  CHECK(r1 != r4);
  CHECK(r1 != r5);

  CHECK(r2 != r3);
  CHECK(r2 != r4);
  CHECK(r2 != r5);

  CHECK(r3 != r4);
  CHECK(r3 != r5);

  CHECK(r4 != r5);

  PCG32 again("initialized somehow");

  uint32_t ragain = again.Rand32();
  CHECK(ragain == r1);
}

static void SaveLoad() {
  PCG32 pcg(0xCAFE);

  [[maybe_unused]]
  uint32_t next1 = pcg.Rand32();

  const uint64_t s = pcg.GetState();

  uint32_t next2 = pcg.Rand32();

  PCG32 pcg2 = PCG32::FromState(s);

  uint32_t re1 = pcg2.Rand32();
  uint32_t next3 = pcg.Rand32();

  uint32_t re2 = pcg2.Rand32();
  uint32_t re3 = pcg2.Rand32();

  uint32_t next4 = pcg.Rand32();

  // next1 is lost, since we didn't save the state.
  CHECK(next2 == re1);
  CHECK(next3 == re2);
  CHECK(next4 == re3);
}

int main(int argc, char **argv) {

  Basic();
  StringInit();
  SaveLoad();

  printf("OK\n");
  return 0;
}

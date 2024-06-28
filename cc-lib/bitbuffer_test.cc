
#include "bitbuffer.h"

#include <vector>
#include <cstdint>

#include "base/logging.h"

static void CreateAndDestroy() {
  BitBuffer bb;
}

static void EZ1() {
  BitBuffer bb;
  bb.WriteBit(true);
  bb.WriteBit(false);
  bb.WriteBit(true);

  {
    CHECK(bb.NumBits() == 3);
    std::vector<uint8_t> b = bb.GetBytes();
    CHECK(b.size() == 1);
    CHECK(b[0] == 0b10100000);
  }

  bb.WriteBits(6, 0b110011);

  {
    CHECK(bb.NumBits() == 9);
    std::vector<uint8_t> b = bb.GetBytes();
    CHECK(b.size() == 2);
    CHECK(b[0] == 0b10111001);
    CHECK(b[1] == 0b10000000);
  }

  bb.WriteBits(7, 0b00110011);
  {
    CHECK(bb.NumBits() == 16);
    std::vector<uint8_t> b = bb.GetBytes();
    CHECK(b.size() == 2);
    CHECK(b[0] == 0b10111001);
    CHECK(b[1] == 0b10110011);
  }
}


int main(int argc, char **argv) {
  CreateAndDestroy();

  EZ1();

  printf("OK\n");
  return 0;
}

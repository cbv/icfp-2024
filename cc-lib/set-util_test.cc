
#include "set-util.h"

#include <unordered_set>
#include <vector>

#include "base/logging.h"
#include "ansi.h"

static void TestToSortedVec() {
  std::unordered_set<int64_t> s;
  s.insert(333);
  s.insert(444);
  s.insert(555);
  s.insert(666);

  const auto v = SetToSortedVec(s);
  CHECK(v.size() == 4);
  CHECK(v[0] == 333);
  CHECK(v[1] == 444);
  CHECK(v[2] == 555);
  CHECK(v[3] == 666);
}

int main(int argc, char **argv) {
  ANSI::Init();

  TestToSortedVec();

  printf("OK\n");
  return 0;
}

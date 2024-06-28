
#include "functional-set.h"

#include <string>

#include "base/logging.h"

static void SimpleTest() {
  using FS = FunctionalSet<std::string>;
  const FS empty;
  CHECK(!empty.Contains("hi"));

  {
    const FS fm1 = empty.Insert("hi");
    CHECK(!empty.Contains("hi"));
    CHECK(fm1.Contains("hi"));
  }

  FS fs = empty;
  for (const std::string &s : {"a", "b", "c", "d", "e",
      "f", "g", "h", "i", "j", "k"}) {
    CHECK(!fs.Contains(s));
    fs = fs.Insert(s);
    CHECK(fs.Contains(s));
  }

  CHECK(fs.Contains("a"));
  CHECK(fs.Contains("d"));
  CHECK(fs.Contains("j"));

  const auto &s = fs.Export();
  CHECK(s.contains("b"));
  CHECK(s.contains("i"));
  CHECK(s.contains("g"));
  CHECK(!s.contains("hi"));
}

int main(int argc, char **argv) {
  SimpleTest();

  printf("OK\n");
  return 0;
}


#include "base/do-not-optimize.h"

#include <string>

#include "base/logging.h"
#include "util.h"

using namespace std;

inline const char *Function(const char *str) {
  return str;
}

static std::string Reverse(std::string s) {
  char *buf = (char*)malloc(s.size());
  for (int i = 0; i < (int)s.size(); i++)
    buf[i] = s[s.size() - 1 - i];
  return std::string(buf, s.size());
}

// Not much we can do here except check that it compiles.
// But we also check that the executable itself has the
// string constant somewhere within. This of course is not
// guaranteed by the
int main(int argc, char **argv) {
  // Make sure it's longer than SSO threshold so that it's likely
  // to be in the binary as a string literal.
  DoNotOptimize(Function("this string constant is not necessary"));

  const std::string self = Util::ReadFile(argv[0]);

  // We don't want to find the search string itself, so we store
  // it in reverse, and then reverse it at runtime. But it would
  // not be that surprising if the compiler actually reversed it
  // at compile time. Check that isn't happening.
  {
    string needle = Reverse("this string does not appear in reverse");
    CHECK(self.find(needle) == string::npos) << "Need a better test "
      "method!";
  }

  {
    string needle = Reverse("yrassecen ton si tnatsnoc gnirts siht");
    CHECK(self.find(needle) != string::npos);
  }

  Function("and this one should be discarded");

  if (self.find(Reverse("dedracsid eb dluohs eno siht dna")) !=
      string::npos) {
    printf("Warning: Expected the unwrapped call to be optimized away!\n");
  }

  printf("OK\n");
  return 0;
}


#include <cstdio>
#include <cstdlib>
#include <string>

#include "util.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "ansi.h"

int main(int argc, char **argv) {
  ANSI::Init();

  CHECK(argc == 4) << "./take-improvements max-num problem dir2\n";

  const int max_num = atoi(argv[1]);

  int improved = 0;
  for (int n = 1; n <= max_num; n++) {
    std::string olde = StringPrintf("../solutions/%s/%s%d.txt",
                                    argv[2], argv[2], n);
    std::string newe = StringPrintf("../solutions/%s/%s%d.txt",
                                    argv[3], argv[2], n);

    std::string oldc =
      Util::NormalizeWhitespace(Util::ReadFile(olde));
    std::string newc =
      Util::NormalizeWhitespace(Util::ReadFile(newe));

    printf(AGREY("[%d]") " ", n);
    if (oldc.empty() && newc.empty()) {
      printf("(no solutions)\n");
    } else if (!oldc.empty() && newc.empty()) {
      printf("%d -> " AORANGE("--") "\n", (int)oldc.size());
    } else if (oldc.size() == newc.size()) {
      printf("%d -> %d\n", (int)oldc.size(), (int)newc.size());
    } else if (oldc.empty() || (oldc.size() > newc.size())) {
      printf("%d -> " AGREEN("%d") "\n", (int)oldc.size(), (int)newc.size());
      // Write over it.
      Util::WriteFile(olde, newc);
      improved++;
    } else {
      printf("%d -> " ARED("%d") "\n", (int)oldc.size(), (int)newc.size());
    }
  }

  printf("\nImproved " AGREEN("%d") "\n", improved);
  return 0;
}

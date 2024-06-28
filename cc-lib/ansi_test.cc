

#include "ansi.h"

#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

static void TestMacros() {
  printf("NORMAL" " "
         ARED("ARED") " "
         AGREY("AGREY") " "
         ABLUE("ABLUE") " "
         ACYAN("ACYAN") " "
         AYELLOW("AYELLOW") " "
         AGREEN("AGREEN") " "
         AWHITE("AWHITE") " "
         APURPLE("APURPLE") "\n");

  printf(AFGCOLOR(50, 74, 168, "Blue FG") " "
         ABGCOLOR(50, 74, 168, "Blue BG") " "
         AFGCOLOR(226, 242, 136, ABGCOLOR(50, 74, 168, "Combined")) "\n");
}

static void TestComposite() {
  std::vector<std::pair<uint32_t, int>> fgs =
    {{0xFFFFFFAA, 5},
     {0xFF00003F, 6},
     {0x123456FF, 3}};
  std::vector<std::pair<uint32_t, int>> bgs =
    {{0x333333FF, 3},
     {0xCCAA22FF, 4},
     {0xFFFFFFFF, 1}};

  printf("Composited:\n");
  for (const char *s :
         {"", "##############",
          "short", "long string that gets truncated",
             "Unic\xE2\x99\xA5" "de"}) {
    printf("%s\n", ANSI::Composite(s, fgs, bgs).c_str());
  }
}

int main(int argc, char **argv) {
  ANSI::Init();

  TestMacros();
  TestComposite();

  return 0;
}

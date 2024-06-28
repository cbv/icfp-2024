
#include "edit-distance.h"

#include <stdio.h>

#include "base/logging.h"

using namespace std;

static void TestGetAlignment() {
  auto GA = [](const string &a, const string &b) {
      return EditDistance::GetAlignment(
          a.size(), b.size(),
          // deletion from a
          [&a](int index1) {
            CHECK(index1 >= 0 && index1 < (int)a.size());
            return 1;
          },
          // insertion from b
          [&b](int index2) {
            CHECK(index2 >= 0 && index2 < (int)b.size());
            return 1;
          },
          // subst
          [&a, &b](int index1, int index2) {
            CHECK(index1 >= 0 && index1 < (int)a.size());
            CHECK(index2 >= 0 && index2 < (int)b.size());
            if (a[index1] == b[index2]) return 0;
            return 1;
          });
    };

  auto Run = [&](int expected, const string &a, const string &b) {
      auto [cmds, cost] = GA(a, b);
      if (cost != expected) {
        printf("On [%s] vs [%s]\n", a.c_str(), b.c_str());
        printf("Wrong cost: Got %d, Expected %d\n", cost, expected);
        for (const EditDistance::Command &cmd : cmds) {
          if (cmd.Delete()) {
            CHECK(cmd.index1 >= 0 &&
                  cmd.index1 < (int)a.size()) << cmd.index1;
            printf("  delete a[%d] '%c'\n", cmd.index1, a[cmd.index1]);
          } else if (cmd.Insert()) {
            CHECK(cmd.index2 >= 0 &&
                  cmd.index2 < (int)b.size()) << cmd.index2;
            printf("  insert b[%d] '%c'\n", cmd.index2, b[cmd.index2]);
          } else {
            CHECK(cmd.index1 >= 0 &&
                  cmd.index1 < (int)a.size()) << cmd.index1;
            CHECK(cmd.index2 >= 0 &&
                  cmd.index2 < (int)b.size()) << cmd.index2;
            CHECK(cmd.Subst());
            printf("  subst a[%d] '%c' with b[%d] '%c'\n",
                   cmd.index1, a[cmd.index1],
                   cmd.index2, b[cmd.index2]);
          }
        }
      }
    };

  Run(0, "", "");
  Run(0, "fistule", "fistule");
  Run(1, "fiztule", "fitule");
  Run(1, "fistule", "fiztule");
  Run(1, "fitule", "fitulxe");

  // TODO: Explicitly check commands.
  // TODO: Test with different cost functions.
}

template<class F>
static void TestDistance(const char *what, F Distance) {
  CHECK_EQ(0,
           Distance("if on a winter's night a traveler",
                    "if on a winter's night a traveler")) << what;
  CHECK_EQ(1,
           Distance("if on a wintxr's night a traveler",
                    "if on a winter's night a traveler")) << what;
  CHECK_EQ(2,
           Distance("f on a wintxr's night a traveler",
                    "if on a winter's night a traveler")) << what;
  CHECK_EQ(2,
           Distance("iff on a winter's night a traveler",
                    "if on a winter's night a travele")) << what;
  CHECK_EQ(4, Distance("zzzz", "yyyy")) << what;
  CHECK_EQ(3, Distance("kitten", "sitting")) << what;
  CHECK_EQ(3, Distance("sitting", "kitten")) << what;
}

static void TestThreshold() {
  CHECK_EQ(3, EditDistance::Ukkonen("zzzz", "yyyy", 3));
  CHECK_EQ(2, EditDistance::Ukkonen("zzzz", "yyyy", 2));
  CHECK_EQ(1, EditDistance::Ukkonen("zzzz", "yyyy", 1));

  CHECK_EQ(2,
           EditDistance::Ukkonen("iff on a winter's night a ta traveler",
                                 "if on a winter's night a travele",
                                 2));
  CHECK_EQ(2,
           EditDistance::Ukkonen("iff on a winter's night a traveler",
                                 "if on a winter's night a travele",
                                 2));
  CHECK_EQ(1,
           EditDistance::Ukkonen("iff on a winter's night a traveler",
                                 "if on a winter's night a travele",
                                 1));

}

int main(int argc, char **argv) {
  TestGetAlignment();

  TestDistance("Distance", EditDistance::Distance);
  TestDistance("Distance(b,a)", [](const string &a, const string &b) {
      return EditDistance::Distance(b, a);
    });
  TestDistance("Ukkonen (high threshold)",
               [](const string &a, const string &b) {
      // Threshold larger than maximum causes this to have the same
      // behavior as Distance().
      return EditDistance::Ukkonen(a, b, std::max(a.size(), b.size()) + 1);
    });
  TestDistance("GetAlignment",
               [](const string &a, const string &b) {
      return EditDistance::GetAlignment(
          a.size(), b.size(),
          // deletion from a
          [&a](int index1) {
            CHECK(index1 >= 0 && index1 < (int)a.size());
            return 1;
          },
          // insertion from b
          [&b](int index2) {
            CHECK(index2 >= 0 && index2 < (int)b.size());
            return 1;
          },
          // subst
          [&a, &b](int index1, int index2) {
            CHECK(index1 >= 0 && index1 < (int)a.size());
            CHECK(index2 >= 0 && index2 < (int)b.size());
            if (a[index1] == b[index2]) return 0;
            return 1;
          }).second;
    });
  TestThreshold();
  printf("OK\n");
  return 0;
}

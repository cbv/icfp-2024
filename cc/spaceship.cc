
#include "util.h"

#include <cstdio>
#include <unordered_set>
#include <cstdlib>
#include <cstdint>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "hashing.h"
#include "bounds.h"

struct Problem {
  static Problem FromFile(const std::string &filename) {
    Problem p;
    for (std::string line : Util::NormalizeLines(
             Util::ReadFileToLines(filename))) {
      int x = atoi(Util::chop(line).c_str());
      int y = atoi(Util::chop(line).c_str());

      CHECK(line.empty()) << line;
      p.stars.emplace_back(x, y);
    }
    return p;
  }

  std::vector<std::pair<int, int>> stars;

  void PrintInfo() {
    std::unordered_set<std::pair<int, int>, Hashing<std::pair<int, int>>>
      unique;
    IntBounds bounds;
    for (const auto &star : stars) {
      unique.insert(star);
      bounds.Bound(star);
    }
    printf("There are %d stars; %d distinct. %d x %d\n",
           (int)stars.size(), (int)unique.size(),
           (int)bounds.Width(), (int)bounds.Height());
  }

};

int main(int argc, char **argv) {
  CHECK(argc == 2) << "./spaceship.exe problem.txt";

  Problem p = Problem::FromFile(argv[1]);
  p.PrintInfo();

  return 0;
}

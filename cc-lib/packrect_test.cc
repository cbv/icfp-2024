
#include "packrect.h"

#include <set>
#include <vector>
#include <utility>

#include "base/logging.h"

using namespace std;

// It's not guaranteed by the interface, but if we can't pack
// one rectangle optimally, something is surely wrong!
static void TestSingleton() {
  vector<pair<int, int>> one = {{3, 4}};
  vector<pair<int, int>> pos;
  // Default settings.
  PackRect::Config config;
  int width = 999, height = 999;
  CHECK(PackRect::Pack(config, one, &width, &height, &pos));
  CHECK_EQ(height, 4);
  CHECK_EQ(width, 3);
  CHECK_EQ(pos.size(), 1);
  CHECK_EQ(pos[0].first, 0);
  CHECK_EQ(pos[0].second, 0);
}

// Should be easy to pack these as well.
static void TestPixels1Pass() {
  vector<pair<int, int>> pixels =
    {{1, 1}, {1, 1}, {1, 1}, {1, 1}};
  vector<pair<int, int>> pos;
  PackRect::Config config;
  config.budget_passes = 1;
  int width = 999, height = 999;
  CHECK(PackRect::Pack(config, pixels, &width, &height, &pos));
  CHECK_EQ(pos.size(), 4);
  for (int i = 0; i < 4; i++) {
    printf("%d,%d @ %d,%d\n",
           pixels[i].first, pixels[i].second,
           pos[i].first, pos[i].second);
  }
  CHECK_GE(height * width, 4) << "height: " << height << " width: " << width;
  std::set<int> seen;
  for (auto [x, y] : pos) {
    CHECK_LT(x, width);
    CHECK_GE(x, 0);
    CHECK_LT(y, height);
    CHECK_GE(y, 0);
    int key = x * 128 + y;
    CHECK(seen.find(key) == seen.end());
    seen.insert(key);
  }
}


// Should be able to optimally pack these if we
// do some searching on the destination size.
static void TestPixelsOpt() {
  vector<pair<int, int>> pixels =
    {{1, 1}, {1, 1}, {1, 1}, {1, 1}};
  vector<pair<int, int>> pos;
  PackRect::Config config;
  config.budget_passes = 1000;
  int width = 999, height = 999;
  CHECK(PackRect::Pack(config, pixels, &width, &height, &pos));
  CHECK_EQ(pos.size(), 4);
  for (int i = 0; i < 4; i++) {
    printf("%d,%d @ %d,%d\n",
           pixels[i].first, pixels[i].second,
           pos[i].first, pos[i].second);
  }
  CHECK_EQ(height * width, 4) << "height: " << height << " width: " << width;
  std::set<int> seen;
  for (auto [x, y] : pos) {
    CHECK_LT(x, width);
    CHECK_GE(x, 0);
    CHECK_LT(y, height);
    CHECK_GE(y, 0);
    int key = x * 128 + y;
    CHECK(seen.find(key) == seen.end());
    seen.insert(key);
  }
}


int main(int argc, char **argv) {
  TestSingleton();
  TestPixels1Pass();
  TestPixelsOpt();

  // XXX nontrivial tests!

  printf("OK\n");
  return 0;
}

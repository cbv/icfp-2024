
#include "tree-2d.h"

#include <string>

#include "base/logging.h"

static void TestInts() {
  Tree2D<int, std::string> tree;
  CHECK(tree.Size() == 0);
  CHECK(tree.Empty());

  using Pos = Tree2D<int, std::string>::Pos;

  std::vector<std::tuple<int, int, std::string>> points = {
    {0, 4, "a"},
    {-1, -1, "b"},
    {100, 300, "c"},
    {100, 301, "d"},
    {100, 299, "e"},
    {-100, 300, "f"},
    {-100, 299, "g"},
    {-100, 301, "h"},
    {-99, 300, "i"},
    {-99, 299, "j"},
    {-99, 301, "k"},
    {-101, 300, "l"},
    {-101, 299, "m"},
    {-101, 301, "n"},
  };

  for (const auto &[x, y, s] : points) {
    tree.Insert(x, y, s);
  }

  tree.DebugPrint();

  for (const auto &[x, y, s] : points) {

    for (int radius : {1, 2, 10}) {
      for (int d : {0, 1, -1}) {
        for (bool dir : {false, true}) {
          int qx = x + (dir ? d * radius : 0.0);
          int qy = y + (dir ? 0.0 : d * radius);
          std::vector<std::tuple<Pos, std::string, double>> near =
          tree.LookUp(std::make_pair(qx, qy),
                      radius);
          bool has = [&]() {
              for (const auto &[p, ss, dist] : near) {
                const auto &[xx, yy] = p;
                if (xx == x && yy == y && ss == s)
                  return true;
              }
              return false;
            }();

          CHECK(has) << "Didn't find point " << s << " with radius "
                     << radius << ". The query was (" << qx << ","
                     << qy << "). Num near: " << near.size();
        }
      }
    }

  }

  CHECK(tree.Size() == points.size());
  CHECK(!tree.Empty());

  {
    const auto &[p, s, d] = tree.Closest(std::make_pair(0, 0));
    CHECK(p == std::make_pair(-1, -1));
    CHECK(s == "b");
    CHECK(d > 1.4 && d < 1.5);
  }

  {
    const auto &[p, s, d] = tree.Closest(std::make_pair(9999, 9999));
    CHECK(p == std::make_pair(100, 301));
    CHECK(s == "d");
    CHECK(d > 9999);
  }

  for (const auto &[x, y, s] : points) {
    CHECK(tree.Remove(x, y)) << x << "," << y;
  }

  CHECK(tree.Size() == 0);
  CHECK(tree.Empty());

  // They should already be gone.
  for (const auto &[x, y, s] : points) {
    CHECK(!tree.Remove(x, y)) << x << "," << y;
  }

  CHECK(tree.Size() == 0);
  CHECK(tree.Empty());
}

int main(int argc, char **argv) {

  TestInts();

  printf("OK");
  return 0;
}

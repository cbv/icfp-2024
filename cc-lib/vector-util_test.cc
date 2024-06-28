#include "vector-util.h"

#include "base/logging.h"
#include <stdio.h>
#include <cstdint>
#include <vector>
#include <string>

using namespace std;

static void TestApp() {
  std::string s = "012345";
  std::vector<std::pair<int, char>> v = {
    {2, 'a'},
    {1, 'c'},
    {5, 'z'},
  };

  VectorApp(v, [&s](const std::pair<int, char> p) {
      CHECK(p.first >= 0 && p.first < (int)s.size());
      s[p.first] = p.second;
    });
  CHECK(s == "0ca34z") << s;
}

static void TestFilter() {
  {
    std::vector<std::string> v = {
      "hello",
      "world",
    };

    VectorFilter(&v, [](const std::string &s) { return true; });
    CHECK(v.size() == 2);
    CHECK(v[0] == "hello");
    CHECK(v[1] == "world");

    VectorFilter(&v, [](const std::string &s) { return false; });
    CHECK(v.empty());
  }

  std::vector<std::string> v = {
    "all",
    "you",
    "have",
    "to",
    "do",
    "is",
    "maybe",
    "do",
    "it",
    "yeah?",
  };

  VectorFilter(&v, [](const std::string &s) { return s.size() == 2; });
  CHECK(v.size() == 5);
  CHECK(v[0] == "to");
  CHECK(v[1] == "do");
  CHECK(v[2] == "is");
  CHECK(v[3] == "do");
  CHECK(v[4] == "it");
}

static void TestReverse() {
  std::vector<std::string> v = { "hello", "world", "yes" };
  VectorReverse(&v);
  CHECK(v[0] == "yes");
  CHECK(v[1] == "world");
  CHECK(v[2] == "hello");
}

int main(int argc, char **argv) {
  TestApp();
  TestFilter();
  TestReverse();
  printf("OK\n");
  return 0;
}


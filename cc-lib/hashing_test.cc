
#include "hashing.h"

#include <utility>
#include <tuple>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_set>

// This is mainly a test that it compiles.

using T1 = std::pair<int, std::string>;
using T2 = std::vector<uint8_t>;
using T3 = std::vector<std::pair<int, char>>;
using T4 = std::pair<int, std::vector<char>>;
using T5 = std::tuple<char, int>;
using T6 = std::tuple<std::pair<std::string, char>,
                      uint64_t, std::string>;

static void Instantiate() {
  std::unordered_set<T1, Hashing<T1>> h1;
  h1.insert(std::make_pair(5, "hi"));
  std::unordered_set<T2, Hashing<T2>> h2;
  h2.insert(std::vector<uint8_t>(3, 0x2A));
  std::unordered_set<T3, Hashing<T3>> h3;
  h3.insert(T3(3, std::make_pair(4, 'z')));
  std::unordered_set<T4, Hashing<T4>> h4;
  h4.insert(std::make_pair(4, std::vector<char>(9, 'z')));
  std::unordered_set<T5, Hashing<T5>> h5;
  h5.insert(std::make_tuple('e', 3));
  std::unordered_set<T6, Hashing<T6>> h6;
  h6.insert(std::tuple(std::make_pair("hello", 'o'),
                       1234, "hash"));
}

int main(int argc, char **argv) {
  Instantiate();

  printf("OK\n");
  return 0;
}

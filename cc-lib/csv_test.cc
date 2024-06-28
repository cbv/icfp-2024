
#include "csv.h"

#include <string>
#include <vector>
#include <cstdio>

#include "base/logging.h"

static void TestParseToVector() {
  std::vector<std::vector<std::string>> csv =
    CSV::ParseFile("test.csv");

  /*
    column one, two, three
    1,2,3
    5,,6
    incomplete line,
    "quoted, string","", nothing
  */

  for (const std::vector<std::string> &row : csv) {
    for (const std::string &field : row) {
      printf("[%s]", field.c_str());
    }
    printf("\n");
  }

  CHECK(csv.size() == 4);
  CHECK(csv[0].size() == 3);
  CHECK(csv[0][0] == "1");
  CHECK(csv[0][1] == "2");
  CHECK(csv[0][2] == "3");
  CHECK(csv[1].size() == 3);
  CHECK(csv[1][1] == "");
  CHECK(csv[2].size() == 2);
  CHECK(csv[2][0] == "incomplete line");
  CHECK(csv[2][1] == "");
  CHECK(csv[3].size() == 3);
  CHECK(csv[3][0] == "quoted, string");
  CHECK(csv[3][1] == "");
  CHECK(csv[3][2] == " nothing");
}

int main(int argc, char **argv) {

  TestParseToVector();

  printf("OK\n");
  return 0;
}

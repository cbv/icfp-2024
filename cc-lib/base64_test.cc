
#include "base64.h"

#include <string>

#include "base/logging.h"

using namespace std;

static void TestRoundTrip(const std::string &s) {
  std::string encoded = Base64::Encode(s);
  std::string decoded = Base64::Decode(encoded);
  CHECK(s == decoded) << "Round trip failed. Encoded: " << encoded;
}

static void Basic() {
  TestRoundTrip("");
  for (int c = 0; c < 256; c++) {
    string s = "_";
    s[0] = c;
    TestRoundTrip(s);
  }

  for (int c = 0; c < 65536; c++) {
    string s = "__";
    s[0] = c >> 8;
    s[1] = c & 0xFF;
    TestRoundTrip(s);
  }

  TestRoundTrip("The Quick Brown Fox");
  TestRoundTrip("The Quick Brown Fox.");
  TestRoundTrip("The Quick Brown Fox!?");
  TestRoundTrip("The Quick Brown Fox...");
}

int main(int argc, char **argv) {

  Basic();

  printf("OK\n");
  return 0;
}


#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <utility>
#include <vector>
#include <variant>
#include <string>
#include <string_view>

#include "ansi.h"
#include "timer.h"
#include "periodically.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "bignum/big.h"
#include "bignum/big-overloads.h"

#include "icfp.h"

static std::string BaseXEncode(const std::string &input) {
  // Count each char in the input. For now, we just have a
  // flat base. But I think it would be doable to perform
  // Huffman encoding.
  std::unordered_map<uint8_t, int> counts;
  for (char c : input) counts[(uint8_t)c]++;

  // Bidirectional mapping.
  std::vector<uint8_t> chars;
  int syms[256];
  for (int &i : syms) i = -1;

  for (const auto &[c, count] : counts) {
    if (count > 0) {
      syms[c] = (int)chars.size();
      chars.push_back(c);
    }
  }

  const int radix = counts.size();

  // The decoding code looks like this
  // fun emit 0 _ = ""
  //   | emit count num =
  //      let val digit = num % RADIX
  //          val rest = num / RADIX
  //      in render digit ^ emit (count - 1) rest
  //      end
  //
  // So the first character of the input string becomes
  // the lowest order digit of the number. This means
  // we will work from back to front on the input string.

  BigInt encoded{0};
  for (int i = input.size() - 1; i >= 0; i--) {
    encoded = encoded * radix;

    uint8_t c = input[i];
    CHECK(syms[c] != -1);
    encoded = encoded + syms[c];
  }

  std::string zero = icfp::IntConstant(BigInt{0});
  std::string one = icfp::IntConstant(BigInt{1});
  const std::string radix_exp = icfp::IntConstant(BigInt(radix));

  std::string raw_lookup;
  for (uint8_t c : chars) raw_lookup.push_back(c);
  std::string lookup =
    StringPrintf("S%s", icfp::EncodeString(raw_lookup).c_str());

  // y combinator
  std::string y =
    "Lf B$ Lx B$ vf B$ vx vx Lx B$ vf B$ vx vx";

  #define EMIT "e"

  // digit is num / RADIX
  std::string digit =
    StringPrintf("B%% vn %s", radix_exp.c_str());

  // The render function is just indexing into the string
  // consisting of all the chars. It drops d and then takes 1.
  std::string render_digit =
    StringPrintf("BT %s BD %s %s",
                 one.c_str(), digit.c_str(), lookup.c_str());

  // rest is num / RADIX
  std::string rest =
    StringPrintf("B/ vn %s", radix_exp.c_str());

  // render digit ^ emit (count - 1) rest
  std::string concat =
    StringPrintf("B. "
                 // render digit
                 "%s "
                 // emit (count - 1) rest
                 "B! B! v" EMIT " B- vc %s %s",
                 render_digit.c_str(),
                 one.c_str(), rest.c_str());

  std::string cond =
    StringPrintf(
        // if count = 0
        "? B= vc %s "
        // then ""
        "S "
        // else concat
        "%s",
        zero.c_str(),
        concat.c_str());

  // fix (fn emit => fn count => fn num => cond)
  std::string fix =
    StringPrintf("B$ %s L" EMIT " Lc Ln %s", y.c_str(), cond.c_str());

  // And apply to the arguments.

  std::string all =
    StringPrintf("B$ B$ %s %s %s",
                 fix.c_str(),
                 icfp::IntConstant(BigInt(input.size())).c_str(),
                 icfp::IntConstant(encoded).c_str());

  return all;
}

int main(int argc, char **argv) {
#if 0
  int max_len = -1;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-max-len") {
      CHECK(i + 1 < argc);
      i++;
      max_len = atoi(argv[i++]);
    } else {
      printf("./compress.exe [-max-len n] < file.txt > file.icfp\n"
             "\n"
             "max-len gives the maximum string length to try\n"
             "factoring out. For big files, setting this much\n"
             "smaller will make it much faster!\n");
    }
  }
#endif

  std::string input = icfp::ReadAllInput();

  std::string output = BaseXEncode(input);
  printf("%s\n", output.c_str());

  return 0;
}

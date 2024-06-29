
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <string>

#include "ansi.h"
#include "timer.h"
#include "periodically.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "bignum/big.h"
#include "bignum/big-overloads.h"

#include "icfp.h"

static std::string BaseXEncode(std::string_view input) {
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

  fprintf(stderr, "%d distinct chars\n\n", radix);
  for (const auto &[c, count] : counts) {
    if (count > 0) {
      fprintf(stderr, "'%c' x %d\n", c, count);
    }
  }

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


  Periodically status_per(1.0);
  Timer timer;

  BigInt encoded{0};
  for (int i = input.size() - 1; i >= 0; i--) {
    uint8_t c = input[i];
    CHECK(syms[c] != -1);
    encoded = encoded * radix + syms[c];
    // encoded = encoded + syms[c];

    if (status_per.ShouldRun()) {
      fprintf(stderr,
              ANSI_UP "%s\n",
              ANSI::ProgressBar(input.size() - i, input.size(),
                                "Encoding",
                                timer.Seconds()).c_str());
    }
  }

  fprintf(stderr, "Generate constants...\n");

  std::string zero = icfp::IntConstant(BigInt{0});
  std::string one = icfp::IntConstant(BigInt{1});
  const std::string radix_exp = icfp::IntConstant(BigInt(radix));

  const std::string encoded_exp = icfp::IntConstant(encoded);

  fprintf(stderr, "Constant is %d bytes. Output decoder...\n",
          (int)encoded_exp.size());

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
                 encoded_exp.c_str());

  return all;
}

int main(int argc, char **argv) {
  std::string prefix;
  bool force_pow2 = false;
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-prefix") {
      CHECK(i + 1 < argc);
      i++;
      prefix = argv[i++];
    } else if (std::string(argv[i]) == "-pow2") {
      force_pow2 = true;
    } else {
      fprintf(stderr,
              "./encode.exe [-prefix \"message\"] [-pow2] "
              "< file.txt > file.icfp\n");
      return -1;
    }
  }

  std::string input = icfp::ReadAllInput();

  CHECK(input.find(prefix) == 0) << "Input must start with exactly the "
    "prefix.";

  std::string_view input_view(input);
  input_view.remove_prefix(prefix.size());

  std::string output = BaseXEncode(input_view);

  if (prefix.empty()) {
    printf("%s\n", output.c_str());
  } else {
    printf("B. S%s %s\n",
           icfp::EncodeString(prefix).c_str(),
           output.c_str());
  }

  return 0;
}

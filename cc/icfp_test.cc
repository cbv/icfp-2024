
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include "icfp.h"

#include "util.h"
#include "ansi.h"
#include "base/logging.h"

#if NO_BIGNUM
#error sorry
#endif

#include "bignum/big.h"
#include "bignum/big-overloads.h"

using namespace icfp;

static void TestInt() {

  for (std::string s : {"0", "1", "93", "94", "95",
      "1028734981723498751923847512",
      "9999999999999999999999999999999999999",
      "1122334455666888990000000000000000000000000000000000000000"}) {
    BigInt b{s};

    std::string enc = IntConstant(b);
    std::string_view enc_view(enc);
    std::shared_ptr<Exp> exp = ParseLeadingExp(&enc_view);
    CHECK(enc_view.empty());
    CHECK(exp.get());

    Evaluation evaluation;
    Value v = evaluation.Eval(exp.get());

    const Int *i = std::get_if<Int>(&v);
    CHECK(i != nullptr) << ValueString(v);

    CHECK(i->i == b) << b.ToString();
  }

}


int main(int argc, char **argv) {
  ANSI::Init();

  TestInt();

  printf("OK");
  return 0;
}

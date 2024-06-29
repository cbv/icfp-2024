
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include "icfp.h"

#include "ansi.h"
#include "base/logging.h"
#include "timer.h"

#include "bignum/big.h"
#include "bignum/big-overloads.h"

using namespace icfp;

static Value Evaluate(std::string_view s) {
  Parser parser;
  std::shared_ptr<Exp> exp = parser.ParseLeadingExp(&s);
  CHECK(s.empty());
  CHECK(exp.get());

  Evaluation evaluation;
  return evaluation.Eval(exp);
}

static void TestInt() {

  for (std::string s : {"0", "1", "93", "94", "95",
      "1028734981723498751923847512",
      "9999999999999999999999999999999999999",
      "1122334455666888990000000000000000000000000000000000000000"}) {
    BigInt b{s};

    std::string enc = IntConstant(b);
    Value v = Evaluate(enc);
    const Int *i = std::get_if<Int>(&v);
    CHECK(i != nullptr) << ValueString(v);

    CHECK(i->i == b) << b.ToString();
  }

}

static void LanguageTest() {
  constexpr const char *test = R"(? B= B$ B$ B$ B$ L$ L$ L$ L# v$ I" I# I$ I% I$ ? B= B$ L$ v$ I+ I+ ? B= BD I$ S4%34 S4 ? B= BT I$ S4%34 S4%3 ? B= B. S4% S34 S4%34 ? U! B& T F ? B& T T ? U! B| F F ? B| F T ? B< U- I$ U- I# ? B> I$ I# ? B= U- I" B% U- I$ I# ? B= I" B% I( I$ ? B= U- I" B/ U- I$ I# ? B= I# B/ I( I$ ? B= I' B* I# I$ ? B= I$ B+ I" I# ? B= U$ I4%34 S4%34 ? B= U# S4%34 I4%34 ? U! F ? B= U- I$ B- I# I& ? B= I$ B- I& I# ? B= S4%34 S4%34 ? B= F F ? B= I$ I$ ? T B. B. SM%,&k#(%#+}IEj}3%.$}z3/,6%},!.'5!'%y4%34} U$ B+ I# B* I$> I1~s:U@ Sz}4/}#,!)-}0/).43}&/2})4 S)&})3}./4}#/22%#4 S").!29}q})3}./4}#/22%#4 S").!29}q})3}./4}#/22%#4 S").!29}q})3}./4}#/22%#4 S").!29}k})3}./4}#/22%#4 S5.!29}k})3}./4}#/22%#4 S5.!29}_})3}./4}#/22%#4 S5.!29}a})3}./4}#/22%#4 S5.!29}b})3}./4}#/22%#4 S").!29}i})3}./4}#/22%#4 S").!29}h})3}./4}#/22%#4 S").!29}m})3}./4}#/22%#4 S").!29}m})3}./4}#/22%#4 S").!29}c})3}./4}#/22%#4 S").!29}c})3}./4}#/22%#4 S").!29}r})3}./4}#/22%#4 S").!29}p})3}./4}#/22%#4 S").!29}{})3}./4}#/22%#4 S").!29}{})3}./4}#/22%#4 S").!29}d})3}./4}#/22%#4 S").!29}d})3}./4}#/22%#4 S").!29}l})3}./4}#/22%#4 S").!29}N})3}./4}#/22%#4 S").!29}>})3}./4}#/22%#4 S!00,)#!4)/.})3}./4}#/22%#4 S!00,)#!4)/.})3}./4}#/22%#4)";

  Value v = Evaluate(test);
  const String *s = std::get_if<String>(&v);
  CHECK(s != nullptr) << ValueString(v);
  CHECK(s->s ==
        "Self-check OK, send `solve language_test 4w3s0m3` "
        "to claim points for it") << "Got:\n" << s->s;
}

static void Bench() {
  constexpr const char *david = R"(B. S3/,6%},!-"$!-!.Y} B$ B$ B$ Lf B$ Lx B$ vf B$ vx vx Lx B$ vf B$ vx vx LS Ln Lr ? B= vn I'E S B. B$ Lx ? B= I! vx SO ? B= I" vx S> ? B= I# vx SF SL B% B/ vr I.gg~B I% B$ B$ vS B+ vn I" B% B+ B* vr I#!Dd I-}c|. IX""|J I! I!)";

  Timer timer;
  Value v = Evaluate(david);
  const String *s = std::get_if<String>(&v);
  CHECK(s != nullptr) << ValueString(v);
  CHECK(s->s ==
        "solve lambdaman4 UUDULDLDUDUULURRDRDDRLDLLLDLLLURULLLLUULDLLLLURRRLRUDDDDURRDRRULDURUUULURURDRRLLURLDLDULDUUDUDDDDDRDUUURDLDRDLULDLULLRRDRLUURDLLDLLDURRDLLLRLLLRDLDRLULUDRLUUURURRRLDDDULLDDRRDUDRDDUDLLLRRURRUURURDURRDDLRDLDDURDRULULDLDDDDLLDDRDUUDDUULRRRDDDDRULUDLDRRRDRRLUULURDRLUURDRLRUULDDURULRRLRLDDRLDDLDUDLRDUULURLURUDLULURDUULULRRLUUULLURRDUDULDDLLLDUURRDRURLUDDUURDDURLDUULRRRLLDRLUDULUUDDULRULLLLDURDULRDURUDLDDURLDLLDRUDURUDURDULLDRLLRLRRDUULDRLUUUDLLDLLRRURDRLUDRULUUDDDLLDURRUDRDLUDLUDRRUURLLLURDUDLDLLLLDRLDUDULLULRRRLDRLRDDDLLDRLDULLUDRRULULRRDDUDRDULUURDLUULDLRDUULLDRDURLURDRURUURRRRDDLLURRLLLURLUUUUL") << s->s;
  printf("Benchmark ran in %s\n", ANSI::Time(timer.Seconds()).c_str());
}

static void Crash() {
  constexpr const char *crash = R"(B. S3/,6%},!-"$!-!.Y} B$ B$ B$ Lf B$ Lx B$ vf B$ vx vx Lx B$ vf B$ vx vx LS Ln Lr ? B= vn I"-E S B. B$ Lx ? B= I! vx SO ? B= I" vx S> ? B= I# vx SF SL B% B/ vr I.gg~B I% B$ B$ vS B+ vn I" B% B+ B* vr I#!Dd I-}c|. IX""|J I! I!)";

  Value v = Evaluate(crash);
  const String *s = std::get_if<String>(&v);
  CHECK(s != nullptr) << ValueString(v);
  CHECK(s->s == "??") << "Got:\n" << s->s;
}

int main(int argc, char **argv) {
  ANSI::Init();

  TestInt();
  LanguageTest();

  Bench();

  Crash();

  printf("OK");
  return 0;
}

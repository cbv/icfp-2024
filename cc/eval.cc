
#include "icfp.h"

#include <string>
#include <string_view>
#include <cassert>
#include <cstdio>
#include <memory>

using namespace icfp;

int main(int argc, char **argv) {
  std::string input = ReadAllInput();
  std::string_view input_view(input);

  std::shared_ptr<Exp> exp = ParseLeadingExp(&input_view);

  assert(input_view.empty() && "extra stuff after expression?");

  Evaluation evaluation;
  Value v = evaluation.Eval(exp.get());

  printf("%s\n", ValueString(v).c_str());
  return 0;
}

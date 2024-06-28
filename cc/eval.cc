
#include "icfp.h"

#include <string>
#include <string_view>
#include <cassert>
#include <cstdio>
#include <memory>

using namespace icfp;

int main(int argc, char **argv) {
  std::string input;
  char c;
  while (EOF != (c = fgetc(stdin))) {
    input.push_back(c);
  }

  std::string_view input_view(input);

  // Strip leading and trailing whitespace.
  while (!input_view.empty() && (input_view[0] == ' ' || input_view[0] == '\n'))
    input_view.remove_prefix(1);
  while (!input_view.empty() && (input_view.back() == ' ' || input_view.back() == '\n'))
    input_view.remove_suffix(1);

  std::shared_ptr<Exp> exp = ParseLeadingExp(&input_view);

  assert(input_view.empty() && "extra stuff after expression?");

  Evaluation evaluation;
  Value v = evaluation.Eval(exp.get());

  printf("%s\n", ValueString(v).c_str());
  return 0;
}

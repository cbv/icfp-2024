
#include "icfp.h"

#include <string>
#include <string_view>
#include <cstdio>
#include <memory>

#include "base/logging.h"

using namespace icfp;

int main(int argc, char **argv) {
  std::string input = ReadAllInput();
  std::string_view input_view(input);

  std::shared_ptr<Exp> exp = ParseLeadingExp(&input_view);

  CHECK(input_view.empty()) << "extra stuff after expression:\n"
                            << input_view
                            << "\nI parsed:\n"
                            << (exp.get() == nullptr ? "nullptr" :
                                PrettyExp(exp.get()));

  Evaluation evaluation;
  Value v = evaluation.Eval(exp.get());

  printf("%s\n", ValueString(v).c_str());
  return 0;
}

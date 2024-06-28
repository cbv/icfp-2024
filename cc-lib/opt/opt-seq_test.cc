#include "opt-seq.h"

#include <cstdio>
#include <vector>
#include <cmath>

#include "base/logging.h"

static void CreateAndDestroy() {
  OptSeq seq({{-1.0, +1.0}});
}

static void SimpleMinimize() {
  OptSeq seq({{-1.0, +1.0}});

  for (int i = 0; i < 10000; i++) {
    std::vector<double> arg = seq.Next();
    CHECK(arg.size() == 1);

    const double x = arg[0];

    const double a = sin(3 * x - 1.0);
    seq.Result(a * a + sin(x));
  }

  auto bo = seq.GetBest();
  CHECK(bo.has_value());
  CHECK(bo.value().first.size() == 1);
  double best_x = bo.value().first[0];
  double best_y = bo.value().second;

  // Minimum is about -0.754742,
  // with a value of about -0.670137
  CHECK(best_x > -0.755 && best_x < -0.754) << best_x;
  CHECK(best_y > -0.671 && best_y < -0.669) << best_y;
}

int main(int argc, char **argv) {
  CreateAndDestroy();

  SimpleMinimize();

  printf("OK\n");
  return 0;
}

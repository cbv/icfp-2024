#include <stdio.h>
#include <cstdint>

#include <math.h>

#include "opt/optimizer.h"
#include "base/logging.h"

using namespace std;

// Two integral arguments, 0 doubles, "output type" is int.
using CircleOptimizer = Optimizer<2, 0, int>;

static pair<double, optional<int>>
DiscreteDistance(CircleOptimizer::arg_type arg) {
  auto [x, y] = arg.first;

  int sqdist = (x * x) + (y * y);
  if (sqdist >= 10 * 10)
    return CircleOptimizer::INFEASIBLE;
  return make_pair(-sqrt(sqdist), make_optional(sqdist));
}

static void CircleTest() {
  CircleOptimizer optimizer(DiscreteDistance);

  CHECK(!optimizer.GetBest().has_value());

  optimizer.SetBest({{0, 0}, {}}, 0.0, 0);

  CHECK(optimizer.GetBest().has_value());

  // Up to 900 calls.
  optimizer.Run({make_pair(-20, 20),
                 make_pair(-20, 20)}, {},
                {900}, nullopt, nullopt, nullopt);

  auto best = optimizer.GetBest();
  CHECK(best.has_value());

  auto [best_arg, best_score, best_sqdist] = best.value();
  auto [best_x, best_y] = best_arg.first;
  printf("Best (%d,%d) score %.3f sqdist %d\n",
         best_x, best_y,
         best_score, best_sqdist);

  // Should be able to find one of the corners.
  CHECK(best_sqdist == 98);
  CHECK(best_x == 7 || best_x == -7);
  CHECK(best_y == 7 || best_y == -7);
  CHECK(best_score < -9.898);

  printf("Circle OK.\n");
}

// Should be able to find an exact vector of integers.
static void ExactIntTest() {
  static constexpr std::array ints = {
    (int32_t)1, 2, 3, 4,
    -1, -2, -3, -4,
    -314159, -2653589, 0, -141421356,
    314159, 2653589, 0, 141421356,
    2, 4, 8, 16,
    32, 64, 128, 256,
    512, 1024, 2048, 4096,
    -3, -5, -3, -7,
    11011011, 22022022, -33033033, -44044044,
    8675309,8675309,18000000,18000000,
    -9999999, -888888, -7777, -666,
    -1582, 34971, -459273, -111511,
    0, 0, 0, 0,
    2, 8, 4, 27,
    -666666666, 3, 1, 3,
    0, 300000000, 0, 7,
  };

  using ExactIntOptimizer = Optimizer<ints.size(), 0, int>;
  ExactIntOptimizer::function_type OptimizeMe =
    [](const ExactIntOptimizer::arg_type &arg) ->
    ExactIntOptimizer::return_type {
      const auto &[intargs, doubles_] = arg;
      double sqdiff = 0.0;
      CHECK(intargs.size() == ints.size());
      for (int i = 0; i < intargs.size(); i++) {
        double d = intargs[i] - ints[i];
        sqdiff += (d * d);
      }
      return std::make_pair(sqdiff, std::make_optional(0));
    };

  ExactIntOptimizer optimizer(OptimizeMe);

  array<pair<int32_t, int32_t>, ints.size()> int_bounds;
  for (int i = 0; i < ints.size(); i++) {
    int_bounds[i] = make_pair(-0x7FFFFFFE, +0x7FFFFFFE);
  }

  optimizer.Run(int_bounds, {},
                {8192 * ints.size()}, nullopt, nullopt, nullopt);

  CHECK(optimizer.GetBest().has_value());
  const auto [bestarg, score, bestout_] =
    optimizer.GetBest().value();
  const auto &bestints = bestarg.first;
  CHECK(bestints.size() == ints.size());
  if (false) {
    printf("Score: %.4f\n", score);
    for (int i = 0; i < bestints.size(); i++) {
      printf("%d. %d\n", i, bestints[i]);
    }
  }
  for (int i = 0; i < bestints.size(); i++) {
    CHECK(bestints[i] == ints[i]) << i << ". Got "
                                  << bestints[i]
                                  << " want "
                                  << ints[i];
  }

  printf("Exact int OK.\n");
}

static void ExplainTest() {

  // Use an actual linear function.

  using LinearOptimizer = Optimizer<3, 2, char>;
  LinearOptimizer::function_type OptimizeMe =
    [](const LinearOptimizer::arg_type &arg) ->
    LinearOptimizer::return_type {
      const auto &[intargs, doubleargs] = arg;
      const auto &[a, b, c] = intargs;
      const auto &[x, y] = doubleargs;

      // bias
      double v = 7.0;
      // categorical
      switch (b) {
      case 0:
        v += 100.0;
        break;
      case 1:
        v += -3.0;
        break;
      case 2:
        v += 110.0;
        break;
      default:
        CHECK(false) << "Should never call outside of gamut: " << b;
      }

      v += a * 31.7;
      v += c * -3.3;

      v += x * -0.98;
      v += y * 5.4;

      return std::make_pair(v, std::make_optional('x'));
    };

  LinearOptimizer optimizer(OptimizeMe);
  optimizer.SetSaveAll(true);

  const std::array<std::pair<int, int>, 3> int_bounds = {
    // This should find -25, but it finds -24?
    make_pair(-25, 26),
    make_pair(0, 3),
    make_pair(-100, 3),
  };

  const std::array<std::pair<double, double>, 2> double_bounds = {
    make_pair(-2.0, 2.0),
    make_pair(-4.0, 4.0),
  };

  optimizer.Run(int_bounds, double_bounds,
                {512}, nullopt, nullopt, nullopt);

  CHECK(optimizer.GetBest().has_value());
  const auto [bestarg, score, bestout_] =
    optimizer.GetBest().value();
  const auto &bestints = bestarg.first;
  const auto &bestdoubles = bestarg.second;
  CHECK(bestints.size() == int_bounds.size());
  CHECK(bestdoubles.size() == double_bounds.size());
  if (true) {
    printf("Score: %.4f\n", score);
    const auto &[a, b, c] = bestints;
    const auto &[x, y] = bestdoubles;

    printf("a=%d, b=%d, c=%d\n"
           "x=%.4f, y=%.4f\n",
           a, b, c,
           x, y);
  }

  printf("\nNow explain...\n");

  // Explain a data set.
  auto Explain = [&](
      const std::string &name,
      const std::vector<std::tuple<LinearOptimizer::arg_type,
                                   double,
                                   std::optional<char>>> &datapoints) {

      const auto &[features, loss] =
        optimizer.Explain(
            datapoints,
            std::array<LinearOptimizer::IntFeature, 3>{
              LinearOptimizer::IntFeature{
                .name = "A",
                .categorical = false,
              },
              LinearOptimizer::IntFeature{
                .name = "B",
                .categorical = true,
              },
              LinearOptimizer::IntFeature{
                .name = "C",
                .categorical = false,
              }
            },
            std::array<LinearOptimizer::DoubleFeature, 2>{
              LinearOptimizer::DoubleFeature{
                .name = "X",
              },
              LinearOptimizer::DoubleFeature{
                .name = "Y",
              },
            },
            // Don't use a bias parameter when there is categorical
            // data.
            std::nullopt
            /* std::make_optional("BIAS") */);

      printf("Loss: %.4f\n", loss);
      for (const LinearOptimizer::Feature &f : features) {
        if (f.type == LinearOptimizer::FeatureType::CATEGORICAL_INT) {
          printf("%s=%d: %.4f\n", f.name.c_str(), f.categorical_value, f.coefficient);
        } else {
          printf("%s: %.4f\n", f.name.c_str(), f.coefficient);
        }
      }

      printf("You gotta check it manually ^\n");
    };

  Explain("all", optimizer.GetAll());

  printf("Explore locally...\n");

  auto expt =
  optimizer.ExploreLocally(
      // categorical?
      std::array<bool, 3>{false, true, false},
      int_bounds,
      double_bounds,
      0.1);

  Explain("expt", expt);
}


int main(int argc, char **argv) {
  CircleTest();
  ExactIntTest();
  ExplainTest();

  printf("OK\n");
  return 0;
}


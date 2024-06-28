#include "large-optimizer.h"

#include <cinttypes>
#include <utility>
#include <optional>
#include <vector>

#include "opt.h"
#include "base/logging.h"
#include "timer.h"
#include "periodically.h"
#include "arcfour.h"
#include "randutil.h"

using namespace std;

static std::pair<double, bool> OnlyIntegers(
    const std::vector<double> &args) {
  CHECK(args.size() == 1);
  double d = args[0];

  CHECK(d == round(d)) << "Should only be called on integers";

  switch ((int)d) {
  case 0: return make_pair(-1, true);
  case 1: return make_pair(0, true);
  case 2: return make_pair(1, true);
  case 3:
  default:
    CHECK(false) << "Did not expect a call with d=" << d
                 << "; this includes 3, the open upper bound.";
    return make_pair(0, true);
  }
}

// Fairly easy optimization problem. Neighboring
// parameters should be close together; sum should
// be low. Best solution is all zeroes.

static std::pair<double, bool> F1(const std::vector<double> &args) {
#if 0
  printf("Call F1:");
  for (double d : args) printf(" %.3f", d);
  printf("\n");
#endif

  for (double d : args) {
    CHECK(std::isfinite(d));
    CHECK(d >= -100.0 && d <= 100.0) << d;
  }

  double sum = 0.0;
  for (double d : args) sum += abs(d);

  double neighbors = 0.0;
  for (int i = 1; i < args.size(); i++) {
    double d = args[i] - args[i - 1];
    neighbors += sqrt(d * d);
  }

#if 0
  printf("Got %.5f + %.5f\n", sum, neighbors);
#endif
  return make_pair(sum + neighbors, true);
}

[[maybe_unused]]
static void SelfOptimize(int n) {
  static constexpr bool CACHE = false;

  int64_t total_calls = 0;
  Periodically spam_per(10.0);

  const auto [arg, score] =
  Opt::Minimize1D(
      [n, &total_calls, &spam_per](double ppp) {
        Timer run_timer;
        using Optimizer = LargeOptimizer<CACHE>;
        Optimizer opt([&total_calls, &spam_per](
            const std::vector<double> v) {
            total_calls++;
            if (spam_per.ShouldRun()) {
              printf("%" PRIi64 " calls\n", total_calls);
            }
            return F1(v);
          }, n, 0);

        // Needs a starting point that's feasible. Everything
        // is feasible for this problem.
        std::vector<double> example(n);
        for (int i = 0; i < n; i++) {
          example[i] = (((i ^ 97 + n) * 31337) % 200) - 100;
        }
        opt.Sample(example);

        std::vector<typename Optimizer::arginfo> arginfos(n);
        for (int i = 0; i < n; i++)
          arginfos[i] = Optimizer::Double(-100.0, 100.0);
        // Run for max 10 seconds because I already know settings that
        // take ~6.
        opt.Run(arginfos, {1000000}, nullopt, {10.0}, {0.01},
                // This is the hyper-parameter being optimized.
                (int)round(ppp));

        auto besto = opt.GetBest();
        CHECK(besto.has_value());
        const auto &v = besto.value().first;

        // Infeasible if it didn't solve it!
        for (int i = 0; i < v.size(); i++) {
          double d = v[i];
          if (!(d >= -0.01 && d <= 0.01))
            return 10000000000.0;
        }

        // Otherwise, faster wall-time is better.
        double sec = run_timer.Seconds();
        printf("With ppp=%d, %.3fs\n", (int)round(ppp), sec);
        return run_timer.Seconds();
      },
      1.0, std::max(1.0, std::min(1000.0, (double)n)), 10);

  printf("Best is %.3f taking %.3fs\n", arg, score);
}

template<bool CACHE = true>
static void OptF1(int n) {
  Timer run_timer;
  using Optimizer = LargeOptimizer<CACHE>;
  Optimizer opt([n](const std::vector<double> v) {
      CHECK(v.size() == n);
      return F1(v);
    }, n, 0);

  // Needs a starting point that's feasible. Everything
  // is feasible for this problem.
  std::vector<double> example(n);
  for (int i = 0; i < n; i++) {
    example[i] = (((i ^ 97 + n) * 31337) % 200) - 100;
  }
  opt.Sample(example);

  std::vector<typename Optimizer::arginfo> arginfos(n);
  for (int i = 0; i < n; i++)
    arginfos[i] = Optimizer::Double(-100.0, 100.0);
  opt.Run(arginfos, {1000000}, nullopt, nullopt, {0.01});

  auto besto = opt.GetBest();
  CHECK(besto.has_value());
  const auto &v = besto.value().first;
  CHECK(v.size() == n) << v.size() << " vs " << n;
  printf("Best (score %.3f):", besto.value().second);
  for (int i = 0; i < v.size(); i++) {
    if (i < 20) {
      double d = v[i];
      printf(" %.3f", d);
    } else {
      printf(" ...");
      break;
    }
  }
  printf("\n");

  for (int i = 0; i < v.size(); i++) {
    double d = v[i];
    CHECK(d >= -0.01 && d <= 0.01) << "#" << i << ": " << d;
  }
  printf("OK with %d param(s) in %.3fs\n",
         n, run_timer.Seconds());
}

template<bool CACHE>
static void OptIntegers() {
  using Optimizer = LargeOptimizer<CACHE>;
  {
    Optimizer opt(OnlyIntegers, 1, 0);
    std::vector<typename Optimizer::arginfo> arginfos = {
      Optimizer::Integer(0, 3),
    };
    opt.Sample({1.0});
    opt.Run(arginfos, nullopt, nullopt, {0.5});
    auto besto = opt.GetBest();
    CHECK(besto.has_value());
    CHECK(besto.value().first.size() == 1);
    // If it can't find the best of three values in 0.5 sec, something
    // is wrong!
    CHECK(besto.value().first[0] == 0.0);
  }

  // Same but excluding 0, which would otherwise be the best
  // solution.
  {
    Optimizer opt(OnlyIntegers, 1, 0);
    std::vector<typename Optimizer::arginfo> arginfos = {
      Optimizer::Integer(1, 3),
    };
    opt.Sample({1.0});
    opt.Run(arginfos, nullopt, nullopt, {0.5});
    auto besto = opt.GetBest();
    CHECK(besto.has_value());
    CHECK(besto.value().first.size() == 1);
    // If it can't find the best of two values in 0.5 sec, something
    // is wrong!
    CHECK(besto.value().first[0] == 1.0);
  }
}

template<bool CACHE>
static void OptDoubles() {
  using Optimizer = LargeOptimizer<CACHE>;

  {
    Optimizer opt([](const std::vector<double> &v) {
        CHECK(v.size() == 3);
        // printf("%.3f %.3f %.3f\n", v[0], v[1], v[2]);
        return std::make_pair(abs(v[1] - 3.141592653589), true);
      }, 3, 0);
    std::vector<typename Optimizer::arginfo> arginfos = {
      Optimizer::Double(-100, -90),
      Optimizer::Double(-5, 5),
      Optimizer::Double(90, 100),
    };
    opt.Sample({-99.0, 0.0, 91.0});
    // Optimize one arg at a time. Expect to eventually optimize
    // the middle parameter.
    opt.Run(arginfos, {1000000}, nullopt, {4}, {0.0005}, 1);
    auto besto = opt.GetBest();
    CHECK(besto.has_value());
    CHECK(besto.value().first.size() == 3);
    const double res = besto.value().first[1];
    const double diff = res - 3.141592653589;
    CHECK(abs(diff) < 0.001) << res << " " << diff;
  }
}

std::pair<double, bool> LineFunction(const std::vector<double> &v) {
  double err = 0.0;
  for (int i = 0; i < v.size(); i++) {
    double diff = v[i] - (double)i;
    err += diff * diff;
  }
  return std::make_pair(sqrt(err), true);
}

// Test an easy function but with local optimization limited
// (+/- 2 at each step). It should still eventually converge.
template<bool CACHE>
static void JitterTo() {
  using Optimizer = LargeOptimizer<CACHE>;

  {
    const int N = 30;
    Optimizer opt(LineFunction, N, 0);
    std::vector<typename Optimizer::arginfo> arginfos;
    std::vector<double> sample;
    for (int i = 0; i < N; i++) {
      arginfos.push_back(
          Optimizer::Double(-100, +100, -2.0, +2.0));
      sample.push_back(0.0);
    }
    opt.Sample(sample);
    opt.Run(arginfos, {1000000}, nullopt, {4}, {0.0005}, 10);
    auto besto = opt.GetBest();
    CHECK(besto.has_value());
    CHECK(besto.value().first.size() == N);
    double total_err = 0.0;
    for (int i = 0; i < N; i++) {
      const double diff = abs(besto.value().first[i] - (double)i);
      total_err += diff;
    }
    CHECK(abs(total_err) < 0.1) << total_err;
  }

}

// This function is flat everywhere except very close to the
// solution. Tests whether the down/up constraints on the
// initial solution (which is close) are honored.
std::pair<double, bool> Flat(const std::vector<double> &v) {

  // Target is a bunch of tens.
  double err = 0.0;
  for (double d : v) {
    if (d < 9.5 || d > 10.5) err += 100000.0;
    err += abs(d - 10.0);
  }
  return std::make_pair(err, true);
}

// TODO: Same for doubles?
template<bool CACHE>
static void JitterInt() {
  using Optimizer = LargeOptimizer<CACHE>;
  ArcFour rc("jitter");

  {
    const int N = 30;
    Optimizer opt(Flat, N, 0);
    std::vector<typename Optimizer::arginfo> arginfos;
    std::vector<double> sample;
    for (int i = 0; i < N; i++) {
      arginfos.push_back(
          Optimizer::Integer(-10000000, +80000000, -5.0, +5.0));
      // A value close to but not equal to 10.
      int x = 0;
      do {
        x = RandTo(&rc, 8) - 4;
      } while (x == 10);
      sample.push_back(x);
    }
    opt.Sample(sample);
    opt.Run(arginfos, {1000000}, nullopt, {4}, {0.0005}, 10);
    auto besto = opt.GetBest();
    CHECK(besto.has_value());
    CHECK(besto.value().first.size() == N);
    // Expect to solve it exactly on integers.
    for (int i = 0; i < N; i++) {
      double v = besto.value().first[i];
      CHECK((int)round(v) == 10) << i << ": " << v;
    }
  }

}

int main(int argc, char **argv) {

  OptIntegers<false>();
  OptIntegers<true>();

  OptDoubles<false>();
  OptDoubles<true>();

  JitterTo<false>();
  JitterTo<true>();

  JitterInt<false>();
  JitterInt<true>();

  // Make sure it doesn't fail if we have fewer parameters
  // than the internal sample size, for example.
  OptF1(1);

  // And make sure the function itself is sound...
  OptF1(5);

  // Small problem with more parameters than the round size.
  OptF1<true>(10);
  // ... and with cache disabled.
  OptF1<false>(10);

  OptF1<false>(1000);

  // Best is 26.
  // SelfOptimize(100);

  // Best is 21.
  // SelfOptimize(1000);

  return 0;
}

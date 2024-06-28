
#ifndef _CC_LIB_OPT_LARGE_OPTIMIZER_H
#define _CC_LIB_OPT_LARGE_OPTIMIZER_H

// Wrapper around black-box optimization, for optimization
// problems with a large number of parameters.
//
// This optimizes a subset of the parameters using the black-box
// optimizer, greedily takes any improvement, and repeats. Since it
// does not restart, it can easily get stuck in local minima. Since
// there are many possible subsets of parameters, it may not find
// an improvement even when a local one does exist. Therefore this
// is best suited to fairly easy convex problems.

#include <cstdio>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <unordered_map>
#include <utility>
#include <cassert>
#include <variant>

#include "opt/opt.h"

template<bool CACHE = true>
struct LargeOptimizer {

  // Convenience constant for inputs where the function cannot even be
  // evaluated. However, optimization will be more efficient if you
  // instead return a penalty that exceeds any feasible score, and has
  // a gradient towards the feasible region.
  static inline constexpr double LARGE_SCORE =
    std::numeric_limits<double>::max();

  // The first component is the score; lower is better.
  // The second parameter is true if this argument is feasible;
  // GetBest will only consider such arguments.
  using return_type = std::pair<double, bool>;

  using arg_type = std::vector<double>;

  // function to optimize.
  using function_type =
    std::function<return_type(const std::vector<double> &)>;

  LargeOptimizer(function_type f,
                 // Number of arguments.
                 int n,
                 uint64_t start_seed = 1);

  // Each argument needs to be described for the search procedure.
  // We have both the absolute lower and upper bounds for the argument
  // (the function being optimized is only called with values in that
  // range) and the downward and upward search limits (in each optimization
  // pass, we consider only values this far from the current best). The
  // default for the search limits optimize within the entire bounds.
  // Although we use extreme defaults here, it will not work well to
  // use extremely large double bounds, as internally the optimizer
  // wants to be able to work with "high - low" as a number that has
  // reasonable precision.
  //
  // The argument can be an integer with bounds [low, high),
  // or a double with bounds [low, high]. If integral, the function
  // is only called with an integer in that position (but represented
  // as a double). All 32-bit integers can be represented exactly
  // as doubles.
  using arginfo =
    std::variant<std::tuple<int32_t, int32_t, int32_t, int32_t>,
                 std::tuple<double, double, double, double>>;
  static arginfo Double(double low, double high,
                        double down = -1.0/0.0,
                        double up = 1.0/0.0) {
    return arginfo(std::make_tuple(low, high, down, up));
  }

  static arginfo Integer(
      int32_t low, int32_t high,
      int32_t down = std::numeric_limits<int32_t>::lowest(),
      int32_t up = std::numeric_limits<int32_t>::max()) {
    return arginfo(std::make_tuple(low, high, down, up));
  }

  // It is currently required to call AddResult or Sample with a known
  // reasonable solution before calling Run. We only optimize subsets
  // of the parameters at a time (and so we need something feasible to
  // use for the rest of them).

  // Consider a candidate from a previous run or known feasible
  // solution.
  // Assumes f(arg) = {{arg_score, true}}
  void AddResult(const arg_type &arg, double score);

  // Force sampling this arg, for example if we know an existing
  // feasible argument from a previous round but not its score. Must
  // be feasible.
  void Sample(const arg_type &arg);

  // Optimize until a termination condition is reached. Can be called
  // multiple times. Must call AddResult or Sample with a feasible
  // argument first.
  void Run(
      // Information about the arguments.
      const std::vector<arginfo> &arginfos,

      // Termination conditions. Stops if any is attained; at
      // least one should be set!
      // Maximum actual calls to f. Note that since calls are
      // cached, if the argument space is exhaustible, you
      // may want to set another termination condition as well.
      std::optional<int> max_calls,
      // Maximum feasible calls (f returns an output). Same
      // caution as above.
      std::optional<int> max_feasible_calls = std::nullopt,
      // Walltime seconds. Typically we run over the budget by
      // some unspecified amount.
      std::optional<double> max_seconds = std::nullopt,
      // Stop as soon as we have any output with a score <= target.
      std::optional<double> target_score = std::nullopt,
      int params_per_pass = 24);

  // Get the best argument we found (with its score), if
  // any were feasible.
  std::optional<std::pair<arg_type, double>> GetBest() const;

  int64_t NumEvaluations() const { return evaluations; }

 private:
  const function_type f;

  // best value so far, if we have one
  std::optional<std::pair<arg_type, double>> best;

  struct HashArg {
    size_t operator ()(const arg_type &arg) const {
      uint64_t result = 0xCAFEBABE;
      for (int i = 0; i < arg.size(); i++) {
        result *= 0x113399557799;
        // This seems to be the most standards-compliant way to get
        // the bytes of a double?
        uint8_t bytes[sizeof (double)] = {};
        memcpy(&bytes, (const uint8_t *)&arg[i], sizeof (double));
        for (size_t j = 0; j < sizeof (double); j ++) {
          result ^= bytes[j];
          result = (result << 17) | (result >> (64 - 17));
        }
      }
      return (size_t) result;
    }
  };

  void MaybeSaveResult(const arg_type &arg,
                       std::pair<double, bool> res) {
    if constexpr (CACHE)
      cached_score[arg] = res;
    if (res.second) {
      // First or improved best?
      if (!best.has_value() || res.first < std::get<1>(best.value())) {
        best.emplace(arg, res.first);
      }
    }
  }

  // Number of arguments.
  int n = 0;

  // cache of previous results; only used if CACHE template parameter
  // is true. This is useful because the underlying optimizer works in
  // the space of doubles, and so we are likely to test the same
  // rounded integral arg multiple times. Cache is not cleaned.
  std::unordered_map<arg_type, std::pair<double, bool>, HashArg>
  cached_score;

  // Number of times we evaluated the target function.
  int64_t evaluations = 0;
  // seed1 and seed2 always nonzero.
  uint32_t seed1 = 1, seed2 = 2;
};


// Template implementations follow.

template<bool CACHE>
inline LargeOptimizer<CACHE>::LargeOptimizer(function_type f,
                                             int n,
                                             uint64_t random_seed) :
  f(std::move(f)), n(n) {
  seed1 = (random_seed >> 32);
  if (!seed1) seed1 = 1;
  seed2 = (random_seed & 0xFFFFFFFF);
  if (!seed2) seed2 = 2;
}

template<bool CACHE>
inline void LargeOptimizer<CACHE>::Sample(const arg_type &arg) {
  // (Note this does not read from cache, but it could?)
  auto res = f(arg);
  assert(res.second && "The sampled argument must be feasible");
  MaybeSaveResult(arg, res);
}

template<bool CACHE>
inline void LargeOptimizer<CACHE>::AddResult(const arg_type &best_arg,
                                             double best_score) {
  MaybeSaveResult(best_arg, std::pair<double, bool>(best_score, true));
}

template<bool CACHE>
inline std::optional<
  std::pair<typename LargeOptimizer<CACHE>::arg_type, double>>
LargeOptimizer<CACHE>::GetBest() const {
  return best;
}

namespace detail {
template<class... Ts>
struct large_optimizer_overloaded : Ts... { using Ts::operator()...; };
}

template<bool CACHE>
inline void LargeOptimizer<CACHE>::Run(
    // Information about the arguments.
    const std::vector<arginfo> &arginfos,
    std::optional<int> max_calls,
    std::optional<int> max_feasible_calls,
    std::optional<double> max_seconds,
    std::optional<double> target_score,
    int params_per_pass) {
  assert(params_per_pass > 0);
  const auto time_start = std::chrono::steady_clock::now();

  // TODO: This should somehow be dynamically determined?
  // static constexpr int PARAMS_PER_PASS = 32;

  // In here we pass off the optimization to Opt. But since
  // we have more parameters than Opt can handle at a time,
  // we choose subsets of the parameters to optimize greedily.

  static constexpr auto LFSRNext = [](uint32_t state) -> uint32_t {
    const uint32_t bit = std::popcount<uint32_t>(state & 0x8D777777) & 1;
    return (state << 1) | bit;
  };

  // Generate a pseudorandom number in [0, n - 1] using rejection
  // sampling, modifying seed1.
  auto RandTo32 = [this](uint32_t n) -> uint32_t {
    uint32_t mask = n - 1;
    mask |= mask >> 1;
    mask |= mask >> 2;
    mask |= mask >> 4;
    mask |= mask >> 8;
    mask |= mask >> 16;

    // Now, repeatedly generate random numbers, modulo that
    // power of two.

    for (;;) {
      seed1 = LFSRNext(seed1);
      const uint32_t x = seed1 & mask;
      if (x < n) return x;
    }
  };

  std::vector<int> optcounts(arginfos.size(), 0);
  std::vector<int> indices(arginfos.size());
  for (int i = 0; i < indices.size(); i++) indices[i] = i;

  auto Shuffle = [&RandTo32](std::vector<int> *v) {
      if (v->size() <= 1) return;
      for (int i = v->size() - 1; i >= 1; i--) {
        int j = RandTo32(i + 1);
        if (i != j) {
          std::swap((*v)[i], (*v)[j]);
        }
      }
    };

  auto GetSubset = [&optcounts, &indices, &Shuffle](int size) {
      // Get up to 'size' random indices.
      Shuffle(&indices);
      std::vector<int> subset;
      subset.reserve(size);
      for (int i = 0; i < indices.size() && i < size; i++)
        subset.push_back(indices[i]);

      // Keep track of how many times we've used them.
      // (XXX but we aren't using this yet!)
      for (int idx : indices) optcounts[idx]++;

      return subset;
    };


  bool stop = false;
  // These are only updated if we use them for termination.
  int num_calls = 0, num_feasible_calls = 0;
  do {
    stop = false;
    // Prep some subset.
    std::vector<int> indices = GetSubset(params_per_pass);
    int n = indices.size();

    // Optimization is based on the current best.
    assert(best.has_value() &&
           "must AddResult or Sample something feasible before Run");
    const arg_type best_arg = best.value().first;

    std::vector<bool> isint(n);
    std::vector<double> lbs(n), ubs(n);
    for (int i = 0; i < n; i++) {
      int idx = indices[i];
      std::visit(detail::large_optimizer_overloaded {
          [&](const std::tuple<int32_t, int32_t, int32_t, int32_t> &intarg) {
            isint[i] = true;
            const auto &[low32, high32, down32, up32] = intarg;
            // We work in the space of doubles; the precision should
            // be adequate even if these 32-bit values are the max or min.
            const double low = low32;
            const double high = high32;

            // Note: The called function is responsible for
            // decrementing the sample if it floors to
            // exactly the upper bound.
            lbs[i] = std::max(low, best_arg[idx] + (double)down32);
            ubs[i] = std::min(high, best_arg[idx] + (double)up32);
          },
          [&](const std::tuple<double, double, double, double> &dblarg) {
            const auto &[low, high, down, up] = dblarg;
            isint[i] = false;
            // max/min work correctly when down/up are infinite (default),
            // as long as low/high are finite.
            lbs[i] = std::max(low, best_arg[idx] + down);
            ubs[i] = std::min(high, best_arg[idx] + up);
          }
        }, arginfos[idx]);
    }

    auto df = [this, n,
               &arginfos,
               &indices, &isint,
               max_calls, max_feasible_calls,
               max_seconds, target_score, time_start,
               &stop, &num_calls, &num_feasible_calls](
                   const std::vector<double> &doubles) {
        if (stop) return LARGE_SCORE;

        // Test timeout first, to avoid cases where we have nearly
        // exhausted the input space.
        if (max_seconds.has_value()) {
          const auto time_now = std::chrono::steady_clock::now();
          const std::chrono::duration<double> time_elapsed =
            time_now - time_start;
          if (time_elapsed.count() > max_seconds.value()) {
            // printf("Ended due to timeout.\n");
            stop = true;
            return LARGE_SCORE;
          }
        }

        // Populate the native argument type.
        // We start with the current best and overwrite just the
        // parameters we're currently optimizing.
        assert(best.has_value() &&
               "must AddResult or Sample something feasible before Run");
        arg_type arg = best.value().first;
        for (int i = 0; i < n; i++) {
          int idx = indices[i];
          if (isint[i]) {
            int64_t a = (int64_t)floor(doubles[i]);
            // As described above; don't create invalid inputs if the
            // upper-bound is sampled exactly.
            //
            // XXX do we also want to treat "up" as exclusive? that
            // would be mean decrementing if we got min(ub, cur + up).
            const auto [lb_, ub, down_, up_] =
              std::get<std::tuple<int32_t, int32_t,
                                  int32_t, int32_t>>(arginfos[idx]);
            if (a >= ub) a = ub - 1;
            arg[idx] = a;
          } else {
            arg[idx] = doubles[i];
          }
        }

        if constexpr (CACHE) {
          // Have we already computed it?
          auto it = cached_score.find(arg);
          if (it != cached_score.end()) {
            return it->second.first;
          }
        }

        // Not cached, so this is a real call.
        if (max_calls.has_value()) {
          num_calls++;
          if (num_calls > max_calls.value()) {
            // printf("Ended due to max calls.\n");
            stop = true;
          }
        }

        evaluations++;
        auto res = f(arg);
        MaybeSaveResult(arg, res);

        if (res.second) {
          // Feasible call.
          if (max_feasible_calls.has_value()) {
            num_feasible_calls++;
            if (num_feasible_calls > max_feasible_calls.value()) {
              stop = true;
            }
          }

          if (res.first <= target_score) {
            // printf("Ended by reaching target score.\n");
            stop = true;
          }
        }
        return res.first;
      };

    // PERF: Better set biteopt parameters based on termination conditions.
    // Linear scaling is probably not right.
    // Perhaps this could itself be optimized?
    //
    // XXX: When cardinality is low, we should certainly reduce this!
    const int ITERS = 1000;

    seed1 = LFSRNext(seed1);
    seed2 = LFSRNext(seed2);
    std::swap(seed1, seed2);
    uint64_t random_seed = (uint64_t)seed1 << 32 | seed2;


    #if 0
    printf("Opt::Minimize with indices:");
    for (int i : indices) printf(" %d", i);
    printf("\n");

    printf("lbs:");
    for (double d : lbs) printf(" %.3f", d);
    printf("\n");

    printf("ubs:");
    for (double d : ubs) printf(" %.3f", d);
    printf("\n");

    printf("Current best: ");
    for (double d : best.value().first) printf(" %.3f", d);
    printf("\n");

    assert(n == lbs.size());
    assert(n == ubs.size());
    #endif

    // stop is set by the callback below, but g++ sometimes gets mad
    (void)(stop = !!stop);
    // PERF: Biteopt now has stopping conditions, so we should be able
    // to be more accurate here.
    (void)Opt::Minimize(n, df, lbs, ubs, ITERS, 1, 1, random_seed);
  } while (!stop);

  #if 0
  for (int i = 0; i < optcounts.size(); i++)
    printf("Arg %d optimized %d time(s)\n", i, optcounts[i]);
  #endif

}

#endif

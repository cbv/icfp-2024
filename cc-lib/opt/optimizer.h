
#ifndef _CC_LIB_OPT_OPTIMIZER_H
#define _CC_LIB_OPT_OPTIMIZER_H

// Fancier wrapper around black-box optimizer (opt.h).
// Improvements here:
//   - function can take integral parameters in addition
//     to floating-point ones.
//   - function returns an output value in addition to
//     its score; we produce the overall best-scoring value
//   - termination condition specified either with number
//     of actual calls to underlying function to be optimized,
//     or wall time
//   - TODO: threads? could be a template arg and we just
//     avoid the overhead if THREADS=1
//   - TODO: serialize state?

#include <array>
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
#include <unordered_set>
#include <string>

#include "opt/opt.h"

template<int N_INTS, int N_DOUBLES, class OutputType>
struct Optimizer {

  static inline constexpr int num_ints = N_INTS;
  static inline constexpr int num_doubles = N_DOUBLES;

  static inline constexpr double LARGE_SCORE =
    std::numeric_limits<double>::max();

  // Convenience constant for inputs where the function cannot even be
  // evaluated. However, optimization will be more efficient if you
  // instead return a penalty that exceeds any feasible score, and has
  // a gradient towards the feasible region.
  static inline constexpr
  std::pair<double, std::optional<OutputType>> INFEASIBLE =
    std::make_pair(LARGE_SCORE, std::nullopt);

  // function takes two arrays as arguments: the int
  //   args and the doubles.
  using arg_type =
    std::pair<std::array<int32_t, N_INTS>,
              std::array<double, N_DOUBLES>>;

  // Return value from the function being optimized, consisting
  // of a score and an output. If the output is nullopt, then
  // this is an infeasible input, and score should ideally give
  // some gradient towards the feasible region (and be bigger
  // than any score in the feasible region).
  using return_type = std::pair<double, std::optional<OutputType>>;

  // function to optimize.
  using function_type = std::function<return_type(arg_type)>;

  explicit Optimizer(function_type f,
                     uint64_t start_seed = 1);

  // Might already have a best candidate from a previous run or
  // known feasible solution. This is not currently used to inform
  // the search at all, but guarantees that GetBest will return a
  // solution at least as good as this one.
  // Assumes f(best_arg) = {{best_score, best_output}}
  void SetBest(const arg_type &best_arg, double best_score,
               OutputType best_output);

  // Force sampling this arg, for example if we know an existing
  // feasible argument from a previous round but not its result.
  void Sample(const arg_type &arg);

  // Run
  void Run(
      // Finite {lower, upper} bounds on each argument. This can be
      // different for each call to run, in case you want to systematically
      // explore different parts of the space, or update bounds from a
      // previous solution.
      // integer upper bounds exclude high: [low, high).
      // double upper bounds are inclusive: [low, high].
      const std::array<std::pair<int32_t, int32_t>, N_INTS> &int_bounds,
      const std::array<std::pair<double, double>, N_DOUBLES> &double_bounds,
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
      std::optional<double> target_score = std::nullopt);

  // Get the best argument we found (with its score and output), if
  // any were feasible.
  std::optional<std::tuple<arg_type, double, OutputType>> GetBest() const;

  // If set to true, the result of every function evaluation is saved and
  // can be queried later.
  void SetSaveAll(bool save);

  // Get all the evaluated results. The output types are only populated
  // if SaveAll was true when they were added (and the result was feasible).
  std::vector<std::tuple<arg_type, double, std::optional<OutputType>>>
  GetAll() const;

  int64_t NumEvaluations() const { return evaluations; }

  // Takes the current best and runs evaluations for points adjacent to
  // it (just changing one component at a time). Returns the newly sampled
  // points (including the best), so that you can call Explain on them.
  std::vector<std::tuple<arg_type, double, std::optional<OutputType>>>
  ExploreLocally(
      // For categorical arguments, we try every possibility in the
      // range.
      const std::array<bool, N_INTS> &categorical,
      const std::array<std::pair<int32_t, int32_t>, N_INTS> &int_bounds,
      const std::array<std::pair<double, double>, N_DOUBLES> &double_bounds,
      // For numerical arguments, explore a point this far from the best,
      // expressed as a fraction of the full width of the bounds. e.g. 0.01
      // means try +/- 1%.
      double numeric_delta) {

    if (!best.has_value()) return {};

    std::vector<std::tuple<arg_type, double, std::optional<OutputType>>>
      results;

    // Copy this, since the process below may update the best.
    const arg_type best_arg = std::get<0>(best.value());

    auto Try = [&](const arg_type &arg) {
        // TODO PERF: If we already have cached value, use it!

        auto [score, res] = f(arg);
        cached_score[arg] = score;
        if (save_all) cached_output[arg] = res;

        // Ignore non-feasible outputs.
        if (res.has_value()) {
          // First or improved best?
          if (!best.has_value() || score < std::get<1>(best.value())) {
            best.emplace(arg, score, std::move(res.value()));
          }

          results.emplace_back(arg, score, res);
        }
      };

    for (int i = 0; i < N_INTS; i++) {
      // Use best arg, but change the one slot.
      arg_type arg = best_arg;

      const int32_t cur = best_arg.first[i];

      if (categorical[i]) {
        for (int32_t c = int_bounds[i].first; c < int_bounds[i].second; c++) {
          if (c != cur) {
            arg.first[i] = c;
            Try(arg);
          }
        }
      } else {
        int width = int_bounds[i].second - int_bounds[i].first;
        if (width > 0) {
          {
            int32_t down = std::round((double)cur + width * -numeric_delta);
            if (down == cur) down--;
            if (down < int_bounds[i].first)
              down = int_bounds[i].first;

            if (down != cur) {
              arg.first[i] = down;
              Try(arg);
            }
          }

          {
            int32_t up = std::round((double)cur + width * numeric_delta);
            if (up == cur) up++;
            if (up >= int_bounds[i].second)
              up = int_bounds[i].second;

            if (up != cur) {
              arg.first[i] = up;
              Try(arg);
            }
          }
        }
      }
    }

    // Same for doubles, but this is much simpler.
    for (int i = 0; i < N_DOUBLES; i++) {
      // Use best arg, but change the one slot.
      arg_type arg = best_arg;

      const double cur = best_arg.second[i];

      const auto [bmin, bmax] = double_bounds[i];

      double width = bmax - bmin;
      {
        double down = std::max(bmin,
                               cur + width * -numeric_delta);

        if (down != cur) {
          arg.second[i] = down;
          Try(arg);
        }
      }

      {
        double up = std::min(bmax,
                             cur + width * numeric_delta);

        if (up != cur) {
          arg.second[i] = up;
          Try(arg);
        }
      }
    }

    return results;
  }

  // Using the saved data (SetSaveAll must be true and we must have
  // observations, usually by running optimization), fit coefficients
  // to the features that explain the output variable.
  //
  // Ignores the infeasible region.
  struct IntFeature {
    std::string name;
    bool categorical;
  };

  struct DoubleFeature {
    std::string name;
  };


  enum class FeatureType {
    NUMERIC_INT,
    NUMERIC_DOUBLE,
    CATEGORICAL_INT,
    BIAS,
  };

  struct Feature {
    FeatureType type;
    // Name, supplied by caller.
    std::string name;
    // Index into integers or doubles.
    // For bias term, holds -1.
    int index;
    // If CATEGORICAL_INT, the value seen.
    int32_t categorical_value;
    // Fit coefficient.
    double coefficient;
    int64_t num_observations;
  };

  // returns the features and the squared loss (L2 norm)
  std::pair<std::vector<Feature>, double>
  Explain(
      // Can pass GetAll(), or your own experiments.
      const std::vector<std::tuple<arg_type, double, std::optional<OutputType>>> &results,
      const std::array<IntFeature, N_INTS> &int_features,
      const std::array<DoubleFeature, N_DOUBLES> &double_features,
      // If present, a named bias term. Usually desirable.
      const std::optional<std::string> &bias) {
    // The coefficients for optimization.
    // We have one coefficient for each (non-categorical) integral
    // feature and each double feature. For categorical features,
    // we have one for each observed value.
    std::vector<std::unordered_set<int32_t>> values(
        N_INTS, std::unordered_set<int32_t>{});
    for (const auto &[arg, score_, ret_] : results) {
      const auto &ints = arg.first;
      for (int i = 0; i < N_INTS; i++) {
        if (int_features[i].categorical) {
          values[i].insert(ints[i]);
        }
      }
    }

    std::vector<Feature> features;

    // Now we have the distinct observed values.
    // Empty if not categorical.
    std::vector<std::vector<int32_t>> categorical(
        N_INTS, std::vector<int32_t>{});
    for (int i = 0; i < N_INTS; i++) {
      Feature f;
      f.name = int_features[i].name;
      f.index = i;
      f.coefficient = 0;
      f.num_observations = 0;
      if (int_features[i].categorical) {
        std::vector<int32_t> v(values[i].begin(), values[i].end());
        std::sort(v.begin(), v.end());
        f.type = FeatureType::CATEGORICAL_INT;
        // one copy for each distinct value
        for (int32_t x : v) {
          f.categorical_value = x;
          features.push_back(f);
        }
      } else {
        f.type = FeatureType::NUMERIC_INT;
        f.categorical_value = 0;
        features.push_back(f);
      }
    }

    for (int i = 0; i < N_DOUBLES; i++) {
      Feature f;
      f.type = FeatureType::NUMERIC_DOUBLE;
      f.name = double_features[i].name;
      f.index = i;
      f.coefficient = 0;
      f.num_observations = 0;
      f.categorical_value = 0;
      features.push_back(f);
    }

    if (bias.has_value()) {
      Feature f;
      f.type = FeatureType::BIAS;
      f.name = bias.value();
      f.index = -1;
      f.coefficient = 0;
      f.num_observations = 0;
      f.categorical_value = 0;
      features.push_back(f);
    }

    // Now do the optimization procedure.
    //

    const int num_features = features.size();

    auto Score = [this, &results, &features](const std::vector<double> &coeffs) -> double {
        double loss = 0.0;
        // For each row, compute the score we'd get with the chosen coefficients.
        for (const auto &[arg, actual_score, ret_] : results) {
          const auto &[int_args, double_args] = arg;
          double computed_score = 0.0;
          for (int i = 0; i < features.size(); i++) {
            const Feature &f = features[i];
            const double coeff = coeffs[i];
            switch (f.type) {
            case FeatureType::BIAS:
              computed_score += coeff;
              break;
            case FeatureType::CATEGORICAL_INT:
              // We only use this coefficient if the value matches
              // the argument.
              if (int_args[f.index] == f.categorical_value) {
                computed_score += coeff;
              }
              break;
            case FeatureType::NUMERIC_INT:
              computed_score += coeff * int_args[f.index];
              break;
            case FeatureType::NUMERIC_DOUBLE:
              computed_score += coeff * double_args[f.index];
              break;
            }
          }

          double diff = computed_score - actual_score;
          loss += (diff * diff);
        }

        return loss;
      };

    // Not much basis for choosing bounds. Could be args?
    std::vector<double> lower_bound(num_features, -1.0e6);
    std::vector<double> upper_bound(num_features, +1.0e6);

    const auto &[coeffs, loss] =
      Opt::Minimize(num_features,
                    Score,
                    lower_bound, upper_bound,
                    // XXX tune? or pass arg
                    1000 * sqrt(num_features));

    for (int i = 0; i < num_features; i++) {
      features[i].coefficient = coeffs[i];
    }

    return std::make_pair(features, loss);
  }

 private:
  static constexpr int N = N_INTS + N_DOUBLES;
  const function_type f;

  // best value so far, if we have one
  std::optional<std::tuple<arg_type, double, OutputType>> best;

  struct HashArg {
    size_t operator ()(const arg_type &arg) const {
      uint64_t result = 0xCAFEBABE;
      for (int i = 0; i < N_INTS; i++) {
        result ^= arg.first[i];
        result *= 0x314159265;
        result = (result << 13) | (result >> (64 - 13));
      }
      for (int i = 0; i < N_DOUBLES; i++) {
        result *= 0x1133995577;
        // This seems to be the most standards-compliant way to get
        // the bytes of a double?
        uint8_t bytes[sizeof (double)] = {};
        memcpy(&bytes, (const uint8_t *)&arg.second[i], sizeof (double));
        for (size_t j = 0; j < sizeof (double); j ++) {
          result ^= bytes[j];
          result = (result << 9) | (result >> (64 - 9));
        }
      }
      return (size_t) result;
    }
  };

  // cache of previous results. This is useful because the
  // underlying optimizer works in the space of doubles, and so
  // we are likely to test the same rounded integral arg multiple
  // times. Cache is not cleaned.
  std::unordered_map<arg_type, double, HashArg> cached_score;

  // Same for the output. Not stored unless save_all is set.
  std::unordered_map<arg_type, std::optional<OutputType>,
                     HashArg> cached_output;
  int64_t evaluations = 0;
  uint64_t random_seed = 1;
  bool save_all = false;
};


// Template implementations follow.

template<int N_INTS, int N_DOUBLES, class OutputType>
Optimizer<N_INTS, N_DOUBLES, OutputType>::Optimizer(function_type f,
                                                    uint64_t random_seed) :
  f(std::move(f)), random_seed(random_seed) {}

template<int N_INTS, int N_DOUBLES, class OutputType>
void Optimizer<N_INTS, N_DOUBLES, OutputType>::SetSaveAll(bool save) {
  save_all = save;
}

template<int N_INTS, int N_DOUBLES, class OutputType>
void Optimizer<N_INTS, N_DOUBLES, OutputType>::Sample(const arg_type &arg) {
  auto [score, res] = f(arg);
  cached_score[arg] = score;
  if (save_all) cached_output[arg] = res;

  if (res.has_value()) {
    // First or improved best?
    if (!best.has_value() || score < std::get<1>(best.value())) {
      best.emplace(arg, score, std::move(res.value()));
    }
  }
}

template<int N_INTS, int N_DOUBLES, class OutputType>
void Optimizer<N_INTS, N_DOUBLES, OutputType>::SetBest(
    const arg_type &best_arg, double best_score,
    OutputType best_output) {
  // Add to cache no matter what.
  cached_score[best_arg] = best_score;
  if (save_all) cached_output[best_arg] = best_output;

  // Don't take it if we already have a better one.
  if (best.has_value() && std::get<1>(best.value()) < best_score)
    return;
  best.emplace(best_arg, best_score, std::move(best_output));
}

template<int N_INTS, int N_DOUBLES, class OutputType>
std::optional<std::tuple<
                typename Optimizer<N_INTS, N_DOUBLES, OutputType>::arg_type,
                double, OutputType>>
Optimizer<N_INTS, N_DOUBLES, OutputType>::GetBest() const {
  return best;
}

template<int N_INTS, int N_DOUBLES, class OutputType>
auto Optimizer<N_INTS, N_DOUBLES, OutputType>::GetAll() const ->
std::vector<std::tuple<arg_type, double, std::optional<OutputType>>> {
  std::vector<std::tuple<arg_type, double, std::optional<OutputType>>> ret;
  ret.reserve(cached_score.size());
  for (const auto &[arg, score] : cached_score) {
    auto it = cached_output.find(arg);
    if (it != cached_output.end()) {
      ret.emplace_back(arg, score, it->second);
    } else {
      ret.emplace_back(arg, score, std::nullopt);
    }
  }

  return ret;
}


template<int N_INTS, int N_DOUBLES, class OutputType>
void Optimizer<N_INTS, N_DOUBLES, OutputType>::Run(
    const std::array<std::pair<int32_t, int32_t>, N_INTS> &int_bounds,
    const std::array<std::pair<double, double>, N_DOUBLES> &double_bounds,
    std::optional<int> max_calls,
    std::optional<int> max_feasible_calls,
    std::optional<double> max_seconds,
    std::optional<double> target_score) {
  const auto time_start = std::chrono::steady_clock::now();

  // Approach to rounding integers: ub is one past the largest integer
  // value to consider, lb is the lowest value considered. we pass the
  // same values as the double bounds. Then the sampled integer is
  // just floor(d), BUT: opt.h is not totally clear on whether the
  // upper bound is inclusive or exclusive. Since testing the actual
  // boundary seems like a reasonable thing, we handle the case
  // where the upper bound is sampled, by just decrementing it.
  std::array<double, N> lbs, ubs;
  for (int i = 0; i < N_INTS; i++) {
    lbs[i] = (double)int_bounds[i].first;
    ubs[i] = (double)int_bounds[i].second;
  }
  for (int i = 0; i < N_DOUBLES; i++) {
    lbs[N_INTS + i] = double_bounds[i].first;
    ubs[N_INTS + i] = double_bounds[i].second;
  }

  bool stop = false;
  // These are only updated if we use them for termination.
  int num_calls = 0, num_feasible_calls = 0;
  auto df = [this,
             max_calls, max_feasible_calls,
             max_seconds, target_score, time_start,
             &int_bounds,
             &stop, &num_calls, &num_feasible_calls](
      const std::array<double, N> &doubles) {
      if (stop) return LARGE_SCORE;

      // Test timeout first, to avoid cases where we have nearly
      // exhausted the input space.
      if (max_seconds.has_value()) {
        const auto time_now = std::chrono::steady_clock::now();
        const std::chrono::duration<double> time_elapsed =
          time_now - time_start;
        if (time_elapsed.count() > max_seconds.value()) {
          stop = true;
          return LARGE_SCORE;
        }
      }

      // Populate the native argument type.
      arg_type arg;
      for (int i = 0; i < N_INTS; i++) {
        int32_t a = (int32_t)doubles[i];
        // As described above; don't create invalid inputs if the
        // upper-bound is sampled exactly.
        if (a >= int_bounds[i].second) a = int_bounds[i].second - 1;
        arg.first[i] = a;
      }
      for (int i = 0; i < N_DOUBLES; i++) {
        const double d = doubles[i + N_INTS];
        arg.second[i] = d;
      }

      {
        // Have we already computed it?
        auto it = cached_score.find(arg);
        if (it != cached_score.end()) {
          return it->second;
        }
      }

      // Not cached, so this is a real call.
      if (max_calls.has_value()) {
        num_calls++;
        if (num_calls > max_calls.value()) {
          stop = true;
        }
      }

      evaluations++;
      auto [score, res] = f(arg);
      cached_score[arg] = score;
      if (save_all) cached_output[arg] = res;

      if (res.has_value()) {
        // Feasible call.
        if (max_feasible_calls.has_value()) {
          num_feasible_calls++;
          if (num_feasible_calls > max_feasible_calls.value()) {
            stop = true;
          }
        }

        // First or new best? Save it.
        if (!best.has_value() || score < std::get<1>(best.value())) {
          best.emplace(arg, score, std::move(res.value()));
        }

        if (score <= target_score)
          stop = true;
      }
      return score;
    };

  // Run double-based optimizer on above.

  // PERF: Better set biteopt parameters based on termination conditions.
  // Linear scaling is probably not right.
  // Perhaps this could itself be optimized?
  const int ITERS = 1000 * powf(N, 1.5f);

  // TODO: 64-bit LFSR
  auto LFSRNext = [](uint32_t state) -> uint32_t {
    const uint32_t bit = std::popcount<uint32_t>(state & 0x8D777777) & 1;
    return (state << 1) | bit;
  };

  // LFSR requires nonzero seeds.
  uint32_t seed1 = (random_seed >> 32);
  if (!seed1) seed1 = 1;
  uint32_t seed2 = (random_seed & 0xFFFFFFFF);
  if (!seed2) seed2 = 2;

  // XXX HERE

  while (!stop) {
    seed1 = LFSRNext(seed1);
    seed2 = LFSRNext(seed2);
    // Update seed member so that next call to Run is (pseudo)independent.
    random_seed = (uint64_t)seed1 << 32 | (uint64_t)seed2;

    // stop is set by the callback below, but g++ sometimes gets mad
    (void)(stop = !!stop);
    // PERF: Biteopt now has stopping conditions, so we should be able
    // to be more accurate here.
    Opt::Minimize<N>(df, lbs, ubs, ITERS, 1, 1, random_seed);
  }
}

#endif

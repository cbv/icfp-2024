#ifndef _CC_LIB_AUTOPARALLEL_H
#define _CC_LIB_AUTOPARALLEL_H

#include <chrono>
#include <cstdint>
#include <limits>
#include <vector>
#include <string>
#include <memory>

#include "arcfour.h"
#include "randutil.h"
#include "util.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "threadutil.h"

// Assumes the workload has constant cost over time. (Typical usage is
// for training a neural network, where the values change each round,
// but the structure and size are the same.)
//
// Automatically determines, by trial and error, the best number of
// threads to use--including the possibility of running in serial.
// Note: The wrapper itself is not thread-safe!
//
// Usage:
//
// AutoParallelComp comp{16, 50};
//
// for (;;) {
//   ...
//   comp.ParallelComp(vector.size(), [&vector](int idx) { ... });
//   ...
// }
struct AutoParallelComp {
  // For max_parallelism, really give the largest number you're willing
  // to consider.
  // max_samples is the maximum number of samples (per bucket) to
  // gather before concluding that its value is the sample mean.
  // If cachefile is not the empty string, read old samples from
  // this file (if any), and write them periodically.
  explicit AutoParallelComp(int max_parallelism,
                            int max_samples = 50,
                            bool verbose = false,
                            const std::string &cachefile = "") :
    max_parallelism(max_parallelism),
    max_samples(max_samples),
    verbose(verbose),
    cachefile(cachefile),
    experiments(max_parallelism, Experiment{}) {
    if (!cachefile.empty()) {
      ReadCache();
      // Number of milliseconds since we started running, so
      // that we can throttle saving.
      last_save = run_timer.MS();
    }
    last_status = run_timer.MS();
    rc = std::make_unique<ArcFour>(
        StringPrintf("apc%lld.%f", time(nullptr), last_save));
  }

  // with f(idx, value), ignoring result
  template<class T, class F>
  void ParallelAppi(const std::vector<T> &vec, const F &f) {
    auto ff = [&vec, &f](int64_t idx) {
        (void)f(idx, vec[idx]);
      };
    ParallelComp(vec.size(), ff);
  }

  // with f(value), ignoring result
  template<class T, class F>
  void ParallelApp(const std::vector<T> &vec, const F &f) {
    auto ff = [&vec, &f](int64_t idx) {
        (void)f(vec[idx]);
      };
    ParallelComp(vec.size(), ff);
  }

  // with f(idx, value)
  template<class T, class F>
  auto ParallelMapi(const std::vector<T> &vec, const F &f) ->
    std::vector<decltype(f((int64_t)0, vec.front()))> {
    using R = decltype(f((int64_t)0, vec.front()));

    std::vector<R> result(vec.size());
    R *data = result.data();
    auto run_write = [data, &f](int64_t idx, const T &arg) {
        data[idx] = f(idx, arg);
      };
    ParallelAppi(vec, run_write);
    return result;
  }

  // with f(value)
  template<class T, class F>
  auto ParallelMap(const std::vector<T> &vec, const F &f) ->
    std::vector<decltype(f(vec.front()))> {
    auto ff = [&f](int64_t idx, const T &arg) { return f(arg); };
    return ParallelMapi(vec, ff);
  }


  template<class F>
  void ParallelComp(int64_t num, const F &f) {
    // Break into batches, so that large calls can still do some
    // autotuning.
    const int64_t MAX_BATCH_SIZE = max_parallelism * (int64_t)10000;
    for (int64_t start = 0; start < num; start += MAX_BATCH_SIZE) {
      const int64_t THIS_BATCH = std::min(MAX_BATCH_SIZE, num - start);
      ParallelCompInternal(THIS_BATCH,
                           [num, start, &f](int64_t idx) {
                             // debuggin'
                             CHECK(start + idx < num);
                             f(start + idx);
                           });
    }
  }

  // Gets the number of threads to use for one round.
  int GetNumThreads(bool verbose_round = false) {
    // We want to balance between running the actual fastest bucket,
    // and running new experiments to improve our understanding of
    // buckets. The way we do this is to generate a variate for each
    // bucket according to its current model, and pick the one with
    // the lowest score. This is not completely principled, but it's
    // very simple! (The intuition is that buckets with little data
    // have high variance, and so will sometimes generate low variates
    // and get an experiment.)

    // PERF avoid this loop once all buckets are full.

    RandomGaussian gauss(rc.get());

    // Remember, i=0 means threads=1
    int best_i = 0;
    double best_ms = std::numeric_limits<double>::infinity();

    for (int i = 0; i < max_parallelism; i++) {
      const double ms = [this, i, verbose_round, &gauss]() {
          if ((int)experiments[i].sample_ms.size() >= max_samples) {
            // Once we have enough samples, just used the computed
            // mean without randomness.
            const double ms = experiments[i].current_mean;
            if (verbose_round) {
              printf(" %d parallelism: DONE. %.5fms\n", i + 1, ms);
            }
            return ms;
          } else {
            const double stdev = experiments[i].current_stdev;
            const double mean = experiments[i].current_mean;
            const double ms = gauss.Next() * stdev + mean;
            if (verbose_round) {
              printf(
                  " %d parallelism: %d samples, "
                  "predict %.5f +/ %.5fms ~= %.5fms\n",
                  i + 1, (int)experiments[i].sample_ms.size(), mean, stdev, ms);
            }
            return ms;
          }
        }();

      if (ms < best_ms) {
        best_i = i;
        best_ms = ms;
      }
    }

    auto Consider = [this, &best_i](int dx) {
        int neighbor = best_i + dx;
        if (neighbor < 0) return false;
        if (neighbor >= max_parallelism) return false;

        int bsamples = experiments[best_i].sample_ms.size();
        int nsamples = experiments[neighbor].sample_ms.size();
        // Never jitter if we are done with the other bucket.
        if (nsamples >= max_samples) return false;

        int diff = bsamples - nsamples;
        if (diff <= 0) return false;
        // The bigger the difference, the more likely we are to do this.
        if ((int)RandTo(rc.get(), bsamples) >= diff) return false;
        best_i = neighbor;
        return true;
      };

    // consider some jitter to nearby buckets if this one has a lot
    // more samples. (could even loop this?)
    if (!Consider(-1)) Consider(+1);

    if (verbose_round) {
      printf("AutoParallelComp: Selected threads=%d (%.5f ms +/- %.5f)\n",
             best_i + 1,
             experiments[best_i].current_mean,
             experiments[best_i].current_stdev);
    }

    return best_i + 1;
  }

  // Core routine that balances the workload. Usually call one of the
  // other functions instead.
  template<class F>
  void ParallelCompInternal(int64_t num, const F &f) {

    bool verbose_round = false;
    if (verbose) {
      const double elapsed_ms = run_timer.MS() - last_status;
      if (elapsed_ms > VERBOSE_EVERY_MS) {
        verbose_round = true;
        last_status = run_timer.MS();
      }
    }

    const int threads = GetNumThreads(verbose_round);
    CHECK(threads > 0);

    MsTimer expt_timer;
    if (threads <= 1) {
      // Run in serial without any locking etc.
      for (int64_t i = 0; i < num; i++) {
        (void)f(i);
      }
    } else {
      // Use unqualified version from threadutil.
      ::ParallelComp(num, f, threads);
    }

    // Compute time per element.
    const double actual_ms = expt_timer.MS() / (double)num;

    // If we're still taking samples, add it and update the
    // model parameters.
    int best_i = threads - 1;
    if ((int)experiments[best_i].sample_ms.size() < max_samples) {
      experiments[best_i].sample_ms.push_back(actual_ms);
      UpdateStatistics(&experiments[best_i]);
      if (verbose_round) {
        printf("Got %.5fms/elt. Set threads=%d to %d samples: %.5f +/- %.5f\n",
               actual_ms,
               best_i + 1,
               (int)experiments[best_i].sample_ms.size(),
               experiments[best_i].current_mean,
               experiments[best_i].current_stdev);
      }
      MaybeWriteCache();
    }
  }

  void WriteCache() {
    if (cachefile.empty()) return;

    std::vector<std::string> lines;
    int total_samples = 0;
    for (int i = 0; i < (int)experiments.size(); i++) {
      if (!experiments[i].sample_ms.empty()) {
        std::string line = StringPrintf("%d", i);
        for (double s : experiments[i].sample_ms) {
          total_samples++;
          StringAppendF(&line, " %.7f", s);
        }
        lines.push_back(line);
      }
    }
    Util::WriteLinesToFile(cachefile, lines);
    if (verbose) {
      printf("Wrote %d samples to %s.\n",
             total_samples, cachefile.c_str());
    }
    // (Don't count the time it took to save, for pathological
    // cases..)
    last_save = run_timer.MS();
  }

  void PrintHisto() {
    printf("%s", HistoString().c_str());
  }

  std::string HistoString() {
    // Get min/max, including stdev
    double min_ms = std::numeric_limits<double>::infinity();
    double max_ms = -std::numeric_limits<double>::infinity();
    for (const Experiment &expt : experiments) {
      min_ms = std::min(min_ms, expt.current_mean - expt.current_stdev);
      max_ms = std::max(max_ms, expt.current_mean + expt.current_stdev);
    }

    if (max_ms <= min_ms) {
      return "(experiment samples are degenerate)\n";
    }

    double width_ms = max_ms - min_ms;

    const char *units = "ms";
    double scale = 1.0;
    // TODO: Other scales!
    if (max_ms < 0.1) {
      units = "us";
      scale = 1000.0;
    } else {
      units = "ms";
      scale = 1.0;
    }

    std::string out;
    //      123 12345 12345678
    //      12345678901234567890
    StringAppendF(&out, "th |  # | avg %s |\n", units);
    static constexpr int HW = 59;
    for (int i = 0; i < (int)experiments.size(); i++) {
      const Experiment &expt = experiments[i];
      // '% 2d' doesn't really work; the space is allocated
      // to the missing '+' sign.
      std::string th = StringPrintf("%d", i + 1);
      while (th.size() < 2) th = (std::string)" " + th;
      std::string sam = StringPrintf("%d", (int)expt.sample_ms.size());
      while (sam.size() < 3) sam = (std::string)" " + sam;
      std::string avg = StringPrintf("%.2f", expt.current_mean * scale);
      while (avg.size() < 8) avg = (std::string)" " + avg;
      StringAppendF(&out, "%s |%s |%s| ",
                    th.c_str(), sam.c_str(), avg.c_str());

      // No need to scale these, since they are already normalized.
      double emin = (expt.current_mean - expt.current_stdev);
      double emax = (expt.current_mean + expt.current_stdev);
      double minf = (emin - min_ms) / width_ms;
      double maxf = (emax - min_ms) / width_ms;
      double avgf = (expt.current_mean - min_ms) / width_ms;
      int imin = round(minf * (HW - 1));
      int iavg = round(avgf * (HW - 1));
      int imax = round(maxf * (HW - 1));
      for (int x = 0; x < HW; x++) {
        char c = ' ';
        if (x == iavg) c = '*';
        else if (x == imin) c = '<';
        else if (x == imax) c = '>';
        else if (x > imin && x < iavg) c = '-';
        else if (x > iavg && x < imax) c = '-';
        out.push_back(c);
      }
      StringAppendF(&out, "\n");
    }
    return out;
  }

private:
  // This can certainly be improved!
  struct Experiment {
    // Actual sample values collected, up to max_samples.
    // sample_ms is (now) the number of milliseconds per element, not per call.
    std::vector<double> sample_ms;
    // Invariant: These agree with the sample_ms vector.
    // Note that 0.0 is a good (though wrong) initial estimate
    // since that means experimenting with an empty bucket will
    // beat out any bucket with actual data.
    double current_mean = 0.0;
    double current_stdev = 0.0;
  };

  // (make configurable?)
  static constexpr double SAVE_EVERY_MS = 60.0 * 1000.0;
  static constexpr double VERBOSE_EVERY_MS = 30.0 * 1000.0;

  void MaybeWriteCache() {
    if (cachefile.empty()) return;
    const double elapsed_ms = run_timer.MS() - last_save;
    if (elapsed_ms > SAVE_EVERY_MS) {
      WriteCache();
    }
  }

  void ReadCache() {
    CHECK(!cachefile.empty());
    std::vector<std::string> lines = Util::ReadFileToLines(cachefile);
    for (int i = 0; i < (int)lines.size(); i++) {
      std::string line = Util::NormalizeWhitespace(lines[i]);
      if (line.empty()) continue;
      std::string bucket_s = Util::chop(line);
      const int bucket = atoi(bucket_s.c_str());
      CHECK(bucket >= 0);
      if (bucket < (int)experiments.size()) {
        experiments[bucket].sample_ms.clear();
        for (;;) {
          const std::string s = Util::chop(line);
          if (s.empty()) break;
          double ss = Util::ParseDouble(s);
          experiments[bucket].sample_ms.push_back(ss);
        }
      } else {
        printf("WARNING: Autoparallel discarding bucket %d "
               "from %s (out of range)\n", bucket, cachefile.c_str());
      }
    }

    if (verbose) {
      printf("From %s:\n", cachefile.c_str());
    }
    for (int i = 0; i < (int)experiments.size(); i++) {
      UpdateStatistics(&experiments[i]);
      if (verbose) {
        printf("  threads=%d with %d samples: %.5f +/- %.5f\n",
               i + 1,
               (int)experiments[i].sample_ms.size(),
               experiments[i].current_mean,
               experiments[i].current_stdev);
      }
    }
  }

  void UpdateStatistics(Experiment *expt) {
    const int num_samples = expt->sample_ms.size();
    if (num_samples == 0) {
      expt->current_mean = 0.0;
      expt->current_stdev = 0.0;
    }

    double total = 0.0;
    for (double d : expt->sample_ms)
      total += d;
    const double mean =
      num_samples == 0 ? 0.0 :
      total / (double)num_samples;


    double sqerr = 0.0;
    for (double d : expt->sample_ms) {
      double dd = (d - mean);
      sqerr += dd * dd;
    }

    // Use the mean as the stddev until we have at least two
    // samples; otherwise it will be zero (or undefined).
    const double stdev =
      num_samples <= 1 ? mean :
      sqrt(sqerr / (double)num_samples);

    expt->current_mean = mean;
    expt->current_stdev = stdev;
  }

  struct MsTimer {
    MsTimer() : time_start(std::chrono::steady_clock::now()) {}

    double MS() const {
      auto time_now = std::chrono::steady_clock::now();
      const std::chrono::duration<double> time_elapsed =
        time_now - time_start;
      return time_elapsed.count() * 1000.0;
    }

  private:
    const std::chrono::time_point<std::chrono::steady_clock> time_start;
  };

  const int max_parallelism = 1;
  const int max_samples = 1;
  const bool verbose = false;
  const std::string cachefile;
  std::unique_ptr<ArcFour> rc;
  // experiments[i] is i+1 threads
  std::vector<Experiment> experiments;
  MsTimer run_timer;
  double last_save = 0.0, last_status = 0.0;
};

#endif

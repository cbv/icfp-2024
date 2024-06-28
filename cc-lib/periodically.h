
// For use in polling loops. Keeps track of a "next time to run"
// and tells the caller when it should run. Thread safe.

#ifndef _CC_LIB_PERIODICALLY_H
#define _CC_LIB_PERIODICALLY_H

#include <atomic>
#include <cstdint>
#include <chrono>
#include <mutex>

struct Periodically {
  // If start_ready is true, then the next call to ShouldRun will return
  // true. Otherwise, we wait for the period to elapse first.
  explicit Periodically(double wait_period_seconds, bool start_ready = true) :
      next_run(std::chrono::steady_clock::now()) {
    using namespace std::chrono_literals;
    wait_period = std::chrono::duration_cast<dur>(1s * wait_period_seconds);
    // Immediately available.
    if (start_ready) {
      next_run.store(std::chrono::steady_clock::now());
    } else {
      next_run.store(std::chrono::steady_clock::now() + wait_period);
    }
  }

  // Return true if 'seconds' has elapsed since the last run.
  // If this function returns true, we assume the caller does
  // the associated action now (and so move the next run time
  // forward).
  bool ShouldRun() {
    const tpoint now = std::chrono::steady_clock::now();
    if (now >= next_run.load()) {
      std::unique_lock ml(m);
      if (paused) return false;
      // double-checked lock so that the previous can be cheap
      if (now >= next_run.load()) {
        next_run.store(now + wait_period);
        times_run++;
        return true;
      }
    }
    return false;
  }

  // Just like ShouldRun, but updates the next run time *after*
  // the function runs. This can be used to prevent the function
  // from running too often, even if it takes a long time. If
  // a function is already in progress, the call will immediately
  // return.
  //
  // Should not call methods of this class from the callback,
  // since this can lead to deadlock if other threads are holding
  // the mutex waiting for the callback to return.
  template<class F>
  void RunIf(const F &f) {
    const tpoint now = std::chrono::steady_clock::now();
    if (now >= next_run.load()) {
      std::unique_lock ml(m);
      if (run_in_progress) return;
      if (paused) return;
      if (now >= next_run.load()) {
        run_in_progress = true;
        times_run++;
        // Try to avoid lock contention by advancing the clock;
        // its final value will be at least this.
        next_run.store(now + wait_period);

        // But don't hold the lock while running the function.
        ml.unlock();
        f();
        ml.lock();

        run_in_progress = false;
        const tpoint now2 = std::chrono::steady_clock::now();
        next_run.store(now2 + wait_period);
      }
    }
  }

  void Pause() {
    std::unique_lock ml(m);
    paused = true;
  }

  void Reset() {
    std::unique_lock ml(m);
    paused = false;
    next_run = std::chrono::steady_clock::now() + wait_period;
    times_run = 0;
  }

  // Sets the wait period, and resets the timer, so the next run
  // will be in this many seconds.
  void SetPeriod(double seconds) {
    std::unique_lock ml(m);
    using namespace std::chrono_literals;
    wait_period = std::chrono::duration_cast<dur>(1s * seconds);
    Reset();
  }

  // Run in this many seconds, but then return to the current period.
  void SetPeriodOnce(double seconds) {
    std::unique_lock ml(m);
    using namespace std::chrono_literals;
    next_run = std::chrono::steady_clock::now() +
      std::chrono::duration_cast<dur>(1s * seconds);
  }

  // Get the number of times that the periodic event has run. This
  // is considered to have run when ShouldRun returns true or
  // RunIf will invoke the function; in both cases, *before* the
  // handler has executed.
  int64_t TimesRun() {
    std::unique_lock ml(m);
    return times_run;
  }

private:
  using dur = std::chrono::steady_clock::duration;
  using tpoint = std::chrono::time_point<std::chrono::steady_clock>;
  std::mutex m;
  std::atomic<tpoint> next_run;
  int64_t times_run = 0;
  dur wait_period = dur::zero();
  bool paused = false;
  bool run_in_progress = false;
};

#endif

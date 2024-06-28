
#ifndef _CC_LIB_THREADUTIL_H
#define _CC_LIB_THREADUTIL_H

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>
#include <cmath>
#include <cassert>

#if __cplusplus >= 201703L
// shared_mutex only available in C++17 and later.
# include <shared_mutex>
#endif


struct MutexLock {
  explicit MutexLock(std::mutex *m) : m(m) { m->lock(); }
  ~MutexLock() { m->unlock(); }
  std::mutex *m;
};

// Read with the mutex that protects it. T must be copyable,
// obviously!
template<class T>
T ReadWithLock(std::mutex *m, const T *t) {
  MutexLock ml(m);
  return *t;
}

// Write with the mutex that protects it. T must be copyable.
template<class T>
void WriteWithLock(std::mutex *m, T *t, const T &val) {
  MutexLock ml(m);
  *t = val;
}

#if __cplusplus >= 201703L
// Overloads for shared_mutex, if available.
struct ReadMutexLock {
  explicit ReadMutexLock(std::shared_mutex *m) : m(m) { m->lock_shared(); }
  ~ReadMutexLock() { m->unlock_shared(); }
  std::shared_mutex *m;
};
// Possible to template this over shared_mutex and mutex without
// requiring an argument?
struct WriteMutexLock {
  explicit WriteMutexLock(std::shared_mutex *m) : m(m) { m->lock(); }
  ~WriteMutexLock() { m->unlock(); }
  std::shared_mutex *m;
};

// Read with the mutex that protects it. T must be copyable,
// obviously!
template<class T>
T ReadWithLock(std::shared_mutex *m, const T *t) {
  ReadMutexLock ml(m);
  return *t;
}

// Write with the mutex that protects it. T must be copyable.
template<class T>
void WriteWithLock(std::shared_mutex *m, T *t, const T &val) {
  WriteMutexLock ml(m);
  *t = val;
}
#endif

// TODO: Could have an optimism parameter for most of these that
// causes each thread to grab up to N work items (using num_threads as
// a stride) before starting to synchronize for indices. If each item
// takes the same amount of time, this just has less overhead.

// A thing that comes up often is where we want to accumulate a
// sum (maybe on many variables) over an array in parallel. The
// typical way to do this involves sharing a mutex to protect some
// variable and doing +=. This utility folds over some abstract result
// type Res; each thread has its own accumulator.
//
// Res should be a small copyable type, like a pair of ints.
// add should be commutative and associative, with zero as its zero value.
// f is applied to every int in [0, num - 1] and Res*, returning void.
// It has exclusive access to res. Typical would be to += into it.
template<class Res, class Add, class F>
Res ParallelAccumulate(int64_t num,
                       Res zero,
                       const Add &add,
                       const F &f,
                       int max_concurrency) {
  if (max_concurrency > num) max_concurrency = num;
  // Need at least one thread for correctness.
  int num_threads = std::max(max_concurrency, 1);

  std::mutex index_m;
  // First num_threads indices are assigned from the start
  int64_t next_index = num_threads;

  // Each thread gets its own accumulator so there's no need to
  // synchronize access.
  std::vector<Res> accs(num_threads, zero);

  auto th = [&accs,
             &index_m, &next_index, num, &f](int thread_num) {
    // PERF consider creating the accumulator in the thread as
    // a local, for numa etc.?
    int my_idx = thread_num;
    Res *my_acc = &accs[thread_num];
    do {
      // Do work, not holding any locks.
      (void)f(my_idx, my_acc);

      // Get next index, if any.
      index_m.lock();
      if (next_index == num) {
        // All done. Don't increment counter so that other threads can
        // notice this too.
        index_m.unlock();
        return;
      }
      my_idx = next_index++;
      index_m.unlock();
    } while (true);
  };

  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back(th, i);
  }

  // Wait for each thread to finish, and accumulate the thread-local
  // values into the final one as we do.
  Res res = zero;
  for (int i = 0; i < num_threads; i++) {
    threads[i].join();
    res = add(res, accs[i]);
  }
  return res;
}

// TODO: Easy to have version of the above that applies to each
// element of a vector, etc.

// Do progress meter.
// It should be thread safe and have a way for a thread to register a sub-meter.

// Parallel comprehension. Runs f on 0...(num-1).
// (Comprehension might not be the right name for this given that it
// doesn't return anything? We can switch to ParallelDo ?)
template<class F>
void ParallelComp(int64_t num,
                  const F &f,
                  int max_concurrency) {
  max_concurrency = std::min(num, (int64_t)max_concurrency);
  // Need at least one thread for correctness.
  max_concurrency = std::max(max_concurrency, 1);
  std::mutex index_m;
  int64_t next_index = 0;

  // Thread applies f repeatedly until there are no more indices.
  // PERF: Can just start each thread knowing its start index, and avoid
  // the synchronization overhead at startup.
  auto th = [&index_m, &next_index, max_concurrency, num, &f]() {
    for (;;) {
      index_m.lock();
      if (next_index == num) {
        // All done. Don't increment counter so that other threads can
        // notice this too.
        index_m.unlock();
        return;
      }
      // Locking can be pretty expensive if there's a lot of
      // contention (if f is fast, for example). Likely better to
      // claim more than one index when we're far from the end.
      int64_t num_left = num - next_index;
      size_t my_batch_size = 1;
      // Only consider batching if there are more remaining items
      // than threads; otherwise we would starve threads for sure.
      if (num_left > max_concurrency) {
        // If every item took the same amount of time,
        // we'd want to claim num_left/max_concurrency items here.
        // But we want to be robust against randomly distributed
        // slow tasks. So we reduce our optimism as we get closer
        // to the end. PERF: Tune!
        my_batch_size = std::max((size_t)(sqrtf(num_left) / max_concurrency),
                                 (size_t)1);
      }
      int64_t my_index = next_index;
      next_index += my_batch_size;
      index_m.unlock();

      // Do work, not holding mutex.
      for (size_t i = 0; i < my_batch_size; i++)
        (void)f(my_index + i);
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(max_concurrency);
  for (int i = 0; i < max_concurrency; i++) {
    threads.emplace_back(th);
  }
  // Now just wait for them all to finish.
  for (std::thread &t : threads) t.join();
}

// Drop-in serial replacement for debugging, etc.
template<class F>
void UnParallelComp(int64_t num, const F &f, int max_concurrency_ignored) {
  for (int64_t i = 0; i < num; i++) (void)f(i);
}

// Run the function f(idx, item) for each item in the vector in up to
// max_concurrency parallel threads. The caller must of course
// synchronize any accesses to shared data structures. Return value of
// function is ignored.
template<class T, class F>
void ParallelAppi(const std::vector<T> &vec,
                  const F &f,
                  int max_concurrency) {

  auto ff = [&vec, &f](int64_t idx) {
      (void)f(idx, vec[idx]);
    };
  ParallelComp(vec.size(), ff, max_concurrency);
}

// Same, but the typical case that the index is not needed.
template<class T, class F>
void ParallelApp(const std::vector<T> &vec,
                 const F &f,
                 int max_concurrency) {
  auto ff = [&f](int64_t i_unused, const T &arg) { return f(arg); };
  ParallelAppi(vec, ff, max_concurrency);
}

// Drop-in serial replacement for debugging, etc.
template<class T, class F>
void UnParallelApp(const std::vector<T> &vec,
                   const F &f,
                   int max_concurrency) {
  for (const auto &t : vec) f(t);
}

// e.g. InParallel(
//    []() { some code; },
//    []() { some more code; }
// );
template<class... Fs>
inline void InParallel(Fs... fs) {
  // PERF: Can we do this without copying?
  std::vector v{std::function<void(void)>(fs)...};
  ParallelApp(v, [](const std::function<void(void)> &f) {
      f();
    }, v.size());
}

// Generate the vector containing {f(0), f(1), ..., f(num - 1)}.
// The result type must be default-constructible.
template<class F>
auto ParallelTabulate(int64_t num,
                      const F &f,
                      int max_concurrency) ->
  std::vector<decltype(f((int64_t)0))> {
  using R = decltype(f((int64_t)0));
  static_assert(std::is_default_constructible<R>::value,
                "result must be default constructible");
  std::vector<R> result;
  result.resize(num);
  R *data = result.data();
  auto run_write = [data, &f](int64_t idx) {
                     data[idx] = f(idx);
                   };
  ParallelComp(num, run_write, max_concurrency);
  return result;
}

// With f(index, value).
// F needs to be callable (std::function or lambda) and thread safe.
// It returns R, which must have a default constructor, and this will
// only be efficient if it has move semantics as well.
template<class T, class F>
auto ParallelMapi(const std::vector<T> &vec,
                  const F &f,
                  int max_concurrency) ->
  std::vector<decltype(f((int64_t)0, vec.front()))> {
  using R = decltype(f((int64_t)0, vec.front()));
  static_assert(std::is_default_constructible<R>::value,
                "result must be default constructible");
  std::vector<R> result;
  result.resize(vec.size());

  // Not sure if C++11 makes thread safety guarantees about
  // vector::operator[], but if we have a data pointer then we can be
  // confident of having one writer to each slot.
  R *data = result.data();
  auto run_write = [data, &f](int64_t idx, const T &arg) {
                     data[idx] = f(idx, arg);
                   };
  ParallelAppi(vec, run_write, max_concurrency);
  return result;
}

// With f(value).
// F needs to be callable (std::function or lambda) and thread safe.
// It returns R, which must have a default constructor, and this will
// only be efficient if it has move semantics as well.
template<class T, class F>
auto ParallelMap(const std::vector<T> &vec,
                 const F &f,
                 int max_concurrency) -> std::vector<decltype(f(vec.front()))> {
  auto ff = [&f](int64_t idx, const T &arg) { return f(arg); };
  return ParallelMapi(vec, ff, max_concurrency);
}

// Drop in replacement for testing, debugging, etc.
template<class T, class F>
auto UnParallelMap(const std::vector<T> &vec,
                   const F &f, int max_concurrency_ignored) ->
  std::vector<decltype(f(vec.front()))> {
  using R = decltype(f(vec.front()));
  static_assert(std::is_default_constructible<R>::value,
                "result must be default constructible");
  std::vector<R> result;
  result.resize(vec.size());

  for (int64_t i = 0; i < (int64_t)vec.size(); i++) {
    result[i] = f(vec[i]);
  }

  return result;
}

template<class T, class F>
auto UnParallelMapi(const std::vector<T> &vec,
                    const F &f, int max_concurrency_ignored) ->
  std::vector<decltype(f((int64_t)0, vec.front()))> {
  using R = decltype(f((int64_t)0, vec.front()));
  static_assert(std::is_default_constructible<R>::value,
                "result must be default constructible");
  std::vector<R> result;
  result.resize(vec.size());

  for (int64_t i = 0; i < (int64_t)vec.size(); i++) {
    result[i] = f(i, vec[i]);
  }

  return result;
}

// When going out of scope, wait for the given thread.
struct ThreadJoiner {
  explicit ThreadJoiner(std::thread *t) : t(t) {}
  ~ThreadJoiner() {
    t->join();
  }
  std::thread *t;
};


namespace internal {

// number of arguments that f takes, at compile time.
template <typename T>
struct get_arity : get_arity<decltype(&T::operator())> {};

template <typename R, typename... Args>
struct get_arity<R(*)(Args...)> :
  std::integral_constant<unsigned, sizeof...(Args)> {};

template <typename R, typename C, typename... Args>
struct get_arity<R(C::*)(Args...)> :
  std::integral_constant<unsigned, sizeof...(Args)> {};
template <typename R, typename C, typename... Args>
struct get_arity<R(C::*)(Args...) const> :
  std::integral_constant<unsigned, sizeof...(Args)> {};

}

// Calls f(x, y) for each 0 <= x < num1 and 0 <= y < num2,
// in up to max_concurrency parallel threads.
// No guarantees about the order that these are run, of course,
// but it attempts to treat x as the minor axis and y as the
// major.
template<class F>
void ParallelComp2D(int64_t num1, int64_t num2,
                    const F &f,
                    int max_concurrency) {
  const int64_t total_num = num1 * num2;
  ParallelComp(total_num,
               [&f, num1](int64_t x) {
                 const int64_t x1 = x % num1;
                 const int64_t x2 = x / num1;
                 f(x1, x2);
               },
               max_concurrency);
}

template<class F>
void UnParallelComp2D(int64_t num1, int64_t num2,
                      const F &f,
                      int max_concurrency_) {
  printf("%lld x %lld\n", num1, num2);
  for (int64_t y = 0; y < num2; y++) {
    for (int64_t x = 0; x < num1; x++) {
      (void)f(x, y);
    }
  }
}

// An n-dimensional parallel comprehension. Argument gives the radix
// of each dimension, and f is called on an array of size N.
//
// If f takes three arguments, then additionally the current index
// and the total indices are passed: f(args, num, den);
template<size_t N, typename F>
void ParallelCompND(
    const std::array<int64_t, N> &dims,
    const F &f,
    int max_concurrency) {
  static constexpr size_t NUM_ARGS =
    internal::get_arity<F>{};

  static_assert(NUM_ARGS == 1 || NUM_ARGS == 3,
                "The function passed to ParallelCompND "
                "should take 1 or 3 arguments.");

  int64_t total_num = 1;
  for (int64_t d : dims) total_num *= d;
  for ([[maybe_unused]] int64_t d : dims) {
    assert(d != 0);
  }
  ParallelComp(total_num,
               [&f, &dims, total_num](const int64_t idx) {
                 std::array<int64_t, N> arg;
                 int64_t x = idx;
                 for (int i = N - 1; i >= 0; i--) {
                   arg[i] = x % dims[i];
                   x /= dims[i];
                 }
                 if constexpr (NUM_ARGS == 1) {
                   f(std::move(arg));
                 } else if constexpr (NUM_ARGS == 3) {
                   f(std::move(arg), idx, total_num);
                 }
               },
               max_concurrency);
}

template<class F>
void ParallelComp3D(int64_t num1, int64_t num2, int64_t num3,
                    const F &f,
                    int max_concurrency) {
  const int64_t total_num = num1 * num2 * num3;
  ParallelComp(total_num,
               [&f, num2, num3](int64_t x) {
                 // TODO: Arrange this in row-major order as well.
                 const int64_t x3 = x % num3;
                 const int64_t xx = x / num3;
                 const int64_t x2 = xx % num2;
                 const int64_t x1 = xx / num2;
                 f(x1, x2, x3);
               },
               max_concurrency);
}


// Run exactly num_threads copies of f, each getting its thread id.
template<class F>
void ParallelFan(int num_threads, const F &f) {
  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([i, &f]() { (void)f(i); });
  }
  // Now just wait for them all to finish.
  for (std::thread &t : threads) t.join();
}

// Manages running up to X asynchronous tasks in separate threads.
// This is intended for use in situations like compressing and writing
// a bunch of frames of a movie out to disk in a loop. There's
// substantial parallelism opportunity, but it can be bad if we
// eaglery generate the frames because they might fill the entire
// memory. This automatically throttles once the specified level of
// parallelism is reached, by waiting until there is at least one
// thread free before returning from Run.
//
// PERF: This could probably use a thread pool instead of spawning
// for each task.
struct Asynchronously {
  explicit Asynchronously(int max_threads) : max_threads(max_threads) {}

  // Wait until we have thread budget, run the function
  // asynchronously, and return.
  void Run(std::function<void()> f) {
    {
      std::unique_lock<std::mutex> ul(m);
      // Wait (without the lock) until we have thread budget.
      cond.wait(ul, [this]{ return threads_active < max_threads; });
      threads_active++;
    }

    // Run the function asynchronously, without the lock.
    std::thread t{[this, f]() {
        f();
        {
          std::unique_lock<std::mutex> ul(m);
          threads_active--;
        }
        // This may activate one waiting call (or the destructor).
        cond.notify_one();
      }};
    t.detach();
  }

  // Wait until all threads have finished.
  void Wait() {
    std::unique_lock<std::mutex> ul(m);
    cond.wait(ul, [this]{ return threads_active == 0; });
  }

  ~Asynchronously() {
    Wait();
  }

 private:
  std::mutex m;
  std::condition_variable cond;
  int threads_active = 0;
  const int max_threads;
};

#endif

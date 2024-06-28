
#ifndef _BASE_DO_NOT_OPTIMIZE_H
#define _BASE_DO_NOT_OPTIMIZE_H

// This mess of macros just defines
//    DoNotOptimize(exp);
// which tries its best to inhibit the compiler from optimizing
// away the result of "exp". This is generally used when profiling
// or benchmarking.
//
// Excerpted from google/benchmark. Apache 2.0 license.
// https://github.com/google/benchmark/blob/main/LICENSE

#if defined(__GNUC__) || defined(__clang__)
#define BENCHMARK_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER) && !defined(__clang__)
#define BENCHMARK_ALWAYS_INLINE __forceinline
#define __func__ __FUNCTION__
#else
#define BENCHMARK_ALWAYS_INLINE
#endif

#define BENCHMARK_INTERNAL_TOSTRING2(x) #x
#define BENCHMARK_INTERNAL_TOSTRING(x) BENCHMARK_INTERNAL_TOSTRING2(x)

#if (defined(__GNUC__) && !defined(__NVCC__) && !defined(__NVCOMPILER)) || defined(__clang__)
#define BENCHMARK_BUILTIN_EXPECT(x, y) __builtin_expect(x, y)
#define BENCHMARK_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#define BENCHMARK_DISABLE_DEPRECATED_WARNING \
  _Pragma("GCC diagnostic push")             \
  _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define BENCHMARK_RESTORE_DEPRECATED_WARNING _Pragma("GCC diagnostic pop")
#elif defined(__NVCOMPILER)
#define BENCHMARK_BUILTIN_EXPECT(x, y) __builtin_expect(x, y)
#define BENCHMARK_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#define BENCHMARK_DISABLE_DEPRECATED_WARNING \
  _Pragma("diagnostic push") \
  _Pragma("diag_suppress deprecated_entity_with_custom_message")
#define BENCHMARK_RESTORE_DEPRECATED_WARNING _Pragma("diagnostic pop")
#else
#define BENCHMARK_BUILTIN_EXPECT(x, y) x
#define BENCHMARK_DEPRECATED_MSG(msg)
#define BENCHMARK_WARNING_MSG(msg)                           \
  __pragma(message(__FILE__ "(" BENCHMARK_INTERNAL_TOSTRING( \
      __LINE__) ") : warning note: " msg))
#define BENCHMARK_DISABLE_DEPRECATED_WARNING
#define BENCHMARK_RESTORE_DEPRECATED_WARNING
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif


namespace internal {
extern void UseCharPointer(char const volatile*);
}

// Force the compiler to flush pending writes to global memory. Acts as an
// effective read/write barrier
#ifdef BENCHMARK_HAS_CXX11
inline BENCHMARK_ALWAYS_INLINE void ClobberMemory() {
  std::atomic_signal_fence(std::memory_order_acq_rel);
}
#endif

// The DoNotOptimize(...) function can be used to prevent a value or
// expression from being optimized away by the compiler. This function is
// intended to add little to no overhead.
// See: https://youtu.be/nXaxk27zwlk?t=2441
#ifndef BENCHMARK_HAS_NO_INLINE_ASSEMBLY
#if !defined(__GNUC__) || defined(__llvm__) || defined(__INTEL_COMPILER)
template <class Tp>
BENCHMARK_DEPRECATED_MSG(
    "The const-ref version of this method can permit "
    "undesired compiler optimizations in benchmarks")
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

#ifdef BENCHMARK_HAS_CXX11
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp&& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}
#endif
#elif defined(BENCHMARK_HAS_CXX11) && (__GNUC__ >= 5)
// Workaround for a bug with full argument copy overhead with GCC.
// See: #1340 and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105519
template <class Tp>
BENCHMARK_DEPRECATED_MSG(
    "The const-ref version of this method can permit "
    "undesired compiler optimizations in benchmarks")
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp const& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
BENCHMARK_DEPRECATED_MSG(
    "The const-ref version of this method can permit "
    "undesired compiler optimizations in benchmarks")
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!std::is_trivially_copyable<Tp>::value ||
                            (sizeof(Tp) > sizeof(Tp*))>::type
    DoNotOptimize(Tp const& value) {
  asm volatile("" : : "m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp& value) {
  asm volatile("" : "+m,r"(value) : : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!std::is_trivially_copyable<Tp>::value ||
                            (sizeof(Tp) > sizeof(Tp*))>::type
    DoNotOptimize(Tp& value) {
  asm volatile("" : "+m"(value) : : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<std::is_trivially_copyable<Tp>::value &&
                            (sizeof(Tp) <= sizeof(Tp*))>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : "+m,r"(value) : : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE
    typename std::enable_if<!std::is_trivially_copyable<Tp>::value ||
                            (sizeof(Tp) > sizeof(Tp*))>::type
    DoNotOptimize(Tp&& value) {
  asm volatile("" : "+m"(value) : : "memory");
}

#else
// Fallback for GCC < 5. Can add some overhead because the compiler is forced
// to use memory operations instead of operations with registers.
// TODO: Remove if GCC < 5 will be unsupported.
template <class Tp>
BENCHMARK_DEPRECATED_MSG(
    "The const-ref version of this method can permit "
    "undesired compiler optimizations in benchmarks")
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const& value) {
  asm volatile("" : : "m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp& value) {
  asm volatile("" : "+m"(value) : : "memory");
}

#ifdef BENCHMARK_HAS_CXX11
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp&& value) {
  asm volatile("" : "+m"(value) : : "memory");
}
#endif
#endif

#ifndef BENCHMARK_HAS_CXX11
inline BENCHMARK_ALWAYS_INLINE void ClobberMemory() {
  asm volatile("" : : : "memory");
}
#endif
#elif defined(_MSC_VER)
template <class Tp>
BENCHMARK_DEPRECATED_MSG(
    "The const-ref version of this method can permit "
    "undesired compiler optimizations in benchmarks")
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const& value) {
  internal::UseCharPointer(&reinterpret_cast<char const volatile&>(value));
  _ReadWriteBarrier();
}

#ifndef BENCHMARK_HAS_CXX11
inline BENCHMARK_ALWAYS_INLINE void ClobberMemory() { _ReadWriteBarrier(); }
#endif
#else
#ifdef BENCHMARK_HAS_CXX11
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp&& value) {
  internal::UseCharPointer(&reinterpret_cast<char const volatile&>(value));
}
#else
template <class Tp>
BENCHMARK_DEPRECATED_MSG(
    "The const-ref version of this method can permit "
    "undesired compiler optimizations in benchmarks")
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const& value) {
  internal::UseCharPointer(&reinterpret_cast<char const volatile&>(value));
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp& value) {
  internal::UseCharPointer(&reinterpret_cast<char const volatile&>(value));
}
#endif

#error DoNotOptimize is not available!

#endif

#endif

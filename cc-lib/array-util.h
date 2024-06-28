
#ifndef _CC_LIB_ARRAY_UTIL_H
#define _CC_LIB_ARRAY_UTIL_H

#include <cstddef>
#include <array>

template<class F, class I, size_t N>
auto MapArray(const F &f,
              const std::array<I, N> &a) ->
  std::array<decltype(f(a[0])), N> {
  using O = decltype(f(a[0]));
  std::array<O, N> out;
  for (size_t i = 0; i < N; i++) {
    out[i] = f(a[i]);
  }
  return out;
}

#endif

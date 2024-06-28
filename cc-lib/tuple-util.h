
#ifndef _CC_LIB_TUPLE_UTIL_H
#define _CC_LIB_TUPLE_UTIL_H

#include <tuple>
#include <utility>
#include <cstdint>
#include <cstdlib>

namespace internal {
template<class F, class T, size_t... I>
auto MapTupleInternal(const F &f,
                      const T &t,
                      std::index_sequence<I...>) {
  // Expands the expression for each int.
  return std::make_tuple(f(std::get<I>(t))...);
}
}

template<class F, class... TS>
auto MapTuple(const F &f,
              const std::tuple<TS...> &t) {
  return internal::MapTupleInternal(
      f, t, std::make_index_sequence<sizeof...(TS)>{});
}

#endif

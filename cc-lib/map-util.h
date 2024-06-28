
#ifndef _CC_LIB_MAP_UTIL_H
#define _CC_LIB_MAP_UTIL_H

#include <algorithm>
#include <utility>
#include <vector>

// This returns a copy of the value (to avoid danging references to
// the default) so it should generally be used when def is a small
// value.
template<class M>
inline auto FindOrDefault(const M &m, const typename M::key_type &k,
                          typename M::mapped_type def) ->
  typename M::mapped_type {
  auto it = m.find(k);
  if (it == m.end()) return def;
  else return it->second;
}

template<class M>
inline auto MapToSortedVec(const M &m) ->
  std::vector<std::pair<typename M::key_type, typename M::mapped_type>> {
  using K = typename M::key_type;
  using V = typename M::mapped_type;
  std::vector<std::pair<K, V>> vec;
  vec.reserve(m.size());
  for (const auto &[k, v] : m) vec.emplace_back(k, v);
  std::sort(vec.begin(), vec.end(),
            [](const std::pair<K, V> &a,
               const std::pair<K, V> &b) {
              return a.first < b.first;
            });
  return vec;
}

#endif

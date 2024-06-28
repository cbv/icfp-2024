
#ifndef _CC_LIB_SET_UTIL_H
#define _CC_LIB_SET_UTIL_H

#include <algorithm>
#include <vector>
#include <unordered_set>
#include <type_traits>

// e.g.
// std::unordered_set<int64_t> s;
// std::vector<int64_t> v = ToSortedVec(s);
//
// TODO: Could take less-than operator, but a really complex utility
// may be defeating the purpose.
template<class S>
auto SetToSortedVec(const S &s) ->
  std::vector<std::remove_cvref_t<decltype(*s.begin())>> {
  using T = std::remove_cvref_t<decltype(*s.begin())>;
  std::vector<T> ret;
  ret.reserve(s.size());
  for (const auto &elt : s)
    ret.push_back(elt);
  std::sort(ret.begin(), ret.end());
  return ret;
}


#endif

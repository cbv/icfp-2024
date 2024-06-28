
#ifndef _CC_LIB_FUNCTIONAL_SET_H
#define _CC_LIB_FUNCTIONAL_SET_H

#include <unordered_set>
#include <functional>

#include "functional-map.h"
#include "hashing.h"

// This is a simple wrapper around FunctionalMap for the case that
// the value type is uninteresting.
//
// Value semantics.
template<
  class Key_,
  // These must be efficiently default-constructible.
  class Hash_ = Hashing<Key_>,
  class KeyEqual_ = std::equal_to<Key_>>
struct FunctionalSet {
  using Key = Key_;
  using Hash = Hash_;
  using KeyEqual = KeyEqual_;

  // Empty.
  FunctionalSet() {}
  // Value semantics.
  FunctionalSet(const FunctionalSet &other) = default;
  FunctionalSet(FunctionalSet &&other) = default;
  FunctionalSet &operator =(const FunctionalSet &other) = default;
  FunctionalSet &operator =(FunctionalSet &&other) = default;

  bool Contains(const Key &k) const {
    return m.FindPtr(k) != nullptr;
  }

  FunctionalSet Insert(const Key &k) const {
    return FunctionalSet(m.Insert(k, {}));
  }

  // i.e., union
  FunctionalSet Insert(const FunctionalSet &other) const {
    FunctionalSet ret = *this;
    for (const Key &k : other.Export()) {
      ret = ret.Insert(k);
    }
    return ret;
  }

  // Initialize with a set of bindings.
  FunctionalSet(const std::vector<Key> &vec) : m(AddUnit(vec)) {}

  // Flatten to a single map, where inner bindings shadow outer
  // ones as expected. Copies the whole map, so this is linear time.
  std::unordered_set<Key, Hash, KeyEqual>
  Export() const {
    std::unordered_set<Key, Hash, KeyEqual> ret;
    for (const auto &[k, v_] : m.Export()) {
      ret.insert(k);
    }
    return ret;
  }

 private:
  struct Unit { };
  using MapType = FunctionalMap<Key, Unit>;

  FunctionalSet(MapType &&mm) : m(mm) {}

  static const std::vector<std::pair<Key, Unit>> AddUnit(
      const std::vector<Key> &vec) {
    std::vector<std::pair<Key, Unit>> ret;
    ret.reserve(vec.size());
    for (const Key &k : vec) {
      ret.emplace_back(k, {});
    }
    return ret;
  }

  MapType m;
};

#endif

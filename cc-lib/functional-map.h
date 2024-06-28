
#ifndef _CC_LIB_FUNCTIONAL_MAP_H
#define _CC_LIB_FUNCTIONAL_MAP_H

#include <unordered_map>
#include <variant>
#include <functional>
#include <memory>

#include "hashing.h"
#include "base/logging.h"

// This is a limited functional map type. The canonical use is a
// context in a programming language implementation, which contains
// statically-scoped variables mapped to e.g. their types. It supports
// reasonably efficient copy-and-insert and lookup, and could perhaps
// be extended to include more.
//
// Values can be copied, so they should be wrapped somehow if their
// identity needs to be preserved.
//
// Value semantics. It is a shared_ptr under the hood.
template<
  class Key_, class Value_,
  // These must be efficiently default-constructible.
  class Hash_ = Hashing<Key_>,
  class KeyEqual_ = std::equal_to<Key_>>
struct FunctionalMap {
  using Key = Key_;
  using Value = Value_;
  using Hash = Hash_;
  using KeyEqual = KeyEqual_;

  // Empty.
  FunctionalMap() : depth(0) {}
  // Value semantics.
  FunctionalMap(const FunctionalMap &other) = default;
  FunctionalMap(FunctionalMap &&other) = default;
  FunctionalMap &operator =(const FunctionalMap &other) = default;
  FunctionalMap &operator =(FunctionalMap &&other) = default;

  // Or returns null if not present.
  const Value *FindPtr(const Key &k) const {
    const FunctionalMap *f = this;
    for (;;) {
      if (f->data.get() == nullptr) return nullptr;
      else if (const Cell *cell = std::get_if<Cell>(f->data.get())) {
        const auto &[kk, vv, pp] = *cell;
        if (KeyEqual()(k, kk)) {
          return &vv;
        } else {
          f = &pp;
        }
      } else {
        CHECK(std::holds_alternative<HashMap>(*f->data));
        const HashMap &m = std::get<HashMap>(*f->data);
        auto it = m.find(k);
        if (it == m.end()) return nullptr;
        else return &it->second;
      }
    }
  }

  bool Contains(const Key &k) const {
    return FindPtr(k) != nullptr;
  }

  FunctionalMap Insert(const Key &k, Value v) const {
    if (depth > kLinearDepth) {
      CHECK(!std::holds_alternative<HashMap>(*data)) << "Bug: Depth implies "
        "pointer representation.";
      HashMap m = GetAll(*this);
      m[k] = std::move(v);
      return FunctionalMap(std::move(m));
    } else {
      return FunctionalMap(
          std::make_tuple(k, std::move(v), *this),
          depth + 1);
    }
  }

  // Initialize with a set of bindings. Later bindings shadow
  // earlier ones, in the case of duplicates.
  FunctionalMap(const std::vector<std::pair<Key, Value>> &vec) {
    HashMap hm;
    for (const auto &[k, v] : vec) {
      hm[k] = v;
    }
    data = std::make_shared<variant_type>(std::move(hm));
  }

  // Flatten to a single map, where inner bindings shadow outer
  // ones as expected. Copies the whole map, so this is linear time.
  std::unordered_map<Key, Value, Hash, KeyEqual>
  Export() const {
    return GetAll(*this);
  }

  // Usually if you are using a "functional map" you want the values
  // to be immutable, but FunctionalMap also allows mutating
  // the values. Since the values are copied around, you would likely
  // need to use a value type like shared_ptr in order for this to
  // make sense.
  Value *FindPtr(const Key &k);

 private:
  using Cell = std::tuple<Key, Value, FunctionalMap>;
  using HashMap = std::unordered_map<Key, Value, Hash, KeyEqual>;
  using variant_type = std::variant<Cell, HashMap>;
  int depth = 0;

  // Every certain number of pointers, we create a cell with a hash
  // map of everything beneath it. PERF: Other ideas would work here.
  // For example, we could just flatten chunks (and still do a search
  // through a linked list of hash maps), rather than consolidating
  // the whole thing each time.
  //
  // Perhaps better would be to actually flatten the nodes in place,
  // but we have to deal with "mutable" and thread safety then.
  //
  // XXX tune this. In the insert-only case, it's much faster to
  // set this really high (unsurprising).
  static constexpr int kLinearDepth = 100;

  static HashMap GetAll(const FunctionalMap &f) {
    if (f.data.get() == nullptr) return HashMap();
    if (const Cell *cell = std::get_if<Cell>(f.data.get())) {
      const auto &[kk, vv, pp] = *cell;
      HashMap ret = GetAll(pp);
      ret[kk] = vv;
      return ret;
    } else {
      CHECK(std::holds_alternative<HashMap>(*f.data));
      return std::get<HashMap>(*f.data);
    }
  }

  explicit FunctionalMap(HashMap m) :
    depth(0), data(std::make_shared<variant_type>(std::move(m))) {}

  // Make a new list cell.
  FunctionalMap(Cell cell, int depth) :
    depth(depth), data(std::make_shared<variant_type>(cell)) {}

  // Or null means empty.
  std::shared_ptr<variant_type> data;
};


// Template implementations follow.

template <class Key, class Value, class Hash, class KeyEqual>
Value *FunctionalMap<Key, Value, Hash, KeyEqual>::FindPtr(const Key &k) {
  const FunctionalMap *f = this;
  for (;;) {
    if (f->data.get() == nullptr) {
      return nullptr;
    } else if (Cell *cell = std::get_if<Cell>(f->data.get())) {
      auto &[kk, vv, pp] = *cell;
      if (KeyEqual()(k, kk)) {
        return &vv;
      } else {
        f = &pp;
      }
    } else {
      CHECK(std::holds_alternative<HashMap>(*f->data));
      HashMap &m = std::get<HashMap>(*f->data);
      auto it = m.find(k);
      if (it == m.end())
        return nullptr;
      else
        return &it->second;
    }
  }
}

#endif

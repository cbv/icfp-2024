
// When using std::optional, the main thing you want to do is test
// whether you have a value, and if so name that value. Like this:
//
// optional<string> os;
// if (os.has_value()) {
//   const string &s = os.value();
//   ...
// }
//
// This utility allows doing this as a one-liner, using the
// range-based for construct:
//
// for (const string &s : GetOpt(os)) {
//   ...
// }
//
// The for loop runs 0 or 1 times. GCC and Clang have no problem
// realizing this, so it seems okay to treat this as zero-overhead.
// Both const and mutable overloads are provided.

#ifndef _CC_LIB_OPTIONAL_ITERATOR_H
#define _CC_LIB_OPTIONAL_ITERATOR_H

#include <optional>

namespace internal {
template<class T>
class optional_iterator final {
 public:
  optional_iterator(T *t) : t(t) {}
  optional_iterator &operator++(int dummy) {
    optional_iterator old(t);
    t = nullptr;
    return old;
  }

  optional_iterator &operator++() {
    t = nullptr;
    return *this;
  }

  T &operator*() {
    return *t;
  }

  const T &operator*() const {
    return *t;
  }

  bool operator!=(const optional_iterator &other) const {
    return t != other.t;
  }

 private:
  // nullptr = end
  T *t = nullptr;
};

// Just provides .begin() and .end() methods.
template<class T>
struct optional_container final {
  explicit optional_container(std::optional<T> &o) : o(o) {}
  optional_iterator<T> begin() {
    if (o.has_value()) return optional_iterator<T>(&o.value());
    else return optional_iterator<T>(nullptr);
  }
  optional_iterator<T> end() {
    return optional_iterator<T>(nullptr);
  }
 private:
  std::optional<T> &o;
};



template<class T>
class const_optional_iterator final {
 public:
  const_optional_iterator(const T *t) : t(t) {}
  const_optional_iterator &operator++(int dummy) {
    const_optional_iterator old(t);
    t = nullptr;
    return old;
  }

  const_optional_iterator &operator++() {
    t = nullptr;
    return *this;
  }

  const T &operator*() const {
    return *t;
  }

  bool operator!=(const const_optional_iterator &other) const {
    return t != other.t;
  }

 private:
  // nullptr = end
  const T *t = nullptr;
};

template<class T>
struct const_optional_container final {
  explicit const_optional_container(const std::optional<T> &o) : o(o) {}
  const_optional_iterator<T> begin() {
    if (o.has_value()) return const_optional_iterator<T>(&o.value());
    else return const_optional_iterator<T>(nullptr);
  }
  const_optional_iterator<T> end() {
    return const_optional_iterator<T>(nullptr);
  }
 private:
  const std::optional<T> &o;
};

}  // internal

template<class T>
inline internal::optional_container<T> GetOpt(std::optional<T> &o) {
  return internal::optional_container<T>(o);
}

template<class T>
inline internal::const_optional_container<T> GetOpt(const std::optional<T> &o) {
  return internal::const_optional_container<T>(o);
}

#endif

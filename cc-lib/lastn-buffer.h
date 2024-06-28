
#ifndef _CC_LIB_LASTN_BUFFER_H
#define _CC_LIB_LASTN_BUFFER_H

#include <vector>
#include <cstddef>

// LastNBuffer<T> is an efficient fixed-size (N elements) buffer that
// stores (only) the last N elements of type T pushed into it. It is
// always "full" with N elements. Pushing is constant time and
// accessing any element by index is also constant time. (It is
// implemented as a circular vector of size N.)
//
// T is expected to have value semantics.

template<class T>
struct LastNBuffer {
  // The buffer begins containing n copies of default_value.
  LastNBuffer(int n, T default_value);

  // If the front of the array (element 0) is at the left,
  // rotate the contents to the left so that element zero
  // becomes the final element, element 1 becomes the 0
  // element, etc.
  void RotateLeft();
  // Inverse of the above.
  void RotateRight();

  // Last element. Element zero falls off.
  void push_back(const T &t);
  // void push_back(T &&t);   // TODO
  // Element zero. An element falls off the back.
  void push_front(const T &t);
  // void push_front(T &&t); // TODO

  int size() const { return (int)data.size(); }

  T &operator [](int i) {
    return data[Wrap(zero + i)];
  }
  const T &operator [](int i) const {
    return data[Wrap(zero + i)];
  }


  template<class F>
  void App(F f) const {
    // PERF don't need to compute indices so many times
    const int n = size();
    for (int i = 0; i < n; i++) {
      f((*this)[i]);
    }
  }

  // An iterator is one of the elements, or one past the last element.
  struct iterator {
    // XXX We should be able to have random access. Need to implement:
    //   https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator
    // using iterator_category = std::random_access_iterator_tag;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type   = size_t;
    using value_type        = T;
    using pointer           = value_type *;
    using reference         = value_type &;

    reference operator*() { return (*buffer)[idx]; }
    pointer operator->() { return &(*buffer)[idx]; }

    iterator& operator++() { idx++; return *this; }
    iterator operator++(int postfix_) {
      iterator tmp = *this; ++(*this); return tmp;
    }

    // These don't compare the buffer itself, since it would be ub
    // to compare iterators from two different containers.
    friend bool operator== (const iterator& a, const iterator& b) {
      return a.idx == b.idx;
    }
    friend bool operator!= (const iterator& a, const iterator& b) {
      return a.idx != b.idx;
    }

    iterator(LastNBuffer *buffer, size_t idx) : buffer(buffer), idx(idx) {}
  private:
    // To avoid oddities (e.g. difference_type) that I don't fully
    // understand, iterators are done with indices.
    // PERF: It can probably done with bare pointers, although the
    // iterator struct still needs to know where to know how to wrap
    // around.
    LastNBuffer *buffer = nullptr;
    size_t idx = 0;
  };

  iterator begin() { return iterator(this, 0); }
  iterator end() { return iterator(this, size()); }
  // TODO: Const iterators.

 private:
  // Like idx % n, but only works when idx is in [0, 2 * n - 1]. This
  // is sufficient because "zero" is always in [0, n - 1].
  inline int Wrap(int idx) const {
    const int n = size();
    return (idx >= n) ? idx - n : idx;
  }
  // Always in [0, n - 1].
  int zero = 0;
  // Fixed size. Maybe could avoid the overhead of vector.
  std::vector<T> data;
};

// Template implementations follow.
template<class T>
LastNBuffer<T>::LastNBuffer(int n, T default_value) :
  zero(0), data(n, default_value) {}

template<class T>
void LastNBuffer<T>::RotateLeft() {
  zero = Wrap(zero + 1);
}

template<class T>
void LastNBuffer<T>::RotateRight() {
  zero--;
  if (zero < 0) zero += size();
}

template<class T>
void LastNBuffer<T>::push_back(const T &t) {
  data[zero] = t;
  RotateLeft();
}

template<class T>
void LastNBuffer<T>::push_front(const T &t) {
  RotateRight();
  data[zero] = t;
}

#endif

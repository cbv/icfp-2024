
// Generic hashing for use in e.g. std::unordered_map. Not guaranteed to
// be stable over time.
//
// It is not allowed to extend std::hash (or anything in std::) unless
// the declaration involves a user-defined type, so the absence of
// std::hash on containers like std::pair is pretty annoying.
//
// Use like:
//  using HashMap =
//     std::unordered_map<std::pair<int, std::string>, int,
//                        Hashing<std::pair<int, std::string>>;

#ifndef _CC_LIB_HASHING_H
#define _CC_LIB_HASHING_H

#include <utility>
#include <vector>
#include <tuple>
#include <array>
#include <cstddef>
#include <bit>

template<class T>
struct Hashing;

namespace hashing_internal {

template<size_t IDX, typename... Ts>
struct HashTupleRec {
  using tuple_type = std::tuple<Ts...>;
  std::size_t operator()(size_t h, const tuple_type &t) const {
    if constexpr (IDX == sizeof... (Ts)) {
      return h;
    } else {
      const auto &elt = std::get<IDX>(t);
      Hashing<std::remove_cvref_t<decltype(elt)>> hashing;
      size_t hh = hashing(elt);
      return HashTupleRec<IDX + 1, Ts...>()(
          std::rotl<size_t>(h, (IDX * 7 + 1) % 31) + hh,
          t);
    }
  }
};
}  // namespace hashing_internal

// Forward anything that already has std::hash overloaded for it.
template<class T>
struct Hashing {
  // TODO: Better to use perfect forwarding here?
  // It of course gave me an inscrutable error when I tried.
  std::size_t operator()(const T &t) const {
    return std::hash<T>()(t);
  }
};

template<class T, class U>
struct Hashing<std::pair<T, U>> {
  std::size_t operator()(const std::pair<T, U> &p) const {
    size_t th = Hashing<T>()(p.first), uh = Hashing<U>()(p.second);
    // This can certainly be improved. Keep in mind that size_t
    // is commonly either 32 or 64 bits.
    return th + 0x9e3779b9 + std::rotl<size_t>(uh, 15);
  }
};


// PERF: Probably should at least specialize for numeric types like
// uint8_t.
template<class T>
struct Hashing<std::vector<T>> {
  std::size_t operator()(const std::vector<T> &v) const {
    size_t h = 0xCAFED00D + v.size();
    for (const T &t : v) {
      h += Hashing<T>()(t);
      h = std::rotl<size_t>(h, 13);
    }
    return h;
  }
};

template<class T, size_t N>
struct Hashing<std::array<T, N>> {
  std::size_t operator()(const std::array<T, N> &v) const {
    size_t h = 0xDECADE00 + N;
    for (const T &t : v) {
      h += Hashing<T>()(t);
      h = std::rotl<size_t>(h, 17);
    }
    return h;
  }
};

template<typename... Ts>
struct Hashing<std::tuple<Ts...>> {
  std::size_t operator()(const std::tuple<Ts...> &t) const {
    return hashing_internal::HashTupleRec<0, Ts...>()(0, t);
  }
};


#endif

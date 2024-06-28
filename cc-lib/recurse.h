
#ifndef _CC_LIB_RECURSE_H
#define _CC_LIB_RECURSE_H

#include <functional>
#include <type_traits>

#include "base/logging.h"

// Make "normal" recursive functions with a knot-tying "Y combinator."
// The idea behind this is that we could maybe reduce the stack usage
// for recursive functions, but that's not really implemented (so there's
// no point other than demonstrating the technique).

// F is a function taking Self, Arg1, Arg2, ... and returning Ret.
// Self is a function that takes Arg1, Arg2, ... and returns Ret.

// Use like this (for an example with two args):
//
// auto Rec = Recursive<Ret, Arg1, Arg2>([&](const std::function<Ret(Arg1, Arg2)> &self,
//                                           Arg1 arg1, Arg2 Arg2) {
//    ... self(b1, b2) ...
// });
//
// Ret r = Rec(c1, c2);

template<class Ret, typename... Args>
struct Recursive {
  using Self = std::function<Ret(Args...)>;
  using F = std::function<Ret(const Self &, Args...)>;
  explicit Recursive(const F &f) : f(f) {}

  Ret operator()(Args... args) const {
    return f(
        Self([this](Args... args) {
            return this->operator ()(args...); }),
        std::forward<Args>(args)...);
  };

  F f;
};

#endif

/* Parser combinators by Tom 7.
   I got fed up! */

#ifndef _CC_LIB_PARSE_H
#define _CC_LIB_PARSE_H

#include <optional>
#include <functional>
#include <concepts>
#include <utility>
#include <deque>
#include <bit>

#include "base/logging.h"

struct Unit { };

// Like std::span<T>, but rooted at the beginning of the token
// stream, so that we can ask "what position is this in the
// input file"? for a subspan. Const view.
template<class T>
struct TokenSpan {
  // The full span.
  TokenSpan(const T *data, size_t length) : root(data), full_length(length),
                                            offset(0), length(length) { }
  // An empty span referring to nothing.
  TokenSpan() {}

  // Value semantics.
  TokenSpan(const TokenSpan &other) = default;
  TokenSpan &operator =(const TokenSpan &other) = default;

  T operator[](size_t idx) const {
    CHECK(idx < length);
    return root[offset + idx];
  }

  bool empty() const {
    return length == 0;
  }

  size_t StartOffset() const {
    return offset;
  }

  size_t Size() const {
    return length;
  }

  TokenSpan SubSpan(size_t start) const {
    CHECK(start <= length);
    return TokenSpan(
      root, full_length,
      offset + start, length - start);
  }

  TokenSpan SubSpan(size_t start, size_t len) const {
    return SubSpan(start).First(len);
  }

  TokenSpan First(size_t len) const {
    CHECK(len <= length);
    return TokenSpan(
      root, full_length,
      offset, len);
  }

  struct ShallowHash {
    // Hash code for the span, which distinguishes spans
    // with the same data but different roots.
    size_t operator()(const TokenSpan &s) const {
      uint64_t hh = (uint64_t)s.root;
      hh += s.offset;
      return std::rotr(hh, 55) ^ s.length;
    }
  };

  struct ShallowEq {
    // Shallow equality; constant time.
    bool operator ()(const TokenSpan &a,
                     const TokenSpan &b) const {
      return a.root == b.root &&
        a.full_length == b.full_length &&
        a.offset == b.offset &&
        a.length == b.length;
    }
  };

 private:
  TokenSpan(const T *root, size_t full_length,
            size_t offset, size_t length) :
    root(root), full_length(full_length),
    offset(offset), length(length) { }

  // The input data, e.g. the bytes of the source file being
  // parsed.
  const T *root = nullptr;
  size_t full_length = 0;
  // The subspan. As a representation invariant, this always
  // refers to some data in [root, root + full_length).
  size_t offset = 0;
  size_t length = 0;
};

// Like std::optional<T>, but with the length of the match
// (number of tokens at the start of the span).
template<class T>
struct Parsed {
  constexpr Parsed() {}
  static constexpr Parsed<T> None() { return Parsed(); }
  Parsed(T t, size_t length) : ot_(t), length_(length) {}
  bool HasValue() const { return ot_.has_value(); }
  const T &Value() const { return ot_.value(); }
  size_t Length() const { return length_; }
private:
  std::optional<T> ot_;
  size_t length_ = 0;
};

template <typename P>
concept Parser = requires(P p, TokenSpan<typename P::token_type> toks) {
  typename P::token_type;
  typename P::out_type;
  { p(toks) } -> std::convertible_to<Parsed<typename P::out_type>>;
};

template <typename Token, typename Out>
struct ParserWrapper {
  template <typename F>
  ParserWrapper(F&& f) : func(f) {}

  Parsed<Out> operator ()(TokenSpan<Token> t) const {
    return func(t);
  }

  using token_type = Token;
  using out_type = Out;

 private:
  std::function<Parsed<Out>(TokenSpan<Token>)> func;
};

static_assert(Parser<ParserWrapper<char, double>>);

// Always fails to parse.
template<class Token, class Out>
struct Fail {
  using token_type = Token;
  using out_type = Out;
  Parsed<Out> operator ()(TokenSpan<Token> unused_) const {
    return Parsed<Out>::None();
  }
};

// Always succeeds, consuming no tokens and returning the given
// output.
template<class Token, class Out>
struct Succeed {
  using token_type = Token;
  using out_type = Out;
  explicit Succeed(Out r) : r(r) {}
  Parsed<Out> operator ()(TokenSpan<Token> unused_) const {
    // consuming no input
    return Parsed(r, 0);
  }
  const Out r;
};

template<class Token>
struct End {
  using token_type = Token;
  using out_type = Unit;
  explicit End() {}
  Parsed<Unit> operator ()(TokenSpan<Token> toks) const {
    if (toks.empty()) return Parsed(Unit{}, 0);
    return Parsed<Unit>::None();
  }
};

// Matches any token; returns it.
template<class Token>
struct Any {
  using token_type = Token;
  using out_type = Token;
  explicit Any() {}
  Parsed<Token> operator ()(TokenSpan<Token> toks) const {
    if (toks.empty()) return Parsed<Token>::None();
    return Parsed(toks[0], 1);
  }
};

// Matches only the argument token.
template<class Token>
struct Is {
  using token_type = Token;
  using out_type = Token;
  explicit Is(Token r) : r(r) {}
  Parsed<Token> operator ()(TokenSpan<Token> toks) const {
    if (toks.empty()) return Parsed<Token>::None();
    if (toks[0] == r) return Parsed(toks[0], 1);
    else return Parsed<Token>::None();
  }
  const Token r;
};

// Expands the length of a parsed result forward by the given amount.
// If the parse failed, also fails.
template<class Out>
Parsed<Out> ExpandLength(Parsed<Out> o, size_t len) {
  if (o.HasValue()) {
    return Parsed<Out>(o.Value(), o.Length() + len);
  } else {
    return o;
  }
}

// A >> B
// parses A then B, and fails if either fails.
// Both must have the same input type.
// If successful, the result is the result of B.
template<Parser A, Parser B>
requires std::same_as<typename A::token_type,
                      typename B::token_type>
inline auto operator >>(const A &a, const B &b) {
  using in = A::token_type;
  using out = B::out_type;
  return ParserWrapper<in, out>(
      [a, b](TokenSpan<in> toks) -> Parsed<out> {
        auto o1 = a(toks);
        if (!o1.HasValue()) return Parsed<out>::None();
        const size_t start = o1.Length();
        auto o2 = b(toks.SubSpan(start));
        return ExpandLength(o2, start);
      });
}

// A << B
// As above, but the result on success is the result of A.
template<Parser A, Parser B>
requires std::same_as<typename A::token_type,
                      typename B::token_type>
inline auto operator <<(const A &a, const B &b) {
  using in = A::token_type;
  using out = A::out_type;
  return ParserWrapper<in, out>(
      [a, b](TokenSpan<in> toks) -> Parsed<out> {
        auto o1 = a(toks);
        if (!o1.HasValue()) return Parsed<out>::None();
        const size_t start = o1.Length();
        auto o2 = b(toks.SubSpan(start));
        if (!o2.HasValue()) return Parsed<out>::None();
        return ExpandLength(o1, o2.Length());
      });
}

// Parse both A and B. Produce a pair of the results.
template<Parser A, Parser B>
requires std::same_as<typename A::token_type,
                      typename B::token_type>
inline auto operator &&(const A &a,
                        const B &b) {
  using in = A::token_type;
  using out1 = A::out_type;
  using out2 = B::out_type;
  return ParserWrapper<in, std::pair<out1, out2>>(
      [a, b](TokenSpan<in> toks) ->
      Parsed<std::pair<out1, out2>> {
        auto o1 = a(toks);
        if (!o1.HasValue())
          return Parsed<std::pair<out1, out2>>::None();
        const size_t start = o1.Length();
        auto o2 = b(toks.SubSpan(start));
        if (!o2.HasValue())
          return Parsed<std::pair<out1, out2>>::None();
        return Parsed<std::pair<out1, out2>>(
            std::make_pair(o1.Value(), o2.Value()),
            start + o2.Length());
      });
}

template<Parser A, Parser B>
requires std::same_as<typename A::token_type,
                      typename B::token_type> &&
         std::same_as<typename A::out_type,
                      typename B::out_type>
inline auto operator ||(const A &a, const B &b) {
  using in = A::token_type;
  using out = A::out_type;
  return ParserWrapper<in, out>(
      [a, b](TokenSpan<in> toks) ->
      Parsed<out> {
        auto o1 = a(toks);
        if (o1.HasValue()) {
          return o1;
        } else {
          return b(toks);
        }
      });
}

// One or zero. Always succeeds with std::optional<>.
template<Parser A>
inline auto Opt(const A &a) {
  using in = A::token_type;
  using out = std::optional<typename A::out_type>;
  return ParserWrapper<in, out>(
      [a](TokenSpan<in> toks) ->
      Parsed<out> {
        auto o = a(toks);
        if (!o.HasValue())
          return Parsed<out>(std::nullopt, 0);
        return Parsed<out>(
            std::make_optional(o.Value()),
            o.Length());
      });
}

// If successful, pair the result with its position,
// which is given as a start position and length (in tokens)
// from the original token stream (its root).
template<Parser A>
inline auto Mark(const A &a) {
  using in = A::token_type;
  using out = std::tuple<typename A::out_type, size_t, size_t>;
  return ParserWrapper<in, out>(
      [a](TokenSpan<in> toks) ->
      Parsed<out> {
        auto o = a(toks);
        if (!o.HasValue())
          return Parsed<out>::None();

        return Parsed<out>(
            std::make_tuple(o.Value(), toks.StartOffset(), o.Length()),
            o.Length());
      });
}


// Zero or more times.
// If A accepts the empty sequence, this will
// loop forever.
template<Parser A>
inline auto operator *(const A &a) {
  using in = A::token_type;
  using out = std::vector<typename A::out_type>;
  return ParserWrapper<in, out>(
      [a](TokenSpan<in> toks) ->
      Parsed<out> {
        std::vector<typename A::out_type> ret;
        size_t total_len = 0;
        for (;;) {
          auto o = a(toks);
          if (!o.HasValue())
            return Parsed<out>(ret, total_len);
          const size_t one_len = o.Length();
          total_len += one_len;
          ret.push_back(o.Value());
          toks = toks.SubSpan(one_len);
        }
      });
}

// A >f
// Maps the function f to the result of A if
// it was successful.
template<Parser A, class F>
requires std::invocable<F, typename A::out_type>
inline auto operator >(const A &a, const F &f) {
  using in = A::token_type;
  using a_out = A::out_type;
  // Get the result type of applying f to the result of a.
  using out = decltype(f(std::declval<a_out>()));
  return ParserWrapper<in, out>(
      [a, f](TokenSpan<in> toks) ->
      Parsed<out> {
        auto o = a(toks);
        if (!o.HasValue()) return Parsed<out>::None();
        else return Parsed<out>(f(o.Value()), o.Length());
      });
}

template <typename T>
concept IsOptional = std::is_constructible_v<std::optional<T>, T>;

// A /= f
// Maps the function f to the result of A, if it
// was successful. f must return std::optional<>, which determines
// whether the whole parse succeeds.
template<Parser A, class F>
requires std::invocable<F, typename A::out_type> &&
         IsOptional<std::invoke_result_t<F, typename A::out_type>>
inline auto operator /=(const A &a, const F &f) {
  using in = A::token_type;
  using a_out = A::out_type;
  // Get the result type of applying f to the result of a.
  using out = decltype(f(std::declval<a_out>()))::value_type;
  return ParserWrapper<in, out>(
      [a, f](TokenSpan<in> toks) ->
      Parsed<out> {
        auto o = a(toks);
        if (!o.HasValue()) return Parsed<out>::None();
        auto fo = f(o.Value());
        if (!fo.has_value()) return Parsed<out>::None();
        else return Parsed<out>(fo.value(), o.Length());
      });
}


#if 0
// A >=f
// Calls f on the result of A, whether successful or not.
template<Parser A, class F>
requires std::invocable<F, Parsed<typename A::out_type>>
inline auto operator >=(const A &a, const F &f) {
  using in = A::token_type;
  using a_out = A::out_type;
  // Get the result type of applying f to the result of a.
  using f_arg_type = std::declval<Parsed<a_out>>();
  using out = decltype(f());
  return ParserWrapper<in, out>(
      [a, f](TokenSpan<in> toks) ->
      Parsed<out> {
        return f(a)(toks);
      });
}
#endif

// Parse one or more A; returns vector.
template<Parser A>
inline auto operator +(const A &a) {
  return (a && *a) >[](const auto &p) {
      const auto &[x, xs] = p;
      std::vector<typename A::out_type> ret;
      ret.reserve(xs.size() + 1);
      ret.push_back(x);
      for (const auto &y : xs)
        ret.push_back(y);
      return ret;
    };
}


template <Parser A>
struct MemoizedParser {
  using token_type = A::token_type;
  using out_type = A::out_type;

  using TS = TokenSpan<token_type>;

  using Table = std::unordered_map<
    TS, Parsed<out_type>, typename TS::ShallowHash, typename TS::ShallowEq>;

  MemoizedParser(A a) : parser(std::move(a)) {
    table = std::make_shared<Table>();
  }

  Parsed<out_type> operator ()(TokenSpan<token_type> t) const {
    auto it = table->find(t);
    if (it != table->end()) {
      // Already computed.
      return it->second;
    }
    auto res = parser(t);
    (*table)[t] = res;
    return res;
  }

 private:
  // We copy around parsers willy-nilly, but the memo table can be
  // big; wrap it so copies are cheap.
  std::shared_ptr<Table> table;
  A parser;
};


// Declaring this as a separate struct:
//  - Allows us to write a requires clause below
//    (here it would need to reference the class being defined)
//  - Simpler to capture the state as "this"
//  - Helps with deducing the F template arg (I don't understand
//    this part).
// TODO: I had to fiddle with this to get it to compile with g++
// and clang. This version is simple but it might not be the
// most robust?
template<class Token, class Out, class F>
struct RecursiveParser {
  using token_type = Token;
  using out_type = Out;

  RecursiveParser(RecursiveParser &&other) = default;
  RecursiveParser(const RecursiveParser &other) = default;
  RecursiveParser &operator=(const RecursiveParser &other) = default;

  /*
  template<class F_>
  RecursiveParser(F_ &&f_) : f(std::forward<F_>(f_)) {}
  */
  RecursiveParser(const F &f) : f(f) {}

  Parsed<Out> operator()(TokenSpan<Token> toks) const {
    return f(*this)(toks);
  }
private:
  F f;
};

template<class Token, class Out, class F>
requires std::invocable<F, RecursiveParser<Token, Out, F>>
inline auto Fix(const F &f) {
  return RecursiveParser<Token, Out, F>(f);
};

template<bool MEMOIZE,
         class Token, class Out1, class Out2, class F1, class F2>
struct RecursiveParsers2 {

  template<class F1_, class F2_>
  RecursiveParsers2(F1_ &&f1_in, F2_ &&f2_in) :
    f1(std::forward<F1_>(f1_in)),
    f2(std::forward<F2_>(f2_in)),
    p1(MEMOIZE ?
       ParserWrapper<Token, Out1>(
           MemoizedParser(
               ParserWrapper<Token, Out1>(
                   [this](TokenSpan<Token> toks) {
                     return this->f1(p1, p2)(toks);
                   }))) :
       ParserWrapper<Token, Out1>(
           [this](TokenSpan<Token> toks) {
             return this->f1(p1, p2)(toks);
           })),

    p2(MEMOIZE ?
       ParserWrapper<Token, Out2>(
           MemoizedParser(
               ParserWrapper<Token, Out2>(
                   [this](TokenSpan<Token> toks) {
                     return this->f2(p1, p2)(toks);
                   }))) :
       ParserWrapper<Token, Out2>(
           [this](TokenSpan<Token> toks) {
        return this->f2(p1, p2)(toks);
      })) {

  }

  static constexpr size_t tuple_size = 2;

  ParserWrapper<Token, Out1> Get1() const { return p1; }
  ParserWrapper<Token, Out2> Get2() const { return p2; }

  template <std::size_t Idx>
  auto get() const {
    if constexpr (Idx == 0) return p1;
    else if constexpr (Idx == 1) return p2;
    else throw std::out_of_range("Invalid index");
  }

private:
  int padding1, padding2, paddgin3;
  F1 f1;
  F2 f2;
  ParserWrapper<Token, Out1> p1;
  ParserWrapper<Token, Out2> p2;
};

namespace std {
template<bool M, class Token, class Out1, class Out2, class F1, class F2>
struct tuple_size<RecursiveParsers2<M, Token, Out1, Out2, F1, F2>> :
    integral_constant<size_t, 2> {};

template<bool M, class Token, class Out1, class Out2, class F1, class F2>
struct tuple_element<0, RecursiveParsers2<M, Token, Out1, Out2, F1, F2>> {
  using type = ParserWrapper<Token, Out1>;
};

template<bool M, class Token, class Out1, class Out2, class F1, class F2>
struct tuple_element<1, RecursiveParsers2<M, Token, Out1, Out2, F1, F2>> {
  using type = ParserWrapper<Token, Out2>;
};
}

template <std::size_t Idx,
          bool M, class Token, class Out1, class Out2, class F1, class F2>
inline auto get(const RecursiveParsers2<M, Token, Out1, Out2, F1, F2> &r) {
  if constexpr (Idx == 0) return r.Get1();
  else if constexpr (Idx == 1) return r.Get2();
  else throw std::out_of_range("Invalid index");
}


// Here, f takes two parsers and returns a tuple-like object.
// const auto &[Expr, Decr] =
//   Fix2<Token, Exp *, Dec *>(
//   // expression parser
//   [](const auto &ExpParser,
//      const auto &DecParser) {
//    return ... ExpParser && DecParser ...
//   },
//   // declaration parser
//   [](const auto &ExpParser,
//      const auto &DecParser) {
//    return ... ExpParser && DecParser ...
// });

template<class Token, class Out1, class Out2, class F1, class F2>
// requires ...
inline auto Fix2(const F1 &f1, const F2 &f2) {
  return RecursiveParsers2<false, Token, Out1, Out2, F1, F2>(f1, f2);
};

template<class Token, class Out1, class Out2, class F1, class F2>
// requires ...
inline auto MemoizedFix2(const F1 &f1, const F2 &f2) {
  return RecursiveParsers2<true, Token, Out1, Out2, F1, F2>(f1, f2);
};

// Parses a b a b .... b a.
// Returns the vector of a's results.
// There must be at least one a.
template<Parser A, Parser B>
requires std::same_as<typename A::token_type,
                      typename B::token_type>
inline auto Separate(const A &a, const B &b) {
  using out = A::out_type;
  return (a && *(b >> a))
    >[](const std::tuple<out, std::vector<out>> &p) {
        const auto &[x, xs] = p;
        std::vector<out> ret;
        ret.reserve(1 + xs.size());
        ret.push_back(x);
        for (const out &y : xs) ret.push_back(y);
        return ret;
      };
}

// Same as Separate, but allowing 0 occurrences.
template<Parser A, Parser B>
requires std::same_as<typename A::token_type,
                      typename B::token_type>
inline auto Separate0(const A &a, const B &b) {
  using in = A::token_type;
  using out = A::out_type;
  return Separate(a, b) ||
    Succeed<in, std::vector<out>>(std::vector<out>{});
}


// For infix operators, directly parsing with combinators
// is not great. So the technique we typically use (from sml-lib,
// perhaps due to Okasaki?) is to parse a list of items
// (like: expressions and operators) using the combinators,
// and then resolve the infix precedence / associativity /
// adjacency using this routine, which is a little shift-reduce
// parser.
//
// Another benefit of this is that it makes it easier to
// reconfigure the infix operators at runtime, since the
// caller can decide how to dynamically wrap items at the
// time this is called.
//
// TODO: It would be nice to support n-ary operators, which
// reduce a vector of inputs (e.g. * in type expressions).
enum class Associativity {
  Left,
  Right,
  Non,
};

enum class Fixity {
  // Atoms are the leaves of the parse tree;
  // typically they are like "const Exp *" in the AST
  // being parsed.
  Atom,
  // Operators combine two nodes of the parse tree.
  Prefix,
  Infix,
  Postfix,
};

inline const char *FixityString(Fixity f) {
  switch (f) {
  case Fixity::Atom: return "Atom";
  case Fixity::Prefix: return "Prefix";
  case Fixity::Infix: return "Infix";
  case Fixity::Postfix: return "Postfix";
  default: return "ILLEGAL FIXITY";
  }
}

template<class Item>
struct FixityItem {
  Fixity fixity = Fixity::Atom;
  // For infix.
  Associativity assoc = Associativity::Non;
  // Higher means tighter binding.
  int precedence = 0;
  // For atom.
  Item item;
  // For prefix, postfix.
  std::function<Item(Item)> unop;
  // For infix.
  std::function<Item(Item, Item)> binop;

  static FixityItem MakeAtom(Item item) {
    FixityItem ret;
    ret.fixity = Fixity::Atom;
    ret.item = std::move(item);
    return ret;
  }
};

// Easier to call one of the helpers below.
template<class Out>
struct FixityResolver {
  using Item = FixityItem<Out>;
  // TODO: PERF: We can just work with pointers (or indices) into the
  // items argument, and not do so much copying.

  // The "operator" to apply to combine adjacent atoms. Used
  // to parse languages where function application is just
  // adjacency, like in ML (e.g. "f x + map f l").
  // For many grammars, this is not a valid parse; without
  // setting this, the parse will fail.
  // Associativity must be Left or Right.
  // This always has infinite precedence.
  void SetAdjacentOp(Associativity assoc,
                     std::function<Out(Out, Out)> op) {
    CHECK(assoc != Associativity::Non) << "Adjacency must "
      "be Left or Right associativity.";
    adj_assoc = assoc;
    adj_op = std::move(op);
  }

  std::optional<Out> Resolve(const std::vector<Item> &items,
                             std::string *error_arg) {
    error = error_arg;
    xs.clear();
    ys.clear();

    for (const Item &item : items) ys.push_back(item);

    for (;;) {
      // Port note: This is the code called "next" in
      // sml-lib's resolvefixity.
      if (ys.empty() &&
          xs.size() == 1 && xs[0].fixity == Fixity::Atom) {
        // Reduced to one item; done.
        return std::make_optional<Out>(xs[0].item);
      } else if (ys.empty()) {
        if (!Reduce()) {
          return std::nullopt;
        }
      } else {
        CHECK(!ys.empty());
        Item y = std::move(ys.front());
        ys.pop_front();
        if (!Resolve(y)) {
          return std::nullopt;
        }
      }
    }
  }

private:
  bool Reduce() {
    // Reduce the top of the stack, noting that the items are in reverse.
    if (xs.size() >= 2 &&
        xs[0].fixity == Fixity::Atom &&
        xs[1].fixity == Fixity::Prefix) {
      // atom :: prefix op :: rest
      Item x = std::move(xs.front());
      xs.pop_front();
      Item f = std::move(xs.front());
      xs.pop_front();
      CHECK(f.unop) << "Unary operator must be given for "
        "a Prefix item.";

      xs.push_front(Item::MakeAtom(f.unop(x.item)));
      return true;
    } else if (xs.size() >= 3 &&
               xs[0].fixity == Fixity::Atom &&
               xs[1].fixity == Fixity::Infix &&
               xs[2].fixity == Fixity::Atom) {
      // atom b :: infix op :: atom a :: rest
      FixityItem b = std::move(xs.front());
      xs.pop_front();
      FixityItem f = std::move(xs.front());
      xs.pop_front();
      FixityItem a = std::move(xs.front());
      xs.pop_front();

      CHECK(f.binop) << "Binary operator must be given for "
        "an Infix item.";
      xs.push_front(Item::MakeAtom(f.binop(a.item, b.item)));
      return true;
    } else if (xs.size() >= 2 &&
               xs[0].fixity == Fixity::Postfix &&
               xs[1].fixity == Fixity::Atom) {
      // postfix op :: atom :: rest
      FixityItem f = std::move(xs.front());
      xs.pop_front();
      FixityItem x = std::move(xs.front());
      xs.pop_front();

      CHECK(f.unop) << "Unary operator must be given for "
        "a Postfix item.";

      xs.push_front(Item::MakeAtom(f.unop(x.item)));
      return true;
    } else {
      if (error != nullptr)
        *error = "No reduction applies.";
      return false;
    }
  }

  bool Resolve(const Item &item) {
    if (adj_assoc != Associativity::Non &&
        item.fixity == Fixity::Atom &&
        !xs.empty() &&
        xs.front().fixity == Fixity::Atom) {

      // This is the case that we have two atoms adjacent to one
      // another. As a trick, we hallucinate an infix operator
      // here that implements the requested adjacency.
      Item adj;
      adj.fixity = Fixity::Infix;
      adj.assoc = adj_assoc;
      adj.precedence = 9999;
      adj.binop = adj_op;
      ys.push_front(item);
      return Resolve(adj);

    } else if (item.fixity == Fixity::Atom) {
      xs.push_front(item);
      return true;
    } else if (item.fixity == Fixity::Prefix) {
      xs.push_front(item);
      return true;
    } else if (xs.size() == 1 &&
               item.fixity == Fixity::Infix) {
      xs.push_front(item);
      return true;
    } else if (xs.size() >= 2 &&
               xs[1].fixity != Fixity::Atom &&
               item.fixity == Fixity::Infix) {
      // PERF: We always put x1 and x2 right back on!
      // Do like I did in the postfix case below.
      FixityItem x1 = std::move(xs.front());
      xs.pop_front();
      FixityItem x2 = std::move(xs.front());
      xs.pop_front();

      if (item.precedence > x2.precedence) {
        xs.push_front(x2);
        xs.push_front(x1);
        xs.push_front(item);
        return true;
      } else if (x2.precedence > item.precedence) {
        xs.push_front(x2);
        xs.push_front(x1);
        ys.push_front(item);
        return Reduce();
      } else if (x2.assoc == Associativity::Left &&
                 item.assoc == Associativity::Left) {
        xs.push_front(x2);
        xs.push_front(x1);
        ys.push_front(item);
        return Reduce();
      } else if (x2.assoc == Associativity::Right &&
                 item.assoc == Associativity::Right) {
        xs.push_front(x2);
        xs.push_front(x1);
        xs.push_front(item);
        return true;
      } else {
        // Ambiguous: Same precedence and incompatible (or no)
        // associativity.
        if (error != nullptr)
          *error = "Ambiguous parse: Infix operators have "
            "the same precedence and incompatible associativity.";
        return false;
      }
    } else if (xs.size() == 1 &&
               item.fixity == Fixity::Postfix) {
      xs.push_front(item);
      return Reduce();
    } else if (xs.size() >= 2 &&
               xs[1].fixity != Fixity::Atom &&
               item.fixity == Fixity::Postfix) {
      const Item &x2 = xs[1];
      if (item.precedence > x2.precedence) {
        xs.push_front(item);
        return Reduce();
      } else if (x2.precedence > item.precedence) {
        ys.push_front(item);
        return Reduce();
      } else {
        // Ambiguous.
        if (error != nullptr)
          *error = "Ambiguous parse (postfix).";
        return false;
      }
    } else {
      // Atom / operator mismatch.
      if (error != nullptr)
        *error = "Parse error: Invalid operands";
      return false;
    }

    LOG(FATAL) << "Should be unreachable.";
  }


private:
  // Normal for these to be unset.
  Associativity adj_assoc = Associativity::Non;
  std::function<Out(Out, Out)> adj_op;

  // State while parsing.
  std::deque<Item> xs;
  std::deque<Item> ys;

  // Optional error message for caller.
  std::string *error = nullptr;
};

template<class Out>
std::optional<Out> ResolveFixity(
    const std::vector<FixityItem<Out>> &items,
    std::string *error_arg = nullptr) {
  FixityResolver<Out> resolver;
  return resolver.Resolve(items, error_arg);
}

template<class Out>
std::optional<Out> ResolveFixityAdj(
    const std::vector<FixityItem<Out>> &items,
    Associativity adj_assoc,
    std::function<Out(Out, Out)> adj_op,
    std::string *error_arg = nullptr) {
  FixityResolver<Out> resolver;
  resolver.SetAdjacentOp(adj_assoc, std::move(adj_op));
  return resolver.Resolve(items, error_arg);
}

// TODO:
// failure handler
// list of alternates
// sequence, making tuple


#endif


#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <memory>
#include <string_view>
#include <unordered_set>

// Seems some problems want bignum, but if you have trouble compiling
// that, you could just use int64.
#if NO_BIGNUM
#else
# include "bignum/big.h"
#endif

namespace icfp {

#if NO_BIGNUM
using int_type = int64_t;
inline int64_t GetInt64(const int_type &i) { return i; }
inline std::string IntToString(const int_type &i) {
  char buf[100];
  sprintf(buf, "%" PRId64, i->i);
  return buf;
}
#else
using int_type = BigInt;
inline int64_t GetInt64(const int_type &i) {
  auto io = i.ToInt();
  assert(io.has_value() && "integer to big to convert to 64-bit");
  return io.value();
}
inline std::string IntToString(const int_type &i) {
  return i.ToString();
}
#endif

static constexpr int RADIX = 94;

std::string EncodeString(std::string_view s);
uint8_t DecodeChar(uint8_t c);

// Defined as a variant below.
struct Bool;
struct Int;
struct String;
struct Unop;
struct Binop;
struct If;
struct Lambda;
struct Var;
struct Error;

using Exp = std::variant<
  Bool,
  Int,
  String,
  Unop,
  Binop,
  If,
  Lambda,
  Var>;

using Value = std::variant<
  Bool,
  Int,
  String,
  Lambda,
  Error>;

struct Error {
  std::string msg;
};

struct Bool {
  bool b;
};

struct Int {
  int_type i;
};

struct String {
  std::string s;
};

/*
unops, represented as the raw byte

- Integer negation  U- I$ -> -3
! Boolean not U! T -> false
# string-to-int: interpret a string as a base-94 number U# S4%34 -> 15818151
$ int-to-string: inverse of the above U$ I4%34 -> test
*/

struct Unop {
  uint8_t op;
  std::shared_ptr<Exp> arg;
};

/*
binops, represented as the raw byte

+ Integer addition  B+ I# I$ -> 5
- Integer subtraction B- I$ I# -> 1
* Integer multiplication  B* I$ I# -> 6
/ Integer division (truncated towards zero) B/ U- I( I# -> -3
% Integer modulo  B% U- I( I# -> -1
< Integer comparison  B< I$ I# -> false
> Integer comparison  B> I$ I# -> true
= Equality comparison, works for int, bool and string B= I$ I# -> false
| Boolean or  B| T F -> true
& Boolean and B& T F -> false
. String concatenation  B. S4% S34 -> "test"
T Take first x chars of string y  BT I$ S4%34 -> "tes"
D Drop first x chars of string y  BD I$ S4%34 -> "t"
$ Apply term x to y (see Lambda abstractions)
*/

struct Binop {
  uint8_t op;
  std::shared_ptr<Exp> arg1, arg2;
};

struct If {
  std::shared_ptr<Exp> cond, t, f;
};

struct Lambda {
  // Variable converted to an integer.
  int_type v;
  std::shared_ptr<Exp> body;
};

struct Var {
  int_type v;
};

std::string ValueString(const Value &v);

struct Evaluation {
  // Number of beta redices performed.
  int64_t betas = 0;
  // We use negative variable names for fresh ones, since they
  // cannot be written in the source language.
  int64_t next_var = -1;

  // [e1/v]e2. Avoids capture (unless simple=true).
  std::shared_ptr<Exp> Subst(std::shared_ptr<Exp> e1,
                             const int_type &v,
                             std::shared_ptr<Exp> e2,
                             bool simple = false);

  template<class F>
  Value EvalToInt(const Exp *exp,
                  const F &f) {
    Value v = Eval(exp);
    if (const Int *i = std::get_if<Int>(&v)) {
      return f(std::move(*i));
    } else if (const Error *e = std::get_if<Error>(&v)) {
      return v;
    }
    return Value(Error{.msg = "Expected int"});
  }

  template<class F>
  Value EvalToBool(const Exp *exp,
                   const F &f) {
    Value v = Eval(exp);
    if (const Bool *b = std::get_if<Bool>(&v)) {
      return f(std::move(*b));
    } else if (const Error *e = std::get_if<Error>(&v)) {
      return v;
    }
    return Value(Error{.msg = "Expected bool"});
  }

  template<class F>
  Value EvalToString(const Exp *exp,
                   const F &f) {
    Value v = Eval(exp);
     if (const String *s = std::get_if<String>(&v)) {
      return f(std::move(*s));
    }
    return Value(Error{.msg = "Expected string"});
  }

  // Evaluate to a value.
  Value Eval(const Exp *exp);

  static std::unordered_set<int_type> FreeVars(const Exp *e);

 private:
  std::shared_ptr<Exp> SubstInternal(
      const std::unordered_set<int_type> &fvs,
      std::shared_ptr<Exp> e1,
      const int_type &v,
      std::shared_ptr<Exp> e2,
      bool simple);
};

std::shared_ptr<Exp> ValueToExp(const Value &v);

// Simple recursive-descent parser. Consumes an expression from the beginning
// of the string view.
std::shared_ptr<Exp> ParseLeadingExp(std::string_view *s);

// Read all the input from stdin; strip leading and trailing space.
std::string ReadAllInput();

}  // namespace icfp

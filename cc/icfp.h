
#ifndef ICFP_H_
#define ICFP_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <memory>
#include <string_view>
#include <unordered_set>
#include <vector>

// If you have problems with bignum, you'll have to get an older
// revision. Many problems assume this.
#include "bignum/big.h"
#include "bignum/big-overloads.h"

namespace icfp {

static constexpr int RADIX = 94;

// Returns e.g. I! for 0.
std::string IntConstant(const BigInt &i);
// Without leading S.
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
struct Memo;

using Exp = std::variant<
  Bool,
  Int,
  String,
  Unop,
  Binop,
  If,
  Lambda,
  Var,
  Memo>;

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
  BigInt i;
};

struct String {
  std::string s;
};

struct Memo {
  // If Exp is present, null means not computed.
  // If Value is present, value is meaningless.
  std::shared_ptr<std::unordered_set<int64_t>> fvs;

  // Exactly one of the following is set.
  std::shared_ptr<Exp> todo;
  std::shared_ptr<Value> done;
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
  // In principle, variables can be bignums, but the number of distinct
  // variables is bounded by the program size (and execution time, when
  // we rename). So we rename these to machine-word sized integers.
  int64_t v;
  std::shared_ptr<Exp> body;
};

struct Var {
  int64_t v;
};

std::string ValueString(const Value &v);
std::string PrettyExp(const Exp *e);

struct Evaluation {
  // Number of beta redices performed.
  int64_t betas = 0;
  // We use negative variable names for fresh ones, since they
  // cannot be written in the source language.
  int64_t next_var = -1;

  // [e1/v]e2. Avoids capture (unless simple=true).
  std::shared_ptr<Exp> Subst(std::shared_ptr<Exp> e1,
                             int64_t,
                             std::shared_ptr<Exp> e2,
                             bool simple = false);

  template<class F>
  Value EvalToInt(std::shared_ptr<Exp> exp,
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
  Value EvalToBool(std::shared_ptr<Exp> exp,
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
  Value EvalToString(const char *what,
                     std::shared_ptr<Exp> exp,
                     const F &f) {
    Value v = Eval(exp);
    if (const String *s = std::get_if<String>(&v)) {
      return f(std::move(*s));
    } else if (const Error *e = std::get_if<Error>(&v)) {
      return v;
    }
    return Value(Error{.msg = std::string("Expected string in ") + what});
  }

  // Evaluate to a value.
  Value Eval(std::shared_ptr<Exp> exp);

  static std::unordered_set<int64_t> FreeVars(const Exp *e);

 private:
  void EnsureFreeVars(Memo *m);

  std::shared_ptr<Exp> SubstInternal(
      const std::unordered_set<int64_t> &fvs,
      std::shared_ptr<Exp> e1,
      int64_t v,
      std::shared_ptr<Exp> e2,
      bool simple);
};

std::shared_ptr<Exp> ValueToExp(const Value &v);

struct Parser {
  Parser();

  // For error messages, we can map from (non-negative) word-sized
  // variables to the original variable number (and then to the
  // string, if we want).
  std::vector<BigInt> original_vars;
  std::unordered_map<BigInt, int> word_var;

  // Simple recursive-descent parser. Consumes an expression from the beginning
  // of the string view.
  std::shared_ptr<Exp> ParseLeadingExp(std::string_view *s);

  int64_t MapVar(const BigInt &b);
};

// Read all the input from stdin; strip leading and trailing space.
std::string ReadAllInput();

}  // namespace icfp

#endif


// #include "bignum/big.h"

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <memory>
#include <cstdio>
#include <cassert>

// TODO: Use bignum, but I want something super portable to start
using int_type = int64_t;

static constexpr int RADIX = 94;

// (size includes terminating \0, unused)
static constexpr char tostring_chars[RADIX + 1] =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ \n";

enum TokenType {
  BOOL,
  INT,
  STRING,
  UNOP,
  BINOP,
  IF,
  LAMBDA,
  VAR,
};

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

// [e1/v]e2
// TODO: Avoid capture!
static std::shared_ptr<Exp> Subst(std::shared_ptr<Exp> e1,
                                  int_type v,
                                  std::shared_ptr<Exp> e2) {
  if (const Bool *b = std::get_if<Bool>(e2.get())) {
    return e2;

  } else if (const Int *i = std::get_if<Int>(e2.get())) {
    return e2;

  } else if (const String *s = std::get_if<String>(e2.get())) {
    return e2;

  } else if (const Unop *u = std::get_if<Unop>(e2.get())) {

    return std::make_shared<Exp>(
        Unop{.op = u->op, .arg = Subst(e1, v, u->arg)});

  } else if (const Binop *b = std::get_if<Binop>(e2.get())) {

    return std::make_shared<Exp>(Binop{
        .op = b->op,
        .arg1 = Subst(e1, v, b->arg1),
        .arg2 = Subst(e1, v, b->arg2),
    });

  } else if (const If *i = std::get_if<If>(e2.get())) {

    return std::make_shared<Exp>(If{
        .cond = Subst(e1, v, i->cond),
        .t = Subst(e1, v, i->t),
        .f = Subst(e1, v, i->f),
    });

  } else if (const Lambda *l = std::get_if<Lambda>(e2.get())) {

    // XXX this is wrong; need to rename first
    return std::make_shared<Exp>(Lambda{
        .v = l->v,
        .body = Subst(e1, v, l->body),
    });

  } else if (const Var *var = std::get_if<Var>(e2.get())) {

    if (var->v == v) {
      return e1;
    } else {
      return e2;
    }
  }

  assert(!"bug: invalid exp variant");
  return nullptr;
}

struct Evaluation {
  int64_t betas = 0;

  template<class F>
  Value EvalToInt(const Exp *exp,
                      const F &f) {
    Value v = Eval(exp);
    if (const Int *i = std::get_if<Int>(&v)) {
      return f(std::move(*i));
    }
    return Value(Error{.msg = "Expected int"});
  }

  template<class F>
  Value EvalToBool(const Exp *exp,
                   const F &f) {
    Value v = Eval(exp);
     if (const Bool *b = std::get_if<Bool>(&v)) {
      return f(std::move(*b));
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
  Value Eval(const Exp *exp) {
    if (const Bool *b = std::get_if<Bool>(exp)) {
      return Value(*b);

    } else if (const Int *i = std::get_if<Int>(exp)) {
      return Value(*i);

    } else if (const String *s = std::get_if<String>(exp)) {
      return Value(*s);

    } else if (const Unop *u = std::get_if<Unop>(exp)) {

      switch (u->op) {
      case '-': {
        // - Integer negation  U- I$ -> -3
        return EvalToInt(u->arg.get(), [&](Int arg) {
            return Value(Int{.i = -arg.i});
          });
      }
      case '!': {
        // ! Boolean not U! T -> false
        return EvalToBool(u->arg.get(), [&](Bool arg) {
            return Value(Bool{.b = !arg.b});
          });
      }
      case '#': {
        // # string-to-int: interpret a string as a base-94 number
        // U# S4%34 -> 15818151
        assert(!"Unimplemented unop #");
      }
      case '$': {
        // $ int-to-string: inverse of the above U$ I4%34 -> test
        assert(!"Unimplemented unop $");
      }
      default:
        return Value(Error{.msg = "Invalid unop"});
      }

    } else if (const Binop *b = std::get_if<Binop>(exp)) {

      switch (b->op) {

      case '$': {
        // $ Apply term x to y (see Lambda abstractions)

        // This operator is lazy: We evaluate the LHS to get a lambda, but the RHS
        // is just substituted without evaluating it first.

        Value arg1 = Eval(b->arg1.get());
        if (const Lambda *lam = std::get_if<Lambda>(&arg1)) {
          betas++;
          // TODO: Check limits
          // TODO PERF: This can be tail recursive.
          std::shared_ptr<Exp> e = Subst(b->arg2, lam->v, lam->body);
          return Eval(e.get());

        } else {
          return Value(Error{.msg = "Expected lambda"});
        }
      }

      case '+': {
        // + Integer addition  B+ I# I$ -> 5

        return EvalToInt(b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Int{.i = arg1.i + arg2.i});
              });
          });
      }

      case '-': {
        // - Integer subtraction B- I$ I# -> 1

        return EvalToInt(b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Int{.i = arg1.i - arg2.i});
              });
          });
      }

      case '*': {
        // * Integer multiplication  B* I$ I# -> 6

        return EvalToInt(b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Int{.i = arg1.i * arg2.i});
              });
          });
      }

      case '/': {
        // / Integer division (truncated towards zero) B/ U- I( I# -> -3

        return EvalToInt(b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                if (arg2.i == 0) {
                  return Value(Error{.msg = "division by zero"});
                } else {
                  // This should be what they want (c truncation), but worth checking
                  return Value(Int{.i = arg1.i / arg2.i});
                }
              });
          });
      }

      case '%': {
        // % Integer modulo  B% U- I( I# -> -1

        return EvalToInt(b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                if (arg2.i == 0) {
                  return Value(Error{.msg = "modulus by zero"});
                } else {
                  // This should be what they want (c truncation), but worth checking
                  return Value(Int{.i = arg1.i % arg2.i});
                }
              });
          });
      }

      case '<': {
        // < Integer comparison  B< I$ I# -> false

        return EvalToInt(b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Bool{.b = arg1.i < arg2.i});
              });
          });
      }

      case '>': {
        // > Integer comparison  B> I$ I# -> true

        return EvalToInt(b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Bool{.b = arg1.i > arg2.i});
              });
          });
      }

      case '=': {
        // = Equality comparison, works for int, bool and string B= I$ I# -> false

        // Ugh, needs to be polymorphic.
        // TODO: Corner case: What if we compare values of different type?
        assert(!"unimplemented");
      }

      case '|': {
        // | Boolean or  B| T F -> true

        return EvalToBool(b->arg1.get(), [&](Bool arg1) {
            return EvalToBool(b->arg2.get(), [&](Bool arg2) {
                return Value(Bool{.b = arg1.b || arg2.b});
              });
          });
      }

      case '&': {
        // & Boolean and B& T F -> false

        return EvalToBool(b->arg1.get(), [&](Bool arg1) {
            return EvalToBool(b->arg2.get(), [&](Bool arg2) {
                return Value(Bool{.b = arg1.b && arg2.b});
              });
          });
      }

      case '.': {
        // . String concatenation  B. S4% S34 -> "test"

        return EvalToString(b->arg1.get(), [&](String arg1) {
            return EvalToString(b->arg2.get(), [&](String arg2) {
                return Value(String{.s = arg1.s + arg2.s});
              });
          });
      }

      case 'T': {
        // T Take first x chars of string y  BT I$ S4%34 -> "tes"

        return EvalToInt(b->arg1.get(), [&](Int arg1) {
            return EvalToString(b->arg2.get(), [&](String arg2) {
                if (arg1.i < 0) {
                  return Value(Error{.msg = "negative length in T"});
                } else {
                  // Corner case: length is bigger than string length
                  if (arg1.i > (int64_t)arg2.s.size()) {
                    return Value(Error{.msg = "length exceeds string size in T"});
                  } else {
                    return Value(String{.s = arg2.s.substr(0, arg1.i)});
                  }
                }
              });
            });
      }

      case 'D': {
        // D Drop first x chars of string y  BD I$ S4%34 -> "t"

        return EvalToInt(b->arg1.get(), [&](Int arg1) {
            return EvalToString(b->arg2.get(), [&](String arg2) {
                if (arg1.i < 0) {
                  return Value(Error{.msg = "negative length in D"});
                } else {
                  // Corner case: length is bigger than string length
                  if (arg1.i > (int64_t)arg2.s.size()) {
                    return Value(Error{.msg = "length exceeds string size in D"});
                  } else {
                    return Value(String{.s = arg2.s.substr(arg1.i, std::string::npos)});
                  }
                }
              });
            });
      }

      default:
        return Value(Error{.msg = "Invalid unop"});
      }

    } else if (const If *i = std::get_if<If>(exp)) {

      EvalToBool(i->cond.get(), [&](Bool cond) {
          if (cond.b) {
            return Eval(i->t.get());
          } else {
            return Eval(i->f.get());
          }
        });

    } else if (const Lambda *l = std::get_if<Lambda>(exp)) {

      return Value(*l);

    } else if (const Var *v = std::get_if<Var>(exp)) {

      return Value(Error{.msg = "unbound variable"});
    }

    assert(!"bug: invalid exp variant in eval");
    return Value(Error{.msg = "invalid exp variant"});
  }

};


int main(int argc, char **argv) {

  // TODO


  printf("OK\n");
  return 0;
}

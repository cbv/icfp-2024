
#include "icfp.h"

#include <optional>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <memory>
#include <cstdio>
#include <cassert>
#include <string_view>

#include "bignum/big.h"
#include "bignum/big-overloads.h"

namespace icfp {

// (size includes terminating \0, unused)
static constexpr const char DECODE_STRING[RADIX + 1] =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ \n";

static constexpr const char ENCODE_STRING[128] =
  "..........~.....................}_`abcdefghijklmUVWXYZ[\\]"
  "^nopqrst;<=>?@ABCDEFGHIJKLMNOPQRSTuvwxyz!\"#$%&'()*+,-./0123456789:.{.|";

std::string ValueString(const Value &v) {

  if (const Bool *b = std::get_if<Bool>(&v)) {
    return b->b ? "true" : "false";

  } else if (const Int *i = std::get_if<Int>(&v)) {
    return IntToString(i->i);

  } else if (const String *s = std::get_if<String>(&v)) {
    return "\"" + s->s + "\"";

  } else if (const Lambda *l = std::get_if<Lambda>(&v)) {
    return "(lambda)";
  } else if (const Error *err = std::get_if<Error>(&v)) {
    return "(ERROR:" + err->msg + ")";
  }

  return "(!!invalid value!!)";
}

// Also used by lambda and variables.
static std::optional<int_type> ConvertInt(std::string_view body) {
  int_type val{0};
  for (char c : body) {
    // assert(val < std::numeric_limits<int64_t>::max() / 94);
    val = val * 94;

    static_assert('~' - '!' == 93, "Encoding space is the size we expect.");
    if (c >= '!' && c <= '~') {
      val += int64_t(c - '!');
    } else {
      return std::nullopt;
    }
  }

  return {val};
}


// Also used by lambda and variables.
static int_type ParseInt(std::string_view body) {
  if (std::optional<int_type> i = ConvertInt(body)) {
    return i.value();
  } else {
    assert(!"Unparseable integer literal (or lambda/var arg)");
    return int_type{0};
  }
}

// [e1/v]e2. Avoids capture (unless simple=true).
std::shared_ptr<Exp> Evaluation::Subst(std::shared_ptr<Exp> e1, int_type v,
                                       std::shared_ptr<Exp> e2, bool simple) {
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

  } else if (const Lambda *lam = std::get_if<Lambda>(e2.get())) {

    // Don't even descend if the binding is shadowed.
    if (lam->v == v)
      return e2;

    if (simple) {
      return std::make_shared<Exp>(Lambda{
          .v = lam->v,
          .body = Subst(e1, v, lam->body),
      });
    } else {
      // Rename target lambda so that we can't have capture.
      // PERF only do this if we would incur capture?
      int_type new_var = int_type{next_var};
      next_var--;
      std::shared_ptr<Exp> new_var_exp =
          std::make_shared<Exp>(Var{.v = new_var});

      // Simple substitution, since the new variable cannot incur capture.
      std::shared_ptr<Exp> body = Subst(new_var_exp, lam->v, lam->body, true);

      // Now do the substitution, which can no longer capture.
      return std::make_shared<Exp>(Lambda{
          .v = new_var,
          .body = Subst(e1, v, body),
      });
    }

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

// Evaluate to a value.
Value Evaluation::Eval(const Exp *exp) {
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
      return EvalToInt(u->arg.get(),
                       [&](Int arg) { return Value(Int{.i = -arg.i}); });
    }
    case '!': {
      // ! Boolean not U! T -> false
      return EvalToBool(u->arg.get(),
                        [&](Bool arg) { return Value(Bool{.b = !arg.b}); });
    }
    case '#': {
      // # string-to-int: interpret a string as a base-94 number
      // U# S4%34 -> 15818151
      return EvalToString(
          u->arg.get(), [&](String arg) {
            // reencode
            std::string enc;
            enc.reserve(arg.s.size());
            for (uint8_t c : arg.s) {
              if (c >= 128) {
                return Value(Error{.msg = "unconvertible string (bad char) in string-to-int"});
              } else {
                enc.push_back(ENCODE_STRING[c]);
              }
            }

            if (std::optional<int_type> i = ConvertInt(enc)) {
              return Value(Int{.i = i.value()});
            } else {
              return Value(Error{.msg = "unconvertible string (not int) in string-to-int"});
            }
          });
    }

    case '$': {
      // $ int-to-string: inverse of the above U$ I4%34 -> test
      return EvalToInt(
          u->arg.get(), [&](Int arg) {
            if (arg.i < 0) {
              return Value(Error{.msg = "don't know how to convert negative integers to "
                  "base-94?"});
            }

            std::string rev;
            int_type i = arg.i;
            while (i > 0) {
              uint8_t digit = i % RADIX;
              rev.push_back(DECODE_STRING[digit]);
              i /= RADIX;
            }

            std::string s;
            s.resize(rev.size());
            for (int i = 0; i < (int)rev.size(); i++) {
              s[rev.size() - 1 - i] = rev[i];
            }
            return Value(String{.s = s});
          });
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

      } else if (const Error *e = std::get_if<Error>(&arg1)) {
        return arg1;
      } else {
        return Value(Error{.msg = "Expected lambda"});
      }
    }

    case '+': {
      // + Integer addition  B+ I# I$ -> 5

      return EvalToInt(
          b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Int{.i = arg1.i + arg2.i});
              });
          });
    }

    case '-': {
      // - Integer subtraction B- I$ I# -> 1

      return EvalToInt(
          b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Int{.i = arg1.i - arg2.i});
              });
          });
    }

    case '*': {
      // * Integer multiplication  B* I$ I# -> 6

      return EvalToInt(
          b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Int{.i = arg1.i * arg2.i});
              });
          });
    }

    case '/': {
      // / Integer division (truncated towards zero) B/ U- I( I# -> -3

      return EvalToInt(
          b->arg1.get(), [&](Int arg1) {
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

      return EvalToInt(
          b->arg1.get(), [&](Int arg1) {
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

      return EvalToInt(
          b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Bool{.b = arg1.i < arg2.i});
              });
          });
    }

    case '>': {
      // > Integer comparison  B> I$ I# -> true

      return EvalToInt(
          b->arg1.get(), [&](Int arg1) {
            return EvalToInt(b->arg2.get(), [&](Int arg2) {
                return Value(Bool{.b = arg1.i > arg2.i});
              });
          });
    }

    case '=': {
      // = Equality comparison, works for int, bool and string B= I$ I# -> false

      // Ugh, needs to be polymorphic.
      // TODO: Corner case: What if we compare values of different type?
      Value arg1 = Eval(b->arg1.get());
      if (const Error *e1 = std::get_if<Error>(&arg1)) return *e1;
      Value arg2 = Eval(b->arg2.get());
      if (const Error *e2 = std::get_if<Error>(&arg2)) return *e2;

      {
        const Int *i1 = std::get_if<Int>(&arg1);
        const Int *i2 = std::get_if<Int>(&arg2);
        if (i1 != nullptr && i2 != nullptr) {
          return Value(Bool{.b = i1->i == i2->i});
        }
      }

      {
        const Bool *b1 = std::get_if<Bool>(&arg1);
        const Bool *b2 = std::get_if<Bool>(&arg2);
        if (b1 != nullptr && b2 != nullptr) {
          return Value(Bool{.b = b1->b == b2->b});
        }
      }

      {
        const String *s1 = std::get_if<String>(&arg1);
        const String *s2 = std::get_if<String>(&arg2);
        if (s1 != nullptr && s2 != nullptr) {
          return Value(Bool{.b = s1->s == s2->s});
        }
      }

      printf("NO:%s\n%s\n", ValueString(arg1).c_str(),
             ValueString(arg2).c_str());



      return Value(
          Error{.msg = "binop = needs two args of the same base type"});
    }

    case '|': {
      // | Boolean or  B| T F -> true

      return EvalToBool(
          b->arg1.get(), [&](Bool arg1) {
            return EvalToBool(b->arg2.get(), [&](Bool arg2) {
                return Value(Bool{.b = arg1.b || arg2.b});
              });
          });
    }

    case '&': {
      // & Boolean and B& T F -> false

      return EvalToBool(
          b->arg1.get(), [&](Bool arg1) {
            return EvalToBool(b->arg2.get(), [&](Bool arg2) {
                return Value(Bool{.b = arg1.b && arg2.b});
              });
          });
    }

    case '.': {
      // . String concatenation  B. S4% S34 -> "test"

      return EvalToString(
          b->arg1.get(), [&](String arg1) {
            return EvalToString(b->arg2.get(), [&](String arg2) {
                return Value(String{.s = arg1.s + arg2.s});
              });
          });
    }

    case 'T': {
      // T Take first x chars of string y  BT I$ S4%34 -> "tes"

      return EvalToInt(
          b->arg1.get(), [&](Int arg1) {
            return EvalToString(b->arg2.get(), [&](String arg2) {
                const int64_t len = GetInt64(arg1.i);

                if (len < 0) {
                  return Value(Error{.msg = "negative length in T"});
                } else {
                  // Corner case: length is bigger than string length
                  if (len > (int64_t)arg2.s.size()) {
                    return Value(Error{.msg = "length exceeds string size in T"});
                  } else {
                    return Value(String{.s = arg2.s.substr(0, len)});
                  }
                }
              });
          });
    }

    case 'D': {
      // D Drop first x chars of string y  BD I$ S4%34 -> "t"

      return EvalToInt(
          b->arg1.get(), [&](Int arg1) {
            return EvalToString(b->arg2.get(), [&](String arg2) {
                const int64_t len = GetInt64(arg1.i);
                if (len < 0) {
                  return Value(Error{.msg = "negative length in D"});
                } else {
                  // Corner case: length is bigger than string length
                  if (len > (int64_t)arg2.s.size()) {
                    return Value(Error{.msg = "length exceeds string size in D"});
                  } else {
                    return Value(String{.s = arg2.s.substr(len, std::string::npos)});
                  }
                }
              });
          });
    }

    default:
      return Value(Error{.msg = "Invalid binop"});
    }

  } else if (const If *i = std::get_if<If>(exp)) {

    return EvalToBool(i->cond.get(), [&](Bool cond) {
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

  assert(exp != nullptr);

  assert(!"bug: invalid exp variant in eval");
  return Value(Error{.msg = "invalid exp variant"});
}

// Simple recursive-descent parser. Consumes an expression from the beginning
// of the string view.
std::shared_ptr<Exp> ParseLeadingExp(std::string_view *s) {
  while (!s->empty() && (*s)[0] == ' ') s->remove_prefix(1);
  assert(!s->empty() && "expected expression but got eos");

  // Always one indicator char.
  char ind = (*s)[0];
  s->remove_prefix(1);

  // Always get body. Might end by EOS or space.
  const size_t body_size = [&]() {
      for (size_t i = 0; i < s->size(); i++) {
        if ((*s)[i] == ' ') return i;
      }
      return s->size();
    }();

  std::string_view body = s->substr(0, body_size);
  s->remove_prefix(body_size);

  // printf("Indicator '%c' with body [%s]\n", ind, std::string(body).c_str());

  switch (ind) {
  case 'T':
    assert(body.empty() && "expected empty body for boolean");
    return std::make_shared<Exp>(Bool{.b = true});
  case 'F':
    assert(body.empty() && "expected empty body for boolean");
    return std::make_shared<Exp>(Bool{.b = false});

  case 'I': {
    assert(!body.empty() && "expected non-empty body for integer");

    int_type val = ParseInt(body);
    return std::make_shared<Exp>(Int{.i = val});
  }

  case 'S': {
    std::string translated;
    for (char c : body) {
      assert(c >= 33 && c <= 126 && "Bad char in string body");
      translated.push_back(DECODE_STRING[c - 33]);
    }
    return std::make_shared<Exp>(String{.s = std::move(translated)});
  }

  case 'U': {
    assert(body.size() == 1 && "unop body should be one char");
    Unop unop;
    unop.op = body[0];
    unop.arg = ParseLeadingExp(s);
    return std::make_shared<Exp>(std::move(unop));
  }

  case 'B': {
    assert(body.size() == 1 && "binop body should be one char");
    Binop binop;
    binop.op = body[0];
    binop.arg1 = ParseLeadingExp(s);
    binop.arg2 = ParseLeadingExp(s);
    return std::make_shared<Exp>(std::move(binop));
  }

  case '?': {
    assert(body.empty() && "if should have empty body");
    If iff;
    iff.cond = ParseLeadingExp(s);
    iff.t = ParseLeadingExp(s);
    iff.f = ParseLeadingExp(s);
    return std::make_shared<Exp>(std::move(iff));
  }

  case 'L': {
    Lambda lam;
    lam.v = ParseInt(body);
    lam.body = ParseLeadingExp(s);
    return std::make_shared<Exp>(std::move(lam));
  }

  case 'v': {
    int_type i = ParseInt(body);
    return std::make_shared<Exp>(Var{.v = i});
  }

  default:
    assert(!"invalid indicator");
  }
}

std::string EncodeString(std::string_view s) {
  std::string enc;
  enc.reserve(s.size());
  for (uint8_t c : s) {
    assert(c <= 128 && "character out of range in EncodeString");
    enc.push_back(ENCODE_STRING[c]);
  }
  return enc;
}

uint8_t DecodeChar(uint8_t digit) {
  assert(digit < RADIX);
  return DECODE_STRING[digit];
}

// Secret ops?
// ~: might be alias for B$, or maybe it is memoizing?
//

}  // namespace icfp


#include "icfp.h"

#include <unordered_set>
#include <optional>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <memory>
#include <cstdio>
#include <string_view>
#include <vector>

#include "base/logging.h"
#include "base/stringprintf.h"
#include "util.h"
#include "bignum/big.h"
#include "bignum/big-overloads.h"

namespace icfp {

static inline int64_t GetInt64(const BigInt &i) {
  auto io = i.ToInt();
  CHECK(io.has_value()) << "integer too big to convert to 64-bit: "
                        << i.ToString();
  return io.value();
}

// (size includes terminating \0, unused)
static constexpr const char DECODE_STRING[RADIX + 1] =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ \n";

static constexpr const char ENCODE_STRING[128] =
  "..........~.....................}_`abcdefghijklmUVWXYZ[\\]"
  "^nopqrst;<=>?@ABCDEFGHIJKLMNOPQRSTuvwxyz!\"#$%&'()*+,-./"
  "0123456789:.{.|";

std::string ValueString(const Value &v) {

  if (const Bool *b = std::get_if<Bool>(&v)) {
    return b->b ? "true" : "false";

  } else if (const Int *i = std::get_if<Int>(&v)) {
    return i->i.ToString();

  } else if (const String *s = std::get_if<String>(&v)) {
    return "\"" + s->s + "\"";

  } else if (const Lambda *l = std::get_if<Lambda>(&v)) {
    (void)l;
    return "(lambda)";

  } else if (const Error *err = std::get_if<Error>(&v)) {
    (void)err;

    return "(ERROR:" + err->msg + ")";
  }

  return "(!!invalid value!!)";
}

// Also used by lambda and variables.
static std::optional<BigInt> ConvertInt(std::string_view body) {
  BigInt val{0};
  for (char c : body) {
    val = val * RADIX;

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
static BigInt ParseInt(std::string_view body) {
  if (std::optional<BigInt> i = ConvertInt(body)) {
    return i.value();
  } else {
    LOG(FATAL) << "Unparseable integer literal (or lambda/var arg)";
    return BigInt{0};
  }
}

static void PopulateFreeVars(const Exp *e, std::unordered_set<int64_t> *fvs) {
  for (;;) {
    if (const Bool *b = std::get_if<Bool>(e)) {
      (void)b;
      return;

    } else if (const Int *i = std::get_if<Int>(e)) {
      (void)i;
      return;

    } else if (const String *s = std::get_if<String>(e)) {
      (void)s;
      return;

    } else if (const Unop *u = std::get_if<Unop>(e)) {
      e = u->arg.get();

    } else if (const Binop *b = std::get_if<Binop>(e)) {
      PopulateFreeVars(b->arg1.get(), fvs);
      e = b->arg2.get();

    } else if (const If *i = std::get_if<If>(e)) {
      PopulateFreeVars(i->cond.get(), fvs);
      PopulateFreeVars(i->t.get(), fvs);
      e = i->f.get();

    } else if (const Lambda *lam = std::get_if<Lambda>(e)) {

      std::unordered_set<int64_t> bfvs;
      PopulateFreeVars(lam->body.get(), &bfvs);
      // But the lambda's argument is bound.
      bfvs.erase(lam->v);

      // And then union them.
      for (auto &i : bfvs) fvs->insert(i);
      return;

    } else if (const Var *var = std::get_if<Var>(e)) {

      fvs->insert(var->v);
      return;

    } else if (const Memo *m = std::get_if<Memo>(e)) {

      if (m->done.get() != nullptr) {
        // Values have no free variables.
      } else {
        CHECK(m->todo.get() != nullptr);

        if (m->fvs.get() != nullptr) {
          for (int64_t v : *m->fvs) fvs->insert(v);
          return;
        } else {
          e = m->todo.get();
          // We could memoize them here, but we have
          // a const pointer.
        }
      }
      return;

    } else {
      LOG(FATAL) << "illegal exp";
    }
  }
}

std::unordered_set<int64_t> Evaluation::FreeVars(const Exp *e) {
  std::unordered_set<int64_t> ret;
  PopulateFreeVars(e, &ret);
  return ret;
}

// [e1/v]e2. Avoids capture (unless simple=true).
std::shared_ptr<Exp> Evaluation::Subst(
    std::shared_ptr<Exp> e1, int64_t v,
    std::shared_ptr<Exp> e2, bool simple) {
  return SubstInternal(FreeVars(e1.get()), e1, v, e2, simple);
}

std::shared_ptr<Exp> Evaluation::SubstInternal(
    const std::unordered_set<int64_t> &fvs,
    std::shared_ptr<Exp> e1, int64_t v,
    std::shared_ptr<Exp> e2, bool simple) {

  if (const Bool *b = std::get_if<Bool>(e2.get())) {
    (void)b;
    return e2;

  } else if (const Int *i = std::get_if<Int>(e2.get())) {
    (void)i;
    return e2;

  } else if (const String *s = std::get_if<String>(e2.get())) {
    (void)s;
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

    if (simple || !fvs.contains(lam->v)) {
      return std::make_shared<Exp>(Lambda{
          .v = lam->v,
          .body = Subst(e1, v, lam->body),
      });
    } else {
      // Rename target lambda so that we can't have capture.
      const int64_t new_var = next_var;
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

  } else if (Memo *m = std::get_if<Memo>(e2.get())) {

    if (m->done.get() != nullptr) {
      // Values have no free variables.
      return e2;

    } else {
      CHECK(m->todo.get() != nullptr);

      // Ensure we have free vars.
      if (m->fvs.get() == nullptr) {
        m->fvs = std::make_shared<std::unordered_set<int64_t>>(
            FreeVars(m->todo.get()));
      }

      CHECK(m->fvs.get() != nullptr);

      if (m->fvs->contains(v)) {
        // Have to create a new memo cell, then.
        std::shared_ptr<Exp> s = Subst(e1, v, m->todo, false);
        return std::make_shared<Exp>(Memo{
            .fvs = nullptr,
            .todo = std::move(s),
            .done = nullptr,
          });

      } else {
        // The variable is not present, so substitution has no effect.
        return e2;
      }
    }

  }

  LOG(FATAL) << "bug: invalid exp variant";
  return nullptr;
}

std::shared_ptr<Exp> ValueToExp(const Value &v) {
  if (const Bool *b = std::get_if<Bool>(&v)) {
    return std::make_shared<Exp>(*b);

  } else if (const Int *i = std::get_if<Int>(&v)) {
    return std::make_shared<Exp>(*i);

  } else if (const String *s = std::get_if<String>(&v)) {
    return std::make_shared<Exp>(*s);

  } else if (const Lambda *l = std::get_if<Lambda>(&v)) {
    return std::make_shared<Exp>(*l);

  } else if (const Error *err = std::get_if<Error>(&v)) {
    (void)err;

    LOG(FATAL) << "cannot make error values into expressions";
  }

  LOG(FATAL) << "invalid value";
  return nullptr;
}

// Evaluate to a value.
Value Evaluation::Eval(std::shared_ptr<Exp> exp) {
  for (;;) {

    if (const Bool *b = std::get_if<Bool>(exp.get())) {
      return Value(*b);

    } else if (const Int *i = std::get_if<Int>(exp.get())) {
      return Value(*i);

    } else if (const String *s = std::get_if<String>(exp.get())) {
      return Value(*s);

    } else if (const Unop *u = std::get_if<Unop>(exp.get())) {

      switch (u->op) {
      case '-': {
        // - Integer negation  U- I$ -> -3
        return EvalToInt(u->arg,
                         [&](Int arg) { return Value(Int{.i = -arg.i}); });
      }
      case '!': {
        // ! Boolean not U! T -> false
        return EvalToBool(u->arg,
                          [&](Bool arg) { return Value(Bool{.b = !arg.b}); });
      }
      case '#': {
        // # string-to-int: interpret a string as a base-94 number
        // U# S4%34 -> 15818151
        return EvalToString("#",
            u->arg, [&](String arg) {
              // reencode
              std::string enc;
              enc.reserve(arg.s.size());
              for (uint8_t c : arg.s) {
                if (c >= 128) {
                  return Value(Error{.msg =
                      "unconvertible string (bad char) in string-to-int"});
                } else {
                  enc.push_back(ENCODE_STRING[c]);
                }
              }

              if (std::optional<BigInt> i = ConvertInt(enc)) {
                return Value(Int{.i = i.value()});
              } else {
                return Value(Error{.msg =
                    "unconvertible string (not int) in string-to-int"});
              }
            });
      }

      case '$': {
        // $ int-to-string: inverse of the above U$ I4%34 -> test
        return EvalToInt(
            u->arg, [&](Int arg) {
              if (arg.i < 0) {
                return Value(Error{.msg =
                    "don't know how to convert negative integers to "
                    "base-94?"});
              }

              std::string rev;
              BigInt i = arg.i;
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

    } else if (const Binop *b = std::get_if<Binop>(exp.get())) {

      switch (b->op) {
      case '$': {
        // $ Apply term x to y (see Lambda abstractions)

        // This operator is lazy: We evaluate the LHS to get a lambda,
        // but the RHS is just substituted without evaluating it
        // first.

        Value arg1 = Eval(b->arg1);
        if (const Lambda *lam = std::get_if<Lambda>(&arg1)) {
          betas++;

          // Memoize arg2, so that if it is evaluated multiple
          // times, we only pay once.
          std::shared_ptr<Exp> arg;

          if (std::holds_alternative<Memo>(*b->arg2)) {
            // Oh, it's already a memo cell. Don't add indirection.
            arg = b->arg2;

          } else {
            arg = std::make_shared<Exp>(Memo{
                .fvs = nullptr,
                .todo = b->arg2,
                .done = nullptr});
          }

          // TODO: Check limits
          exp = Subst(std::move(arg), lam->v, lam->body);
          // Tail recursion.
          continue;

        } else if (const Error *e = std::get_if<Error>(&arg1)) {
          (void)e;
          return arg1;

        } else {
          return Value(Error{.msg = "Expected lambda"});
        }
      }

      case '!': {
        // Secret call-by-value version of application.
        Value arg1 = Eval(b->arg1);
        if (const Lambda *lam = std::get_if<Lambda>(&arg1)) {

          Value arg2 = Eval(b->arg2);
          if (const Error *e = std::get_if<Error>(&arg2)) {
            (void)e;
            return arg2;
          }

          betas++;
          // TODO: Check limits
          exp = Subst(ValueToExp(arg2), lam->v, lam->body);
          // Tail recursion.
          continue;

        } else if (const Error *e = std::get_if<Error>(&arg1)) {
          (void)e;
          return arg1;

        } else {
          return Value(Error{.msg = "Expected lambda"});
        }
      }

      case '+': {
        // + Integer addition  B+ I# I$ -> 5

        return EvalToInt(
            b->arg1, [&](Int arg1) {
              return EvalToInt(b->arg2, [&](Int arg2) {
                  return Value(Int{.i = arg1.i + arg2.i});
                });
            });
      }

      case '-': {
        // - Integer subtraction B- I$ I# -> 1

        return EvalToInt(
            b->arg1, [&](Int arg1) {
              return EvalToInt(b->arg2, [&](Int arg2) {
                  return Value(Int{.i = arg1.i - arg2.i});
                });
            });
      }

      case '*': {
        // * Integer multiplication  B* I$ I# -> 6

        return EvalToInt(
            b->arg1, [&](Int arg1) {
              return EvalToInt(b->arg2, [&](Int arg2) {
                  return Value(Int{.i = arg1.i * arg2.i});
                });
            });
      }

      case '/': {
        // / Integer division (truncated towards zero) B/ U- I( I# -> -3

        return EvalToInt(
            b->arg1, [&](Int arg1) {
              return EvalToInt(b->arg2, [&](Int arg2) {
                  if (arg2.i == 0) {
                    return Value(Error{.msg = "division by zero"});
                  } else {
                    // This should be what they want (c truncation), but
                    // worth checking
                    return Value(Int{.i = arg1.i / arg2.i});
                  }
                });
            });
      }

      case '%': {
        // % Integer modulo  B% U- I( I# -> -1

        return EvalToInt(
            b->arg1, [&](Int arg1) {
              return EvalToInt(b->arg2, [&](Int arg2) {
                  if (arg2.i == 0) {
                    return Value(Error{.msg = "modulus by zero"});
                  } else {
                    // This should be what they want (c truncation), but
                    // worth checking
                    return Value(Int{.i = arg1.i % arg2.i});
                  }
                });
            });
      }

      case '<': {
        // < Integer comparison  B< I$ I# -> false

        return EvalToInt(
            b->arg1, [&](Int arg1) {
              return EvalToInt(b->arg2, [&](Int arg2) {
                  return Value(Bool{.b = arg1.i < arg2.i});
                });
            });
      }

      case '>': {
        // > Integer comparison  B> I$ I# -> true

        return EvalToInt(
            b->arg1, [&](Int arg1) {
              return EvalToInt(b->arg2, [&](Int arg2) {
                  return Value(Bool{.b = arg1.i > arg2.i});
                });
            });
      }

      case '=': {
        // = Equality comparison, works for int, bool and string.
        // B= I$ I# -> false

        // Ugh, needs to be polymorphic.
        // TODO: Corner case: What if we compare values of different type?
        Value arg1 = Eval(b->arg1);
        if (const Error *e1 = std::get_if<Error>(&arg1)) return *e1;
        Value arg2 = Eval(b->arg2);
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

        return Value(
            Error{.msg = "binop = needs two args of the same base type"});
      }

      case '|': {
        // | Boolean or  B| T F -> true

        return EvalToBool(
            b->arg1, [&](Bool arg1) {
              return EvalToBool(b->arg2, [&](Bool arg2) {
                  return Value(Bool{.b = arg1.b || arg2.b});
                });
            });
      }

      case '&':
        // & Boolean and B& T F -> false
        return EvalToBool(
            b->arg1, [&](Bool arg1) {
              return EvalToBool(b->arg2, [&](Bool arg2) {
                  return Value(Bool{.b = arg1.b && arg2.b});
                });
            });

      case '.':
        // . String concatenation  B. S4% S34 -> "test"
        return EvalToString(
            ".lhs", b->arg1, [&](String arg1) {
                return EvalToString(
                    ".rhs", b->arg2, [&](String arg2) {
                        return Value(String{.s = arg1.s + arg2.s});
                      });
              });

      case 'T':
        // T Take first x chars of string y  BT I$ S4%34 -> "tes"
        return EvalToInt(b->arg1, [&](Int arg1) {
            return EvalToString("T", b->arg2, [&](String arg2) {
                const int64_t len = GetInt64(arg1.i);

                if (len < 0) {
                  return Value(Error{.msg = "negative length in T"});
                } else {
                  // Corner case: length is bigger than string length
                  if (len > (int64_t)arg2.s.size()) {
                    return Value(Error{.msg =
                        "length exceeds string size in T"});
                  } else {
                    return Value(String{.s = arg2.s.substr(0, len)});
                  }
                }
              });
          });

      case 'D':
        // D Drop first x chars of string y  BD I$ S4%34 -> "t"
        return EvalToInt(b->arg1, [&](Int arg1) {
            return EvalToString("D", b->arg2, [&](String arg2) {
                const int64_t len = GetInt64(arg1.i);
                if (len < 0) {
                  return Value(Error{.msg = "negative length in D"});
                } else {
                  // Corner case: length is bigger than string length
                  if (len > (int64_t)arg2.s.size()) {
                    return Value(Error{.msg =
                        "length exceeds string size in D"});
                  } else {
                    return Value(String{.s =
                        arg2.s.substr(len, std::string::npos)});
                  }
                }
              });
          });

      default:
        return Value(Error{.msg = "Invalid binop"});
      }

    } else if (const If *i = std::get_if<If>(exp.get())) {

      return EvalToBool(i->cond, [&](Bool cond) {
        if (cond.b) {
          return Eval(i->t);
        } else {
          return Eval(i->f);
        }
      });

    } else if (const Lambda *l = std::get_if<Lambda>(exp.get())) {

      return Value(*l);

    } else if (const Var *v = std::get_if<Var>(exp.get())) {
      (void)v;
      return Value(Error{.msg = StringPrintf(
              // TODO: Print the original variable name, but we
              // need to pass the parser's variable map to do
              // that.
              "unbound variable %lld",
              v->v)});

    } else if (Memo *m = std::get_if<Memo>(exp.get())) {

      if (m->done.get() == nullptr) {
        CHECK(m->todo.get() != nullptr);

        m->done = std::make_shared<Value>(Eval(m->todo));
        m->todo = nullptr;
        m->fvs = nullptr;
      }

      CHECK(m->done.get() != nullptr);

      return *m->done;

    } else {
      CHECK(exp != nullptr);

      LOG(FATAL) << "bug: invalid exp variant in eval";
      return Value(Error{.msg = "invalid exp variant"});
    }

    // Some constructs work by replacing the exp arg and looping,
    // to ensure tail-recursion.
  }

}

Parser::Parser() {

}

int64_t Parser::MapVar(const BigInt &b) {
  if (const auto it = word_var.find(b); it != word_var.end()) {
    return it->second;
  } else {
    int64_t wv = original_vars.size();
    word_var[b] = wv;
    original_vars.push_back(b);
    return wv;
  }
}

// Simple recursive-descent parser. Consumes an expression from the beginning
// of the string view.
std::shared_ptr<Exp> Parser::ParseLeadingExp(std::string_view *s) {
  while (!s->empty() && (*s)[0] == ' ') s->remove_prefix(1);
  CHECK(!s->empty()) << "expected expression but got eos";

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
    CHECK(body.empty()) << "expected empty body for boolean";
    return std::make_shared<Exp>(Bool{.b = true});
  case 'F':
    CHECK(body.empty()) << "expected empty body for boolean";
    return std::make_shared<Exp>(Bool{.b = false});

  case 'I': {
    CHECK(!body.empty()) << "expected non-empty body for integer";

    BigInt val = ParseInt(body);
    return std::make_shared<Exp>(Int{.i = val});
  }

  case 'S': {
    std::string translated;
    for (char c : body) {
      CHECK(c >= 33 && c <= 126) << "Bad char in string body";
      translated.push_back(DECODE_STRING[c - 33]);
    }
    return std::make_shared<Exp>(String{.s = std::move(translated)});
  }

  case 'U': {
    CHECK(body.size() == 1) << "unop body should be one char. got: [" <<
      body << "]";
    Unop unop;
    unop.op = body[0];
    unop.arg = ParseLeadingExp(s);
    return std::make_shared<Exp>(std::move(unop));
  }

  case 'B': {
    CHECK(body.size() == 1) << "binop body should be one char. got: [" <<
      body << "]";
    Binop binop;
    binop.op = body[0];
    binop.arg1 = ParseLeadingExp(s);
    binop.arg2 = ParseLeadingExp(s);
    return std::make_shared<Exp>(std::move(binop));
  }

  case '?': {
    CHECK(body.empty()) << "if should have empty body";
    If iff;
    iff.cond = ParseLeadingExp(s);
    iff.t = ParseLeadingExp(s);
    iff.f = ParseLeadingExp(s);
    return std::make_shared<Exp>(std::move(iff));
  }

  case 'L': {
    Lambda lam;
    lam.v = MapVar(ParseInt(body));
    lam.body = ParseLeadingExp(s);
    return std::make_shared<Exp>(std::move(lam));
  }

  case 'v': {
    int64_t i = MapVar(ParseInt(body));
    return std::make_shared<Exp>(Var{.v = i});
  }

  default:
    LOG(FATAL) << StringPrintf("invalid indicator '%c'", ind);
  }
}

std::string IntConstant(const BigInt &i) {
  CHECK(i >= 0) <<
    "only non-negative integers can be represented as constants";

  // Unclear whether it would accept just "I" for zero.
  if (i == 0) return "I!";

  BigInt val = i;
  std::string rev;
  while (val > 0) {
    // We don't have quotrem with int64, so just do two operations.
    int64_t digit = BigInt::CMod(val, RADIX);
    val = BigInt::Div(val, RADIX);
    CHECK(digit >= 0 && digit < RADIX);
    rev.push_back('!' + digit);
  }

  std::string out;
  out.reserve(1 + rev.size());
  out.push_back('I');
  for (int i = rev.size() - 1; i >= 0; i--)
    out.push_back(rev[i]);

  return out;
}

std::string EncodeString(std::string_view s) {
  std::string enc;
  enc.reserve(s.size());
  for (uint8_t c : s) {
    CHECK(c <= 128) << "character out of range in EncodeString";
    enc.push_back(ENCODE_STRING[c]);
  }
  return enc;
}

uint8_t DecodeChar(uint8_t digit) {
  CHECK(digit < RADIX);
  return DECODE_STRING[digit];
}

static std::string PrettyVar(int64_t v) {
  CHECK(v >= 0);
  if (v < 26) return StringPrintf("%c", 'a' + v);
  return StringPrintf("v%zu", (size_t)v);
}

// Flatten n-ary operator if it's 'op', or just e.
static void PrettyFlat(uint8_t op, const Exp *exp,
                       std::vector<std::string> *out) {
  if (const Binop *b = std::get_if<Binop>(exp)) {
    if (b->op == op) {
      PrettyFlat(op, b->arg1.get(), out);
      PrettyFlat(op, b->arg2.get(), out);
      return;
    }
  }
  // Otherwise...
  out->push_back(PrettyExp(exp));
}

std::string PrettyExp(const Exp *exp) {
  if (const Bool *b = std::get_if<Bool>(exp)) {
    return b->b ? "true" : "false";

  } else if (const Int *i = std::get_if<Int>(exp)) {
    return i->i.ToString();

  } else if (const String *s = std::get_if<String>(exp)) {
    return "\"" + s->s + "\"";

  } else if (const Unop *u = std::get_if<Unop>(exp)) {

    switch (u->op) {
    case '-':
      return StringPrintf("(- %s)",
                          PrettyExp(u->arg.get()).c_str());

    case '!':
      return StringPrintf("(not %s)",
                          PrettyExp(u->arg.get()).c_str());
    case '#':
    case '$':
    default:
      return StringPrintf("(%c %s)",
                          u->op,
                          PrettyExp(u->arg.get()).c_str());
    }

  } else if (const Binop *b = std::get_if<Binop>(exp)) {

    switch (b->op) {
    case '$':
      // If it's ($ (\x. body) rhs) then rewrite this
      // to "let"
      if (const Lambda *lam = std::get_if<Lambda>(b->arg1.get())) {
        return StringPrintf("let %s = %s\n"
                            "in %s\n"
                            "end",
                            PrettyVar(lam->v).c_str(),
                            PrettyExp(b->arg2.get()).c_str(),
                            PrettyExp(lam->body.get()).c_str());
      } else {
        return StringPrintf("%s %s",
                            PrettyExp(b->arg1.get()).c_str(),
                            PrettyExp(b->arg2.get()).c_str());
      }

    case '|': {
      std::vector<std::string> args;
      PrettyFlat('|', b->arg1.get(), &args);
      PrettyFlat('|', b->arg2.get(), &args);

      return StringPrintf("(or %s)",
                          Util::Join(args, " ").c_str());
    }

    case '&': {
      std::vector<std::string> args;
      PrettyFlat('&', b->arg1.get(), &args);
      PrettyFlat('&', b->arg2.get(), &args);

      return StringPrintf("(and %s)",
                          Util::Join(args, " ").c_str());
    }

    default:
      return StringPrintf("(%c %s %s)",
                          b->op,
                          PrettyExp(b->arg1.get()).c_str(),
                          PrettyExp(b->arg2.get()).c_str());

    }

  } else if (const If *i = std::get_if<If>(exp)) {

    return StringPrintf("(if %s then %s else %s)",
                        PrettyExp(i->cond.get()).c_str(),
                        PrettyExp(i->t.get()).c_str(),
                        PrettyExp(i->f.get()).c_str());

  } else if (const Lambda *l = std::get_if<Lambda>(exp)) {

    return StringPrintf("(λ %s. %s)",
                        PrettyVar(l->v).c_str(),
                        PrettyExp(l->body.get()).c_str());

  } else if (const Var *v = std::get_if<Var>(exp)) {
    return PrettyVar(v->v);

  } else if (const Memo *m = std::get_if<Memo>(exp)) {
    return "(memo cell)";

  }

  return "???INVALID???";
}


// Secret ops?
// ~: might be alias for B$, or maybe it is memoizing?

std::string ReadAllInput() {
  std::string input;
  char c;
  while (EOF != (c = fgetc(stdin))) {
    input.push_back(c);
  }

  std::string_view input_view(input);

  // Strip leading and trailing whitespace.
  while (!input_view.empty() &&
         (input_view[0] == ' ' ||
          input_view[0] == '\n' ||
          input_view[0] == '\r'))
    input_view.remove_prefix(1);
  while (!input_view.empty() &&
         (input_view.back() == ' ' ||
          input_view.back() == '\n' ||
          input_view.back() == '\r'))
    input_view.remove_suffix(1);

  return std::string(input_view);
}

}  // namespace icfp

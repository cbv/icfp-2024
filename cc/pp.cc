
#include "icfp.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <cassert>
#include <cstdio>
#include <memory>
#include <variant>
#include <vector>

#include "base/logging.h"
#include "base/stringprintf.h"
#include "ansi.h"
#include "util.h"

using namespace icfp;


static std::string PrettyVar(const icfp::int_type &i) {
  // If you're using bignum and this fails, you could just call
  // i.ToString() here.
  int64_t ii = icfp::GetInt64(i);
  CHECK(ii >= 0);
  if (ii < 26) return StringPrintf("%c", 'a' + ii);
  return StringPrintf("v%zu", (size_t)ii);
}

static std::string Pretty(const Exp *exp);

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
  out->push_back(Pretty(exp));
}

static std::string Pretty(const Exp *exp) {
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
                          Pretty(u->arg.get()).c_str());

    case '!':
      // ! Boolean not U! T -> false
      return StringPrintf("(not %s)",
                          Pretty(u->arg.get()).c_str());

    case '#':
      return StringPrintf("(# %s)",
                          Pretty(u->arg.get()).c_str());

    case '$':
      return StringPrintf("($ %s)",
                          Pretty(u->arg.get()).c_str());

    default:
      return "???";
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
                            Pretty(b->arg2.get()).c_str(),
                            Pretty(lam->body.get()).c_str());
      } else {
        return StringPrintf("%s %s",
                            Pretty(b->arg1.get()).c_str(),
                            Pretty(b->arg2.get()).c_str());
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
                          Pretty(b->arg1.get()).c_str(),
                          Pretty(b->arg2.get()).c_str());

    }

  } else if (const If *i = std::get_if<If>(exp)) {

    return StringPrintf("(ite %s %s %s)",
                        Pretty(i->cond.get()).c_str(),
                        Pretty(i->t.get()).c_str(),
                        Pretty(i->f.get()).c_str());

  } else if (const Lambda *l = std::get_if<Lambda>(exp)) {

    return StringPrintf("(λ %s. %s)",
                        PrettyVar(l->v).c_str(),
                        Pretty(l->body.get()).c_str());

  } else if (const Var *v = std::get_if<Var>(exp)) {
    return PrettyVar(v->v);

  }

  return "???INVALID???";
}


int main(int argc, char **argv) {
  ANSI::Init();

  std::string input = ReadAllInput();
  std::string_view input_view(input);

  std::shared_ptr<Exp> exp = ParseLeadingExp(&input_view);

  CHECK(input_view.empty()) << "extra stuff after expression?";

  printf("%s\n", Pretty(exp.get()).c_str());

  return 0;
}

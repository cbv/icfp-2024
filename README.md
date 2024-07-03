# icfp-2024
ICFP contest 2024

Team: Cult of the Bound Variable

The team called "Cult of the Bound Variable" once again participated
in the ICFP Programming Contest in 2024! This year we were the
unordered set of {Tom 7, Jason Reed, Jim McCann, David Renshaw, Jed
Grabman}. Actually I used the signing order from `constitution.txt`, a
tradition that dates back to the inauguration of our subversion
repository for the [2006 contest](http://boundvariable.org/), which
some of us designed while students at CMU. We were excited to see
references to our lore in the 2024 contest, although we would have
been excited regardless, because we LOVE PROGRAMMING.

The evening before the contest is our regular beer outing, which has
been running continuously on Thursdays since before that 2006 contest
even (!), and we "strategized," which means that Tom steeled himself
to wake at the crisp dazzle of "8am EST" and agreed to use "git" and
David hoped that we would program in Rust or Lean (?!) and jcreed
promised that he would not use Perl, and Jim reminded us that he had a
bike ride scheduled on Saturday, but we all knew that even during his
bike ride he would be thinking about how to solve the puzzles and
come back with some kind of wizard stuff.

# The contest

(many spoilers henceforth)

The structure of the 2024 contest is a set of four mostly-independent
problems called "lambdaman", "spaceship", "3d", and "efficiency."
You get the problems (and confirm solutions) by communicating with their
server, but the problems are generally solved offline. Communication
happens with ASCII programs written in a little lazily-evaluated lambda
calculus (called "icfp", which we'll spell in lowercase).

Implementing this language was the first order of business as far as
Tom was concerned. For some reason he has been punishing himself with
writing language implementations in C++ recently. C++ has gotten a lot
better by 2024, but there are a couple things that suck about it for
implementing a lambda calculus evaluator. One is that the default
stack size in gcc and clang is tiny (like 1 megabyte) and you have to
rely on weird, non-portable incantations on the command line like
`-Wl,-z,stack-size=167772160` to make it be a reasonable size, or else
your programs will just silently die when they get big, which does
not embody the spirit of functional programming one bit. Nonetheless,
Tom churned out a reasonable evaluator in a couple hours. Of course
it passed the language tests the first time, because he is not about
to be tricked by capture-avoiding substitution or lazy evaluation or
arbitrary precision integers, even in C++!

Meanwhile, it turns out that most of the basic communication with the
server requires only string literals, and you can do this with only a
simple encoder/decoder. So while Tom embarked on evaluator hacking,
Jason implemented the codec in Typescript, giving us a lovely web
browser client for interacting with the server. With that,
we explored the initial problems available to us in the lambadaman
and spaceship categories and started checking them into our git repo
under the puzzles/ directory.

# Evaluator

The evaluator ([eval.cc](cc/eval.cc), [icfp.h](cc/icfp.h)) is pretty
straightforward. It takes icfp code on stdin and runs it to produce a
value, and outputs that on stdout. To represent expressions, of course
what you really want is ML's `datatype`, but we don't have that
because we're using C++ for some reason. Tom's current favorite
approach in C++ is to use `std::variant` (sum type) over a bunch of
structs, one for each arm. This gives you a sort of destructuring
one-level pattern match like
```
  if (const Binop *b = std::get_if<Binop>(&exp)) {
    // do something with b ...
  } else if ...
```
which is not so bad (`get_if` returns `nullptr` if `exp` is not the Binop
arm). In case this looks "too easy" and starts not feeling like C++
programming: A surprising and unconscionable twist with the syntax here
is that `b` is also bound _outside_ the body of the `if` (why??). So
you can still hurt yourself by copy/pasting code and referring to `b`
in the other branches, where it is always null. On the other hand,
one honestly nice thing about this approach is that the little structs
can be used in multiple variants without any tricks. So we have
```
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
```
which I think I even prefer to what you have to do in ML. Note how the
`Value` type includes the possibility of an `Error` result, but an
expression cannot be an error. (You have to start being weird again if
any shared arms are inductive, however). Speaking of which, when an
inductive type like `Exp` has a child of the same type,
`std::shared_ptr<Exp>` is a convenient way to do it. This is reference
counted, which is fine here (expressions cannot have cycles) but not
always acceptable. The reference count is thread safe, which does have
some overhead for synchronization (unnecessary here). If you want to
support cycles or be a little faster, the best way I know how to do it
is to manually manage "arenas" and implement GC yourself.

Tom previously made for his `cc-lib` an arbitrary precision integer
library ([big.h](cc-lib/bignum/big.h)) that can use GMP or be
totally portable, since one of the things he hates the most is
trying to get packaged libraries to work on various computers. This
came in handy here, since as we anticipated, the icfp language uses
very large integers for some of its problems. (They are also quite
useful for encoding data to send back to the server.)

The only other interesting things about the evaluator relate to lambda
applications. This is a statically scoped lazy language, so when we
have a term like `(λx. f x)(y % 0)` you must substitute `y % 0` into
the function's body without evaluating it first. (And note that if the
expression is eventually discarded, you may never actually incur that
division by zero). The two expressions might use the "same" variables,
like in `e1 e2` where `e1 = (λx. λy. x)` and `e2 = (λf. y)`. In this
case the correct result of substitution is _not_ a textual
substitution of `(λf. y)` for `y` in `e1`, giving `(λy. λf. y)`. The
variable `y` in `e2` is a free variable, referring to something
different than the bound variable `y` in `e1` (which refers to the
binding introduced by the lambda right there). A correct result would
be something like `(λz. (λf. y))`, where the bound variable `y` in
`e1` was renamed to `z` or something else that does not conflict.
Substitution that treats this case correctly is called "capture
avoiding substitution." The simple way to do this is to rename each
bound variable to a completely new variable as you recurse into each
lambda during substitution. Renaming a variable is recursive just like
substitution. To avoid writing it twice, the substitution function
takes a `simple` flag, and if `simple` is true then it ignores the
possibility of capture. Then proper substitution calls simple
substitution recursively to perform the renaming. This can be
exponential time, so I first compute the free variables of the
expressions to see whether the simple version will be equivalent
anyway. When you rename variables to fresh ones, you usually get to a
situation where all the variables are unique pretty quickly, and then
the substitutions all become simple.

Finally: Memoization. (TODO)

# Lambdaman

Lambdaman is a Pacman-like two-dimensional puzzle where you need to
collect all of the dots as efficiently as possible. As Jason and
David browsed the set of given problems, we imagined applying
traditional graph optimization techniques, as the setup seemed akin
to Traveling Salesman. Jason started and David took over a Rust
implementation of Lambdaman.

David implemented the simplest search procedure he could imagine. At
each step, it uses breadth-first-search to find the nearest
uncollected dot, and then it goes towards that dot. This algorithm
gave us baseline solutions for all problems (except lambdaman21, which
Tom's evaluator was not yet efficient enough to compute). We were
happy when one of the early solves unlocked the 3D "course", because
that meant we could start parallelizing our efforts more.

At some point, as we re-read the Lambdaman specification, it dawned
on us that Lambdaman is much more of a code golf puzzle than a
graph optimization puzzle. A solution's score is the length of
the *icfp program* that computes it, not the number of moves that
it takes. And moves that step into walls are allowed -- they just
become no-ops.

To facilitate writing icfp-langauge programs by hand, Jason wrote
bunch of Typescript helper functions for constructing expressions, so
that for example instead of

```typescript
   { t: 'binop', opr: '+', a: exp1, b: exp2}
```
we could write
```typescript
   add(exp1, exp2)
```

In a stroke of inspired unhingedness, Jason used Javascript's
`function.toString()` method to implement "higher-order abstract
syntax", meaning that we could model icfp-language functions directly
as Typescript/Javascript functions. (See `getParamNames()` in
[expr.ts](gui/src/expr.ts).) Small issue with that, though: the
behavior of `function.toString()` differed between the browser's
Javascript interpreter and the node Javascript interpreter we were
using from the commandline. David chased down that bug, made the the
HOAS implementation even more hacky to work around it, and resolved to
avoid such sins in the future.

For hand-crafting solutions, the straight-line lambdaman6 and the
empty-square lambdaman9 were obvious places to start. We noticed that
the "best score" reported on the leaderboard indicated that teams were
coming up with quite small programs indeed. With that in mind, David
implemented a random walk program, based on a linear congruential
random number generator. Random walks provided better solutions for a
bunch of problems, but did not suffice for the larger ones, because we
ran up against the 1 million move limit.

David ported over Jason's helper functions (minus HOAS) from
Typescript to Rust and started doing more code golf there. He imagined
eventually implementing a search algorithm that would optimize over
sequences of high-level actions such as "random walk for a while", "do
L for 100 steps", but in the end did not get past manual code
writing.

David wrote icfp-language programs for the fractal problems
[lambdaman16](lambdaman-solver/src/bin/lambdaman16.rs) and
[lambdaman19](lambdaman-solver/src/bin/lambdaman19.rs).

At some point, Tom wrote a compresser that could take a flat string
solution and produce a much smaller icfp program that would evaluate
to that string. We used this approach for problems where we were unable
to get anything working other than the baseline simple search, such as
the mazes in problems 11 through 15.

# Spaceship

# 3D

# Efficiency

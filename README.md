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

Despite the name, 3D was a 2D grid programming language with an interesting
scoring function -- programs were charged for the total volume of space/time
they took up while running. A "time-travel" operator (`@`) allowed values
to be passed backward through time, re-starting computation from a past state
with a value overwritten.

I think Jason was the first to get into this stack of puzzles, and he had
several solves done before we even had a way to run them outside of the
organizers' provided test endpoint.

Once I (Jim, in this part of the section) got done writing several slower,
less elegant versions of the core language interpreter (over in
[seaplusplus/evel.cpp](seaplusplus/evel.cpp)) as a check that I could still
understand the basic lambda calculus and how to run it, I wrote an interpreter
for grid programs ([seaplusplus/3v.cpp](seaplusplus/3v.cpp)) and got into
solving a few more grid problems.

These grid programming puzzles reminded me a lot of the excellent
programming-game-pretending-to-be-a-factory-game Manufactoria, as well as
other factory games in general, since you both need to think about how to
perform a computation with an odd set of operators and how to pack that
computation into a small space(+time) region.

One of the difficulties of working with these grid programs is that text
editors don't have the best (2D) copy-paste operators. This is something
that Jason addressed later in the competition by adding a grid program
visualizer and editor to his typescript contest frontend multitool program
thing. Honestly, it was pretty darn useful (especially for moving code
around and targeting the time-travel jump operator, `@`).

The rest of this section introduces the language

## The Language

A brief overview. Programs are on a grid. Grid cells contain integers, labels, operators, or are empty.

Integers start in the range `-99` - `99` but can grow arbitrarily during program execution.

The labels `A`, `B`, and `S` mark special cells. `A` and `B` are replaced by input values before execution starts, and if `S` is overwritten with another value that value is called the output and the program halts.

Operators take inputs from the cells around them and (generally, except for `@`) write outputs to other cells around them. This is called "reduction" (`~>`) and operators don't "reduce" unless all of their inputs are present and of supported types. Operators reduce in parallel, with all inputs being "taken" before any are written. Any write conflicts cause the program to crash. If no operator can reduce the program halts without producing a value.

The arrow operators (`>`, `<`, `^`, `v`) have one input and can be used to move cell contents:

```
 .  .  .        .  .  .
 6  >  .   ~>   .  >  6
 .  .  .        .  .  .
```

Arrows move in the direction they point and overwrite their output cell:
```
 .  2  .        .  2  .
 .  ^  .   ~>   .  ^  .
 .  7  .        .  .  .
```
Using only arrow operators, the parity of a value's x+y coordinates cannot be changed. That is, (thinking about a checkerboard), black-square and white-square values can never move to the opposite color.

Arrows can move operators and well as values:
```
 4  >  .  .      .  >  4  .      .  >  .  .
 v  >  <  .  ~>  .  >  v  .  ~>  .  >  v  .
 .  .  S  .      4  .  S  .      .  .  4  .
```

This last point about arrows allows making a sort of "conditional move" where an arrow is overwritten by a value to prevent it from firing. (Though this behavior was actually bugged in the official interpreter, and by the time the organizers fixed it I had come up with a different/better solution to the puzzle I wanted to use it in.)

The binary operators `+`, `-`, `*`, `/`, `%`, `=`, and `#` take input from their top/right and write output to their bottom/left. These operators don't run unless both inputs can be taken.

The `+`, `-`, `*`, `/` (integer division, round toward zero), and `%` (mod) operators all follow this pattern:
```
 .  b  .      .  .  .
 a  +  .  ~>  .  + a+b
 .  .  .      . a+b .
```

This lends programs a sort of down-right execution flow, since it's much easier to chain operators downward or rightward than upward or leftward.

Also notice that -- thinking of parity again -- math operators take values of both parities and duplicate their outputs over both parities. This occasionally made routing values between operators somewhat tricky if the wrong input or outputs are chosen (or required, since `-`, `/`, and `%` have asymmetric meaning to their inputs). 

Also, in contrast to the arrow operators, the math operators only work on values, so you can "cork" part of a computation by writing an operator over an input:
```
 .  *  . 
 5  +  . (does not reduce)
 .  .  .
```

The equality and inequality operators reduce like this:
```
 .  b  .      .  .  .
 a  =  .  ~>  .  =  b  (only if a == b)
 .  .  .      .  a  .
```
```
 .  b  .      .  .  .
 a  #  .  ~>  .  #  b  (only if a != b)
 .  .  .      .  a  .
```

Equality and inequality work on both values and operators and turn out to be very useful for a few different plumbing moves.

Inequality can be used to replace any value with a fixed value:
```
 .  *  .  0 .
 5  #  .  # .
 .  .  .  . .
```

Though for some values doing the same thing with math is more compact:
```
 .  0  .
 5  *  .
 .  .  .
```

Inequality can be used to change a value's coordinate parity during routing:
```
 .  .  .  *  .      .  .  .  *  .      .  .  .  .  .
 5  >  .  #  .      .  >  5  #  .      .  >  .  #  *
 .  .  .  .  .  ~>  .  .  .  .  .  ~>  .  .  .  5  .
 .  .  .  v  .      .  .  .  v  .      .  .  .  v  .
 .  .  .  .  .      .  .  .  .  .      .  .  .  .  .x```
I actually had the `#` reduction wrong (swapped `a` and `b` in the output) initially in my interpreter. This would have been nice (let values "cross" each-other in the grid), but also prevents this parity-swapping trick from working.

Inequality can also be used to "turn around" at the top or bottom of a "delay line" in a more compact way than an arrow could:
```
 .  *  .  *  .  *
 .  #  .  #  .  #
 ^  .  ^  .  ^  .
 .  v  .  v  .  v
 ^  .  ^  .  ^  .
 3  v  .  v  .  v
 .  .  ^  .  ^  .
 *  #  .  v  .  v
 .  .  .  .  ^  .
 .  .  *  #  .  .
```
(Slightly more awkward at the bottom because the arrow operators steal the `*` if it isn't out of the way enough.)

Equality can shift the outputs of other operators diagonally in the grid:
```
 .  1  .  .      .  .  .  .      .  .  .  .
 1  +  .  .  ~>  .  +  2  .  ~>  .  +  .  .
 .  .  =  .      .  2  =  .      .  .  =  2
 .  .  .  .      .  .  .  .      .  .  2  .
```

You can do the same sort of trick with inequality to make a sort of "value slide":
```
 .  *  .  .      .  .  .  .      .  .  .  .
 2  #  .  .  ~>  .  #  *  .  ~>  .  #  .  .
 .  .  #  .      .  2  #  .      .  .  #  *
 .  .  .  .      .  .  .  .      .  .  2  .
```

Finally, the time-travel operator `@`:
```
 .  c  .
 dx @ dy
 . dt  .
```

This restores the board state from `dt` ticks ago, but copies the value or operator `c` to the cell that is `dx` steps to the left of and `dy` steps above the `@`. Multiple time travels can write results into the past at the same time, but if those values conflict (or if the `dt`'s don't match) the program "crashes" without returning a value.

The time travel operator is the spice that keeps this language interesting.


## The Puzzles


### 3d11 -- Lambdaman Path

Statement: given a large integer representing a set of 2D steps (up to 100) in space, report how many unique coordinate pairs are visited by the path.

This one was my (Jim's) nemesis during the competition because I had three good ideas for solutions, all of which were notionally "fast enough" (fit in the established area and tick limit), and none of which the organizers' interpreter could actually run fast enough.
(Well, I ran out of time on a revision of one of the ideas, it might have eventually been fast enough.)

The core trouble being that this problem invites you to think about time complexity like a ciruit (parallel operations are free, depth is the main cost), but that the organizers' evaluator was not actually fast enough to support the "obvious" / "efficient" circuit-like implementations.


Anyway, the basic infrastructure for this problem was just a digit-peeler and a few little networks to accumulate current position.
After hours of working in the language this was straightforward to write.

The thing that caused me consternation was how to store the set of visited cells.

The first thing I tried was to **accumulate a bit-field** into a large integer value ([solutions/threed/threed11-bits.txt]).
This worked fine but took a lot of real evaluation steps and caused the organizers' interpreter to time out (despite this solution being well under the area and tick limits). I suppose bigints with hundreds of digits cause things to run slowly (I half-suspect that the organizers were trying to pretty-print the outputs and the real problem was formatting those integers as nicely aligned table columns; I know that's where my interpreter was burning cycles).

I am especially proud of this extendible N-bit-per-time-loop bit counter setup though (this is the 4-bit version):
```
 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
 .  .  .  .  .  .  .  1  .  2  .  0  .  .  .  .  .  . 
 .  .  .  .  .  <  A  *  .  %  .  +  .  .  .  .  .  . 
 .  .  .  0  =  .  .  .  .  .  .  .  =  .  .  .  .  . 
 .  .  .  .  .  .  .  v  2  .  2  .  .  .  .  .  .  . 
 .  .  .  .  v  .  .  .  /  .  %  .  +  .  .  .  .  . 
 .  .  .  <  .  >  .  .  .  .  .  .  .  =  .  .  .  . 
 . -2  @  4  v -8  @ -6  v  2  .  2  .  .  .  .  .  . 
 .  .  3  .  .  .  3  .  .  /  .  %  .  +  .  .  .  . 
 .  .  . -7  @ -7  .  .  .  .  .  .  .  .  =  .  .  . 
 .  .  .  .  3  .  .  .  .  v  2  .  2  .  .  .  .  . 
 .  .  .  .  .  .  .  .  .  .  /  .  %  .  +  .  .  . 
 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  >  .  . 
 .  .  .  .  .  .  .  .  .  .  v  2  .  3  @ 12  v  . 
 .  .  .  .  .  .  .  .  .  .  .  /  .  .  9  .  S  . 
 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
 .  .  .  .  .  .  .  .  .  .  5  @ 14  .  .  .  .  . 
 .  .  .  .  .  .  .  .  .  .  .  9  .  .  .  .  .  . 
 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
```
It's a compact structure, you can extend it to peel more bits relatively easilly, and it kinda looks like a penis especially if you increase the bits-peeled count (which has a certain low-humor value to it).


With that running too slowly, I decided to go wide and implemented the first thing I actually thought of when reading the problem (but dismissed in favor of the bitfield solution), which was to use a 200x200-ish 2D look-up table to store the visit counts of each cell. Basically, this replaced "accumate into a bit vector" with "write `1`'s into this structure":
```
 . 1 . 0 . 0 . 0 . .
 0 + . + . + . + . .
 . 0 . 0 . 0 . . . .
 0 + . + . + . + . .
 . 0 . 0 . 0 . . . .
 0 + . + . + . + . .
 . . . . . . . S . .
```
(This is the 3x3 version; the "real" version of the code used a 101x101 version.)
It's actually pretty cool that you can make an accumulator like this so compactly (2x2 cells).

Also notice that this looks like it will just sum up the whole grid and then immediately write the result, halting the program. But what actually happens is that the logic that fills in the grid will always time-travel before the grid finishes summing. So only when that logic quiesces will the final summation be reported. Elegant!

This finishes in a lot fewer reduction steps than the previous solution, but takes up a lot more grid area, which again seemed to make the organizer's simulator choke.


Finally, I built a "trampoline" solution that would jump into each cell of a grid, then have each cell jump back with a `1` or `0` based on whether the cell had been visited. This took even more area but the number of active reductions was much smaller. This was a bit of a futile attempt to optimize for execution speed on whatever black box the contest organizers were actually running these things on. It ended up taking too much grid area.


After the contest ended I realized I could have just made a storage for 101 x-y locations and though it would have been more expensive per step it probably was the intended solution. Ah, well.

## Hypothetical: Writing 2-Tick Programs

An observation that I didn't really get too far with during the contest (I really wanted to finish 3d11; but failed) is that you can trade space for total time taken by inserting time travel between each operator (effectively: pipelining the computation).
This matters because the score of a solution is the volume of the space-time bounding box around it.

**Example:** this computes `((A+B)*5)/2` in 3 ticks and has an X-Y-Time bounding box area of `7*3*3 = 63`:
```
 .  .  B  .  5  .  2  .  .  . 
 .  A  +  .  *  .  /  S  .  . 
 .  .  .  .  .  .  .  .  .  . 
```

This computes the same thing using only two distinct time values, for a total X-Y-Time bounding box area of `9*5*2 = 90`:
```
 .  .  B  .  .  5  .  .  2  .  .
 .  A  +  .  .  *  .  .  /  S  .
 .  .  .  .  .  .  .  .  .  .  . 
 .  -2 @  2 -2  @  2  .  .  .  . 
 .  .  1  .  .  1  .  .  .  .  .
```

Okay, it's not a savings in this case, but with larger expressions that visit many real times it can actually be an advantage.


One early concern I had with the strategy is that it seems to not support fan-out, but you can just move fan-out into its own stages:
```
 .  .  <  A  >  .  .
x1  @ y1  v x2  @ y2
 . t1  .  .  . t2  .
 .  . x3  @ y3  .  .
 .  .  . t3  .  .  .
```

The second concern is that while this works well for straight-line programs, it might have problems with propogating junk values in programs which are already using time travel for loops. I think this can be addressed by using some extra "clock" or "valid" signals, but it might require moving to a 3-tick setup. I didn't work this out fully.

Anyway, this was a nice idea and I hope to have the leisure to revisit it at some point.
Not sure if it would have been contest-breaking but would definitley have improved the performance of some of our straight-line solutions.


# Efficiency

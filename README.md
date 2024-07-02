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
server is just using string literals, so you can do this with only
a simple encoder/decoder. So while Tom was at this ...

(please describe your things!!)

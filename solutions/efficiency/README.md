Efficiency solutions
====================

efficiency1
-----------

```
((λ x (x (x (x (x (x (x (x (x (x (x (x (x (x (x (x (x (x (x (x (x (x (x 1)))))))))))))))))))))))
   (λ x (+ (+ x x) (+ x x))))
```

This computes
$$4^{22} = 17592186044416$$
So we do

```
solve efficiency1 17592186044416
```

efficiency2
-----------
The raw program is
```
(+ 2134 (* (((λ x ((λ y (x (y y))) (λ y (x (y y))))) (λ z (λ a (if (= a 0) 1 (+ 1 (z (- a 1))))))) 9345873499) 0))
```
which is
```
(+ 2134 (* ((fix z (λ a (if (= a 0) 1 (+ 1 (z (- a 1)))))) 9345873499) 0))
```
i.e.
```
fun z 0 = 1
  | z a = 1 + z (a-1)

2134 + (0 * z 9345873499)
```

So the function $z$ is just a ticking clock. The solution is $2134$.

efficiency3
-----------

Raw program
```
(+ 2134 (* (((λ x ((λ y (x (y y))) (λ y (x (y y))))) (λ z (λ a (if (= a 0) 1 (+ 1 (z (- a 1))))))) 9345873499) 1))
```
which we analyze as
```
(+ 2134 (* ((fix z (λ a (if (= a 0) 1 (+ 1 (z (- a 1)))))) 9345873499) 1))
```
which is
```
fun z 0 = 1
  | z a = 1 + z (a-1)

2134 + (1 * z 9345873499)
```
which should be (because the base case of z is 1)
$$2134 + 9345873500 = 9345875634$$

efficiency4
-----------

Raw program is

```
(((λ x ((λ y (x (y y))) (λ y (x (y y))))) (λ z (λ a (if (< a 2) 1 (+ (z (- a 1)) (z (- a 2))))))) 40)
```

so

```
((fix z (λ a (if (< a 2) 1 (+ (z (- a 1)) (z (- a 2)))))) 40)
```
i.e.
```
fun z 0 = 1
 |  z 1 = 1
 |  z a = z(a-1) + z(a-2)
z 40
```
so the 40th fibonacci number 165580141

Note: this would be accessible to direct eval if we had caching call-by-need or something

efficiency5
-----------

Raw program is
```
((λ c ((λ d (((λ x ((λ y (x (y y))) (λ y (x (y y))))) (λ z (λ a (if (& (> a 1000000) (& (c a) (d (+ a 1)))) a (z (+ a 1)))))) 2)) ((λ x ((λ y (x (y y))) (λ y (x (y y))))) (λ z (λ a (if (= a 1) true (if (= (% a 2) 1) false (z (/ a 2))))))))) (λ b (((λ x ((λ y (x (y y))) (λ y (x (y y))))) (λ z (λ a (if (= a b) true (if (= (% b a) 0) false (z (+ a 1))))))) 2)))
```
so that's
```
let c = (λ b ((fix  z (λ a (if (= a b) true (if (= (% b a) 0) false (z (+ a 1)))))) 2))
let d = (fix  z (λ a (if (= a 1) true (if (= (% a 2) 1) false (z (/ a 2))))))
(fix z (λ a (if (& (> a 1000000) (& (c a) (d (+ a 1)))) a (z (+ a 1))))) 2
```

Which is morally:
```
fun prime b =
   let fun z1 = (λ a (if (= a b) true (if (= (% b a) 0) false (z1 (+ a 1)))))
   in z1 2

let ispow2 = (fix z2 (λ a (if (= a 1) true (if (= (% a 2) 1) false (z2 (/ a 2))))))

((fix first_big_pow2_prime (λ a (if (& (> a 1000000) (& (prime a) (ispow2 (+ a 1)))) a (first_big_pow2_prime (+ a 1))))) 2)
```
so the answer is $2^{31} - 1$.

efficiency6
-----------

The cleaned up code is
```
let is_prime = (λ b ((fix (λ z (λ a (if (= a b) true (if (= (% b a) 0) false (z (+ a 1))))))) 2))

let fib = (fix (λ z (λ a (if (< a 2) 1 (+ (z (- a 1)) (z (- a 2)))))))

((fix (λ z (λ a (if (& (> a 30) (is_prime (fib a))) a (z (+ a 1)))))) 2)
```
so we're meant to submit the smallest $a > 30$ such that the $a^{th}$ fibonacci number is prime, which is 42.

efficiency7
-----------

This looks like a 3SAT problem. Throw it at Z3.

efficiency8
-----------

This is a SAT problem. Tom converted it to Z3 with cc/pp.exe and some
emacs macros (see e8.z3). Note: It's searching for the *minimal* solution.

The answer is 422607674157562.

efficiency9
-----------

This is some kind of constraint solving problem that is picking out digits
in base 9. Z3 could be helpful.

efficiency10
------------

Similar to previous.

efficiency11
------------

Similar to previous.

efficiency12
------------

It seems like this program looks like

```
fun z a =
   let
      fun b c d = if c = a then d else (b (c+1) (if ((z c) > (c - 1)) (if (= (% a c) 0) (* (/ d (z c)) (- (z c) 1)) d) d))

   in
   ((min a (+ 1 (if (> a 2) (b 2 a) a))))
z 1234567
```

In javascript this is
```
function loop(a) {
  function aux (c, d) {
    if (c == a) {
      return d;
    }
    const newd = ((loop(c) <= c-1) || (a % c) != 0) ? d : (k => (Math.floor(d/k) * (k-1))) (loop(c));
    return aux(c+1, newd);
  }
  return Math.min(a, (a > 2 ? aux(2, a) : a) + 1);
}
console.log(loop(parseInt(process.argv[2])));
```
and I confirmed up to n=7 that it's doing the same thing as our evaluator.

OEIS search suggests this is [A039649](https://oeis.org/A039649),
and submitting $\phi(1234567)+1 = 1224721$ was successful.

efficiency13
------------

This program prepends $2^28$ copies of \texttt{na} to \texttt{heyjude} and computes its length.
That amounts to
\[ 2^{29} + 7 = 536870919\]

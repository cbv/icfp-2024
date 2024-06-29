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
so the 40th fibonacci function 165580141

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

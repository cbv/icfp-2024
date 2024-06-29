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

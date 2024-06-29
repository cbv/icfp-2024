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
(+ 2134 (* (((λ y ((λ z (y (z z))) (λ z (y (z z))))) (λ w (λ a (if (= a 0) 1 (+ 1 (w (- a 1))))))) 9345873499) 0))
```
which is
```
(+ 2134 (* ((fix w (λ a (if (= a 0) 1 (+ 1 (w (- a 1)))))) 9345873499) 0))
```

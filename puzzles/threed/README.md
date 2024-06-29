threed
======

summary of puzzles

1. factorial
2. absolute value
3. sign
4. max
5. lcm
6. primality testing
7. palindrome testing
8. smallest base for which a number is a palindrome
9. parentheses balancing
10. parentheses and brackets balancing
11. lambdaman path
12. sine

# Input
  `-1570796327 <= A <= 1570796327`

# Output
  `truncate(sin(A / 1_000_000_000) * 1_000_000_000)`,

sine
----
I think the hint about Horner's method is suggesting that we compute

$$ x - x^3/3! + x^5/5! - x^7/7! + \cdots$$

as

$$ x(1 - x^2(1/3! - x^2(1/5! - x^2(1/7! - \cdots))))$$

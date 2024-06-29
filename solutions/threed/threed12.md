Here's an approach to organizing the arithmetic for 3d12.
We do fixedpoint arithmetic with a scaling factor *slightly*
larger than 1e9, by a factor of `FUDGE`, and this gets us enough
accuracy to be within 1 of test cases they provide.

```
BASE = 1e9;
FUDGE = 2;
TERMS = 6;

function mul(a,b) {
  return (a * b) / (BigInt(FUDGE) * BigInt(BASE));
}

// return x * n^m
function pow(x, n, m) {
  if (m == 0) return x;
  return mul(pow(x, n, m-1), n);
}
function div(a, b) {
  return Math.floor(a/b);
}
function factorial(n) {
  if (n == 0) return BigInt(1);
  return BigInt(n) * factorial(n-1);
}

function sine(raw) {
  x = raw * BigInt(FUDGE);
  let acc = BigInt(0);
  for (let i = 0; i < TERMS; i++) {
    acc += BigInt(i % 2 == 0 ? 1 : -1) * pow(x / factorial(1 + 2 * i), x, 2 * i);
  }
  return acc / BigInt(FUDGE);
}

// # Example
//   * `A = 1047197551`
//     `Answer = 866025403`
//   * `A = -1168378317`
//     `Answer = -920116684`
console.log(sine(BigInt(1047197551)) +  "\n" + BigInt(866025403) + "\n");
console.log(sine(BigInt(-1168378317)) +  "\n" + BigInt(-920116684) + "\n");
```

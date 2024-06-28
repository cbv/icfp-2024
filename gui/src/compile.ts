import { repeat } from './lib/util';
import { Expr, add, app, concat, cond, equ, expToIcfp, expToToks, lam, litbool, litnum, litstr, mul, rawlam, rec, sub, vuse } from './expr';

/*

repeat s n = if n = 1 then s else s . repeat s n

*/

function pair(x1: Expr, x2: Expr): Expr {
  return lam(x => cond(x, x1, x2));
}

function fst(x: Expr): Expr {
  return app(x, litbool(true));
}

function snd(x: Expr): Expr {
  return app(x, litbool(false));
}

function appSpine(a: Expr, b: Expr[]): Expr {
  return [a, ...b].reduce(app);
}

type Binding = { v: string, body: Expr };
function letbind(bindings: Binding[], body: Expr): Expr {
  bindings.reverse().forEach(binding => {
    body = app(rawlam(binding.v, body), binding.body);
  });
  return body;
}

export function compileExample() {
  // let rv = '';
  // for (let i = 1; i < 48; i++) {
  //   rv += repeat('D', 2 * i) + repeat('L', 2 * i);
  //   rv += repeat('U', 2 * i + 2) + repeat('R', 2 * i + 2);
  // }
  // rv += repeat('D', 98);
  // rv += repeat('L', 97);

  // repeat "D" 3 = "DDD"
  const R2 = rec(R => lam(s => lam(n => cond(equ(n, litnum(1)), s, concat(s, appSpine(R, [s, sub(n, litnum(1))]))))));
  let repeat = vuse('R2');
  const spiral = letbind(
    [{ v: 'R2', body: R2 }],
    rec(S => lam(n =>
      // have we finished?
      cond(equ(n, litnum(98)),
        // If so, do the last couple moves
        concat(
          appSpine(repeat, [litstr("D"), litnum(98)]),
          appSpine(repeat, [litstr("L"), litnum(97)])),
        // Otherwise do some moves and recurse
        concat(
          concat(
            concat(
              appSpine(repeat, [litstr("D"), n]),
              appSpine(repeat, [litstr("L"), n])),
            concat(
              appSpine(repeat, [litstr("U"), add(n, litnum(2))]),
              appSpine(repeat, [litstr("R"), add(n, litnum(2))])),
          ),
          app(S, add(n, litnum(4)))
        ),
      ))));

  const fact = rec(f => lam(x => cond(equ(litnum(0), x),
    litnum(1),
    mul(x, app(f, sub(x, litnum(1)))))));

  return expToIcfp(concat(litstr("solve lambdaman8 "), appSpine(spiral, [litnum(2)])));
}

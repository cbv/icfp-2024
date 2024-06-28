import { repeat } from './lib/util';
import { Expr, app, concat, cond, equ, expToIcfp, expToToks, lam, litbool, litnum, litstr, mul, rec, sub } from './expr';

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

export function compileExample() {
  // let rv = '';
  // for (let i = 1; i < 48; i++) {
  //   rv += repeat('D', 2 * i) + repeat('L', 2 * i);
  //   rv += repeat('U', 2 * i + 2) + repeat('R', 2 * i + 2);
  // }
  // rv += repeat('D', 98);
  // rv += repeat('L', 97);

  // repeat "D" 3 = "DDD"
  const repeat = rec(R => lam(s => lam(n => cond(equ(n, litnum(1)), s, concat(s, appSpine(R, [s, sub(n, litnum(1))]))))));

  const fact = rec(f => lam(x => cond(equ(litnum(0), x),
    litnum(1),
    mul(x, app(f, sub(x, litnum(1)))))));

  return expToIcfp(appSpine(repeat, [litstr("D"), litnum(10)]));

}

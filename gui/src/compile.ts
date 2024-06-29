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

export function lambdaman6() {
  const repeat = rec(R => lam(n => cond(equ(n, litnum(1)), litstr("R"), concat(litstr("R"), appSpine(R, [sub(n, litnum(1))])))));
  let quad = vuse("q");
  return expToIcfp(concat(litstr("solve lambdaman6 "), letbind([{
    v: 'q', body:
      lam(a => concat(concat(a, a), concat(a, a)))
  }], app(quad, app(quad, app(quad, litstr("RRRR")))))));
}

export function lambdaman8() {
  // let rv = '';
  // for (let i = 1; i < 48; i++) {
  //   rv += repeat('D', 2 * i) + repeat('L', 2 * i);
  //   rv += repeat('U', 2 * i + 2) + repeat('R', 2 * i + 2);
  // }
  // rv += repeat('D', 98);
  // rv += repeat('L', 97);

  // repeat "D" 3 = "DDD"
  const R = rec(r => lam(s => lam(n => cond(equ(n, litnum(1)), s, concat(s, appSpine(r, [s, sub(n, litnum(1))]))))));
  let repeat = vuse('R');
  const Mbody = litnum(205);
  const M = vuse('M')
  const spiral = letbind(
    [{ v: 'R', body: R }, { v: 'M', body: Mbody }],
    rec(S => lam(n =>
      // have we finished?
      cond(equ(n, litnum(98)),
        // If so, do the last couple moves
        appSpine(repeat, [litstr("DL"), M]),
        // Otherwise do some moves and recurse
        concat(
          concat(
            appSpine(repeat, [litstr("DL"), M]),
            appSpine(repeat, [litstr("UR"), M])
          ),
          app(S, add(n, litnum(4)))
        ),
      ))));

  return expToIcfp(concat(litstr("solve lambdaman8 "), appSpine(spiral, [litnum(2)])));
}

export function compileExample() {
  return lambdaman8();
}

import { repeat } from './lib/util';
import { Expr, add, mod, app, concat, cond, equ, expToIcfp, expToToks, lam, litbool, litnum, litstr, mul, rawlam, rec, sub, div, vuse } from './expr';

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

export function lambdaman4() {
  // 21 x 21 maze

  //  const  = rec(r => lam(s => lam(n => cond(equ(n, litnum(1)), s, concat(s, appSpine(r, [s, sub(n, litnum(1))]))))));
  //  let repeat = vuse('R');
  const zag =
    rec(S => lam(n => lam(r =>
      // have we finished?
      cond(equ(n, litnum(1000000)),
        // done
        litstr(""),
        // Otherwise do some moves and recurse
        concat(
          letbind([{ v: 'x', body: mod(div(r, litnum(0x3fffffff)), litnum(4)) }],
            cond(equ(litnum(0), vuse('x')),
              litstr("U"),
              cond(equ(litnum(1), vuse('x')),
                litstr("D"),
                cond(equ(litnum(2), vuse('x')),
                  litstr("L"),
                  litstr("R"))))),
          appSpine(S, [add(n, litnum(1)),
          mod(add(mul(r, litnum(1664525)), litnum(1013904223)),
            litnum(0xffffffff))]))))));

  return expToIcfp(concat(litstr("solve lambdaman4 "), appSpine(zag, [litnum(0), litnum(0)])));

}

export function lambdaman6() {
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
      cond(equ(n, litnum(25)),
        // done
        litstr(""),
        // Otherwise do some moves and recurse
        concat(
          concat(
            appSpine(repeat, [litstr("DL"), M]),
            appSpine(repeat, [litstr("UR"), M])
          ),
          app(S, add(n, litnum(1)))
        ),
      ))));

  return expToIcfp(concat(litstr("solve lambdaman8 "), appSpine(spiral, [litnum(0)])));
}

export function lambdaman9() {
  // 50 x 50 grid with all empties. Start in upper left.
  const R = rec(r => lam(s => lam(n => cond(equ(n, litnum(1)), s, concat(s, appSpine(r, [s, sub(n, litnum(1))]))))));
  let repeat = vuse('R');
  const zag = letbind(
    [{ v: 'R', body: R }],
    rec(S => lam(n =>
      // have we finished?
      cond(equ(n, litnum(25)),
        // done
        litstr(""),
        // Otherwise do some moves and recurse
        concat(
          concat(
            concat(
              appSpine(repeat, [litstr("R"), litnum(50)]),
              litstr("D")),
            concat(
              appSpine(repeat, [litstr("L"), litnum(50)]),
              litstr("D"))),
          app(S, add(n, litnum(1))))))));

  return expToIcfp(concat(litstr("solve lambdaman9 "), appSpine(zag, [litnum(0)])));
}

export function compileExample() {
  return lambdaman8();
}

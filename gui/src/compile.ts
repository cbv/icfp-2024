import { repeat } from './lib/util';
import { app, cond, equ, expToIcfp, expToToks, lam, litnum, litstr, mul, rec, sub } from './expr';
export function compileExample() {
  // let rv = '';
  // for (let i = 1; i < 48; i++) {
  //   rv += repeat('D', 2 * i) + repeat('L', 2 * i);
  //   rv += repeat('U', 2 * i + 2) + repeat('R', 2 * i + 2);
  // }
  // rv += repeat('D', 98);
  // rv += repeat('L', 97);

  const fact = rec(f => lam(x => cond(equ(litnum(0), x),
    litnum(1),
    mul(x, app(f, sub(x, litnum(1)))))));
  return expToIcfp(app(fact, litnum(3)));

}

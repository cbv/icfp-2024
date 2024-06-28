import { repeat } from './lib/util';
import { expToIcfp, expToToks, litnum, litstr } from './expr';
export function compileExample() {
  // let rv = '';
  // for (let i = 1; i < 48; i++) {
  //   rv += repeat('D', 2 * i) + repeat('L', 2 * i);
  //   rv += repeat('U', 2 * i + 2) + repeat('R', 2 * i + 2);
  // }
  // rv += repeat('D', 98);
  // rv += repeat('L', 97);

  return expToIcfp(litstr("get index"));
}

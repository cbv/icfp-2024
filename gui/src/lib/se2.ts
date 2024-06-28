import { Point } from "./types";
import { vm, vm2, vm3 } from "./vutil";

export type SE2 = {
  scale: Point,
  translate: Point,
}

export function apply(a: SE2, x: Point): Point {
  return vm3(a.scale, a.translate, x, (s, t, x) => s * x + t);
}

export function compose(a: SE2, b: SE2): SE2 {
  return {
    scale: vm2(a.scale, b.scale, (a, b) => a * b),
    translate: vm3(a.scale, a.translate, b.translate, (a_s, a_t, b_t) => a_s * b_t + a_t),
  }
}

export function composen(...xforms: SE2[]): SE2 {
  return xforms.reduce(compose);
}

export function inverse(a: SE2): SE2 {
  return {
    scale: vm(a.scale, a => 1 / a),
    translate: vm2(a.scale, a.translate, (s, t) => -t / s),
  }
}

export function ident(): SE2 {
  return {
    scale: { x: 1, y: 1 },
    translate: { x: 0, y: 0 },
  }
}


export function translate(p: Point): SE2 {
  return {
    scale: { x: 1, y: 1 },
    translate: { x: p.x, y: p.y },
  }
}

export function scale(p: Point): SE2 {
  return {
    scale: { x: p.x, y: p.y },
    translate: { x: 0, y: 0 },
  }
}

export function mkSE2(scale: Point, translate: Point): SE2 {
  return { scale, translate };
}

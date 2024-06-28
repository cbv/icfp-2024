import { apply, SE2 } from "./se2";
import { Point, Rect } from "./types";
import { vm2, vmul, vsub } from "./vutil";

export function apply_to_rect(a: SE2, x: Rect): Rect {
  return {
    p: apply(a, x.p),
    sz: vm2(a.scale, x.sz, (s, sz) => s * sz)
  }
}

// Suppose we have a transform t, and points p0 and p1.
// We want to solve the problem of finding a transform u such that
// u(p₀) = p₁ and also that u and t only differ by a translation,
// i.e. they share the same scale factor.
export function matchScale(t: SE2, p0: Point, p1: Point): SE2 {
  // t = mp + ⋯
  // u = mp + k
  // u(p₀) = p₁
  // ∴ mp₀ + k = p₁
  // ∴  k = p₁ - mp₀
  return {
    scale: t.scale,
    translate: vsub(p1, vmul(p0, t.scale)),
  };
}

// Returns homogeneous coordinate affine transform matrix
// [c11, c12, c13,
//  c21, c22, c23,
//  c31, c32, c33]
// sutiable for shipping off to WebGL
export function asMatrix(u: SE2): number[] {
  const { scale: s, translate: t } = u;
  return [
    s.x, 0, 0,
    0, s.y, 0,
    t.x, t.y, 1,
  ]
}

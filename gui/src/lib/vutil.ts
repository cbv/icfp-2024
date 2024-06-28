import { Point } from './types';

export function int(x: number): number {
  return Math.floor(x);
}

export function mod(x: number, y: number): number {
  var z = x % y;
  if (z < 0) z += y;
  return z;
}

export function div(x: number, y: number): number {
  return int(x / y);
}

export function vm(a: Point, f: (a: number) => number): Point {
  return { x: f(a.x), y: f(a.y) };
}

export function vm2(a: Point, b: Point, f: (a: number, b: number) => number): Point {
  return { x: f(a.x, b.x), y: f(a.y, b.y) };
}

export function vm3(a: Point, b: Point, c: Point, f: (a: number, b: number, c: number) => number): Point {
  return { x: f(a.x, b.x, c.x), y: f(a.y, b.y, c.y) };
}

export function vmn(ps: Point[], f: (ns: number[]) => number): Point {
  return { x: f(ps.map(p => p.x)), y: f(ps.map(p => p.y)) };
}

export function vequal(a: Point, b: Point): boolean {
  return a.x == b.x && a.y == b.y;
}

export function vadd(a: Point, b: Point): Point {
  return { x: a.x + b.x, y: a.y + b.y };
}

export function vsub(a: Point, b: Point): Point {
  return { x: a.x - b.x, y: a.y - b.y };
}

export function vmul(a: Point, b: Point): Point {
  return { x: a.x * b.x, y: a.y * b.y };
}

export function vscale(b: Point, s: number): Point {
  return { x: s * b.x, y: s * b.y };
}

export function vdiv(b: Point, s: number): Point {
  return { x: b.x / s, y: b.y / s };
}

export function vinv(b: Point): Point {
  return { x: 1 / b.x, y: 1 / b.y };
}

export function vint(v: Point): Point {
  return { x: int(v.x), y: int(v.y) };
}

export function vfpart(v: Point): Point {
  return { x: v.x - int(v.x), y: v.y - int(v.y) };
}

export function vtrans(v: Point): Point {
  return { x: v.y, y: v.x };
}

export function vsnorm(v: Point): number {
  return v.x * v.x + v.y * v.y;
}

// Returns a unit vector in the direction of v
// v must not be the zero vector.
export function vunit(v: Point): Point {
  return vscale(v, 1 / Math.sqrt(vsnorm(v)));
}

export function vdiag(x: number): Point {
  return { x: x, y: x };
}

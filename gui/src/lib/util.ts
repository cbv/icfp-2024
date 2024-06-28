import { Point, Rect } from "./types";
import { vadd, vm, vm2, vm3, vmul, vscale, vsub } from "./vutil";

export function mapval<T, U>(m: { [k: string]: T }, f: (x: T, k: string) => U): { [k: string]: U } {
  return Object.fromEntries(Object.entries(m).map(([k, v]) => [k, f(v, k)]));
}

export class Rand {
  n: number;
  constructor(n?: number) { this.n = n || 42; for (let i = 0; i < 3; i++) this.f(); }
  f(): number {
    this.n = (2147483629 * this.n + 2147483587) % 2147483647;
    return (this.n & 0xffff) / (1 << 16);
  }
  i(n: number): number {
    return Math.floor(this.f() * n);
  }
}

export function zeropad(str: string, length: number): string {
  return repeat('0', Math.max(0, length - str.length)) + str;
}

export function repeat(x: string, n: number) {
  if (n < 0)
    throw 'negative repeat';

  let s = '';
  for (; ;) {
    if (n & 1) s += x;
    n >>= 1;
    if (n) x += x;
    else break;
  }
  return s;
}

export const int = Math.floor;

export function lerp(a: number, b: number, t: number) {
  return a + (b - a) * t;
}

// multiplicative interpolation
export function mlerp(a: number, b: number, t: number) {
  return Math.exp(lerp(Math.log(a), Math.log(b), t));
}

export function unreachable<T>(_x: never): T {
  throw new Error('unreachable');
}

export function filterKeys<T>(rec: Record<string, T>, pred: (x: string) => boolean): Record<string, T> {
  return Object.fromEntries(
    Object.entries(rec)
      .filter(([k, _v]) => pred(k))
  );
}

// RtlUniform from Native API[13] from
// https://en.wikipedia.org/wiki/Linear_congruential_generator
export function next_rand(seed: number): { seed: number, float: number } {
  const next = (2147483629 * seed + 2147483587) % 2147483647;
  return { seed: next, float: (next & 0xffff) / (1 << 16) };
}

export function boundRect(points: Point[]): Rect {
  const xs = points.map(p => p.x);
  const ys = points.map(p => p.y);
  const mins = { x: Math.min(...xs), y: Math.min(...ys) };
  const maxs = { x: Math.max(...xs), y: Math.max(...ys) };
  return { p: mins, sz: vsub(maxs, mins) };
}

export function pointInRect(p: Point, r: Rect): boolean {
  return p.x >= r.p.x && p.y >= r.p.y && p.x <= r.p.x + r.sz.x && p.y <= r.p.y + r.sz.y;
}

export function point_hash(p: Point, seed: number): number {
  let c = seed + 1000 * p.x + 3758 * p.y;

  for (let i = 0; i < 20; i++) {
    const { seed } = next_rand(c);
    c = seed;
  };
  return next_rand(c).float;
}

export function midpointOfRect(rect: Rect): Point {
  return vm2(rect.p, rect.sz, (p, s) => p + s / 2);
}

export function scaleRectToCenter(rect: Rect, s: number): Rect {
  return {
    p: vm2(rect.p, rect.sz, (p, sz) => (s * (p - (p + sz / 2)) + (p + sz / 2))),
    sz: vscale(rect.sz, s),
  }
}

export function scaleRectToCenterPoint(rect: Rect, sf: Point): Rect {
  return {
    p: vm3(rect.p, rect.sz, sf, (p, sz, s) => (s * (p - (p + sz / 2)) + (p + sz / 2))),
    sz: vmul(rect.sz, sf),
  }
}

export function insetRect(rect: Rect, i: number): Rect {
  return {
    p: vm(rect.p, p => p + i),
    sz: vm(rect.sz, sz => sz - 2 * i),
  }
}

// Returns a random permutation of [0,...,length-1].
export function getRandomOrder(length: number): number[] {
  const rv: [number, number][] = [];
  for (let i = 0; i < length; i++) {
    rv.push([i, Math.random()]);
  }
  rv.sort((a, b) => a[1] - b[1]);
  return rv.map(a => a[0]);
}

export function range(n: number): number[] {
  return [...new Array(n)].map((_a, b) => b);
}

export function randPointInRect(rect: Rect): Point {
  return vm2(rect.p, rect.sz, (p, sz) => Math.random() * sz + p);
}

export async function grab(path: string): Promise<string> {
  const resp = await fetch(path, { cache: "no-cache" });
  if (resp.status != 200)
    throw new Error(`Couldn't load ${path}, status code ${resp.status}`);
  return await resp.text();
}

export function rectPts(rect: Rect): [Point, Point] {
  return [rect.p, vadd(rect.p, rect.sz)];
}

export function allRectPts(rect: Rect): Point[] {
  return [rect.p, { x: rect.p.x + rect.sz.x, y: rect.p.y }, vadd(rect.p, rect.sz), { x: rect.p.x, y: rect.p.y + rect.sz.y }];
}

// Gives a rect covering the same area but "negatively", so that
// allRectPts(invertRect(r)) = allRectPts(r).reverse()
export function invertRect(rect: Rect): Rect {
  return { p: { x: rect.p.x + rect.sz.x, y: rect.p.y }, sz: { x: -rect.sz.x, y: rect.sz.y } }
}

export function pixelSnap(p: Point): Point {
  return vm(p, x => Math.floor(x * devicePixelRatio) / devicePixelRatio);
}

export function pixelSnapRect(r: Rect): Rect {
  return { p: pixelSnap(r.p), sz: pixelSnap(r.sz) };
}

export function fpart(x: number): number {
  return x - Math.floor(x);
}

export function flatUndef<A>(xs: (undefined | A)[]): undefined | A[] {
  const rv: A[] = [];
  for (const x of xs) {
    if (x == undefined) return undefined;
    rv.push(x);
  }
  return rv;
}

export function findLast<T>(xs: T[], f: (x: T) => boolean): T | undefined {
  for (let i = xs.length - 1; i >= 0; i--) {
    if (f(xs[i])) return xs[i];
  }
  return undefined;
}

export function findLastIndex<T>(xs: T[], f: (x: T) => boolean): number {
  for (let i = xs.length - 1; i >= 0; i--) {
    if (f(xs[i])) return i;
  }
  return -1;
}

export function arrayJoin<T>(xs: T[], delim: T): T[] {
  const rv: T[] = [];
  xs.forEach((x, ix) => {
    if (ix > 0) rv.push(delim);
    rv.push(x);
  });
  return rv;
}

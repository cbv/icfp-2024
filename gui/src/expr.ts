import { encodeString } from './codec';

export type Binop =
  | '+'
  | '*'
  | '/'
  | '%'
  | '<'
  | '>'
  | '='
  | '|'
  | '&'
  | '.'
  | 'T'
  | 'D'
  | '$'
  ;

export type Unop =
  | '-'
  | '!'
  | '#'
  | '$'
  ;

export type Expr =
  | { t: 'binop', a: Expr, b: Expr, opr: Binop }
  | { t: 'unop', a: Expr, opr: Unop }
  | { t: 'cond', cond: Expr, tt: Expr, ff: Expr }
  | { t: 'lam', v: string, body: Expr }
  | { t: 'str', x: string }
  | { t: 'int', x: number }
  | { t: 'bool', x: boolean }
  | { t: 'var', v: string }
  ;

function base94encode(n: number): string {
  let rv = '';
  if (n < 0) throw Error(`unimplemented`);
  if (n == 0) return '!';
  while (n > 0) {
    rv = String.fromCharCode(33 + (n % 94)) + rv;
    n = Math.floor(n / 94)
  }
  return rv;
}

export function add(a: Expr, b: Expr): Expr { return { t: 'binop', a, b, opr: '+' }; }
export function app(a: Expr, b: Expr): Expr { return { t: 'binop', a, b, opr: '$' }; }
export function litnum(x: number): Expr { return { t: 'int', x }; }
export function litstr(x: string): Expr { return { t: 'str', x }; }
export function litbool(x: boolean): Expr { return { t: 'bool', x }; }
export function lam(v: string, body: Expr): Expr { return { t: 'lam', v, body }; }
export function vuse(v: string): Expr { return { t: 'var', v }; }

export function expToToks(e: Expr): string[] {
  switch (e.t) {
    case 'binop': return [`B${e.opr}`, ...expToToks(e.a), ...expToToks(e.b)];
    case 'unop': return [`U${e.opr}`, ...expToToks(e.a)];
    case 'cond': return [`?`, ...expToToks(e.cond), ...expToToks(e.tt), ...expToToks(e.ff)];
    case 'lam': return [`L${e.v}`, ...expToToks(e.body)];
    case 'var': return [`v${e.v}`];
    case 'str': return [`S${encodeString(e.x)}`];
    case 'bool': return [e.x ? 'T' : 'F'];
    case 'int': return [base94encode(e.x)];
  }
}

export function expToIcfp(e: Expr): string {
  return expToToks(e).join(' ');
}

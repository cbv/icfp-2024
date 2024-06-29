import { decodeIntLit, decodeString } from './codec';
import { Binop, Expr, Unop } from './expr';

function getExprFromStream(toks: string[], pos: number): [Expr, number] {
  if (pos >= toks.length) {
    throw new Error(`Ran off end of token stream trying to parse`);
  }
  const tok = toks[pos];
  switch (tok[0]) {
    case 'T': return [{ t: 'bool', x: true }, pos + 1];
    case 'F': return [{ t: 'bool', x: false }, pos + 1];
    case '?': {
      const [cond, afterCond] = getExprFromStream(toks, pos + 1);
      const [tt, afterFf] = getExprFromStream(toks, afterCond);
      const [ff, afterTt] = getExprFromStream(toks, afterFf);
      return [{ t: 'cond', cond, tt, ff }, afterTt];
    } case 'B': {
      const opr = tok.substring(1) as Binop;
      const [a, afterA] = getExprFromStream(toks, pos + 1);
      const [b, afterB] = getExprFromStream(toks, afterA);
      return [{ t: 'binop', a, b, opr }, afterB];
    }
    case 'U': {
      const opr = tok.substring(1) as Unop;
      const [a, afterA] = getExprFromStream(toks, pos + 1);
      return [{ t: 'unop', a, opr }, afterA];
    }
    case 'L': {
      const v = tok.substring(1);
      const [body, afterBody] = getExprFromStream(toks, pos + 1);
      return [{ t: 'lam', v, body }, afterBody];
    }
    case 'v': {
      const v = tok.substring(1);
      return [{ t: 'var', v }, pos + 1];
    }
    case 'S': {
      const x = decodeString(tok.substring(1));
      return [{ t: 'str', x }, pos + 1];
    }
    case 'I': {
      const x = decodeIntLit(tok.substring(1));
      return [{ t: 'int', x }, pos + 1];
    }
    default: throw new Error(`Don't understand indicator '${tok[0]}'`);
  }
}

export function parseIcfp(icfp: string): Expr {
  const toks = icfp.split(/\s+/);
  const [expr, newPos] = getExprFromStream(toks, 0);
  return expr;
}

export function unparseSexp(e: Expr): string {
  function prettyVar(x: string) {
    const nicevars = 'xyzwabcd'.split('');
    const n = Number(decodeIntLit(x));
    if (n < nicevars.length) return nicevars[n];
    return x;
  }
  switch (e.t) {
    case 'binop': {
      if (e.opr == '$') {
        return `(${unparseSexp(e.a)} ${unparseSexp(e.b)})`;
      }
      else {
        return `(${e.opr} ${unparseSexp(e.a)} ${unparseSexp(e.b)})`;
      }
    }
    case 'unop': return `(${e.opr} ${unparseSexp(e.a)})`;
    case 'lam': return `(λ ${prettyVar(e.v)} ${unparseSexp(e.body)})`;
    case 'cond': return `(if ${unparseSexp(e.cond)} ${unparseSexp(e.tt)} ${unparseSexp(e.ff)})`;
    case 'str': const escaped = e.x.replace(/(["\\])/g, x => "\\" + x[1]);
      return `"${escaped}"`;
    case 'var': return prettyVar(e.v);
    case 'int': return e.x + '';
    case 'bool': return e.x ? 'true' : 'false';
  }
}

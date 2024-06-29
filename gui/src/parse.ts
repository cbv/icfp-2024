import { decodeIntLit, decodeString } from './codec';
import { Binop, Expr, Unop } from './expr';

function getExprFromStream(toks: string[], pos: number): [Expr, number] {
  const tok = toks[pos];
  switch (tok[0]) {
    case 'T': return [{ t: 'bool', x: true }, pos + 1];
    case 'F': return [{ t: 'bool', x: false }, pos + 1];
    case 'I': {
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
  }
}

export function parseIcfp(icfp: string): Expr {
  const toks = icfp.split(/\s+/);

  const [expr, newPos] = getExprFromStream(toks, 0);
  return expr;
}

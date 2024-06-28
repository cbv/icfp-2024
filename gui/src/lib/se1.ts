export type SE1 = {
  scale: number,
  translate: number,
}

export function apply(a: SE1, x: number): number {
  return a.scale * x + a.translate;
}

export function compose(a: SE1, b: SE1): SE1 {
  return {
    scale: a.scale * b.scale,
    translate: a.scale * b.translate + a.translate,
  }
}

export function composen(...xforms: SE1[]): SE1 {
  return xforms.reduce(compose);
}

export function inverse(a: SE1): SE1 {
  return {
    scale: 1 / a.scale,
    translate: -a.translate / a.scale,
  }
}

export function ident(): SE1 {
  return {
    scale: 1,
    translate: 0,
  }
}


export function translate(p: number): SE1 {
  return {
    scale: 1,
    translate: p,
  }
}

export function scale(p: number): SE1 {
  return {
    scale: p,
    translate: 0,
  }
}

export function mkSE1(scale: number, translate: number): SE1 {
  return { scale, translate };
}

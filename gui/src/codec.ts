const spaceEncoding = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ \n";

export function decodeString(text: string): string {
  let rv: string = '';
  for (const x of text.split('')) {
    const ix = x.charCodeAt(0) - 33;
    if (ix < 0 || ix >= spaceEncoding.length) throw new Error('outside encoding');
    else
      rv += spaceEncoding[ix];
  }
  return rv;
}

export function encodeString(text: string): string {
  let rv: string = '';
  for (const x of text.split('')) {
    const ix = spaceEncoding.indexOf(x);
    if (ix == -1) throw new Error('outside encoding');
    else
      rv += String.fromCharCode(33 + ix);
  }
  return rv;
}

export function decodeIntLit(text: string): number {
  let rv: number = 0;
  for (const x of text.split('')) {
    const ix = x.charCodeAt(0) - 33;
    if (ix < 0 || ix >= 94) throw new Error('outside base-94 encoding');
    rv = 94 * rv + ix
  }
  return rv;
}

export function encodeIntLit(n: number): string {
  let rv = '';
  if (n < 0) throw Error(`unimplemented`);
  if (n == 0) return '!';
  while (n > 0) {
    rv = String.fromCharCode(33 + (n % 94)) + rv;
    n = Math.floor(n / 94)
  }
  return rv;
}

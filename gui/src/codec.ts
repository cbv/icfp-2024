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

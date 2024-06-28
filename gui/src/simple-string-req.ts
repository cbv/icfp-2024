import * as fs from 'fs';
import * as path from 'path';
import { decodeString, encodeString } from './codec';
const token = fs.readFileSync(path.join(__dirname, '../../API_KEY'), 'utf8').replace(/\n/g, '');

export class DecodeError extends Error {
  constructor(msg: string, public raw: string) {
    super(msg);
  }
}

export async function simpleStringReq(text: string): Promise<string> {
  const body = 'S' + encodeString(text);
  const preq = new Request("https://boundvariable.space/communicate", {
    method: 'POST',
    headers: { 'Authorization': `Bearer ${token}` },
    body
  });
  const presp = await fetch(preq);
  const ptext = await presp.text();
  if (ptext.match(/^S/)) {
    return decodeString(ptext.replace(/^S/, ''));
  }
  else {
    throw new DecodeError(`don't know how to handle ${ptext}`, ptext);
  }
}

// no encoding/decoding
export async function simpleStringReqRaw(text: string): Promise<string> {
  const body = text;
  const preq = new Request("https://boundvariable.space/communicate", {
    method: 'POST',
    headers: { 'Authorization': `Bearer ${token}` },
    body
  });
  const presp = await fetch(preq);
  const ptext = await presp.text();
  return ptext;
}

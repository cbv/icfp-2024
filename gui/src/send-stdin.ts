import { simpleStringReq } from './simple-string-req';
import * as fs from 'fs';

async function go() {
  const input = fs.readFileSync(0, 'utf8').replace(/\s+$/, '').replace(/^\s+/, '');
  console.log(await simpleStringReq(input));
}

go();

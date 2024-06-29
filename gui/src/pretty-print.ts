import { parseIcfp } from './parse';
import * as fs from 'fs';

const program = fs.readFileSync(0, 'utf8').replace(/\n$/, '');
console.log(JSON.stringify(parseIcfp(program), null, 2));

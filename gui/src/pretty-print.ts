import { parseIcfp, unparseSexp } from './parse';
import * as fs from 'fs';

const program = fs.readFileSync(0, 'utf8').replace(/\n$/, '');
console.log(unparseSexp(parseIcfp(program)));

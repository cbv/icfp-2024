import * as fs from 'fs';
import * as path from 'path';
import { DecodeError, simpleStringReq } from '../src/simple-string-req';

function delay(ms: number): Promise<void> {
  return new Promise((res, rej) => {
    setTimeout(() => { res(); }, ms);
  });
}

async function go() {
  fs.mkdirSync(path.join(__dirname, `../../puzzles/lambdaman`), { recursive: true });
  for (let i = 1; i <= 21; i++) {
    try {
      const lambdamanPuzzle = await simpleStringReq(`get lambdaman${i}`);
      const fpath = path.join(__dirname, `../../puzzles/lambdaman/lambdaman${i}.txt`);
      fs.writeFileSync(fpath, lambdamanPuzzle, 'utf8');
      console.log(`wrote: ${fpath}`);

    }
    catch (e) {
      if (e instanceof DecodeError) {
        const fpath = path.join(__dirname, `../../puzzles/lambdaman/lambdaman${i}.icfp`);
        fs.writeFileSync(fpath, e.raw, 'utf8');
        console.log(`wrote: ${fpath}`);
      }
      else {
        throw e;
      }
    }
    await delay(4000);
  }
}

go();

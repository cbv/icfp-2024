import * as fs from 'fs';
import * as path from 'path';
import { DecodeError, simpleStringReq } from './simple-string-req';

function delay(ms: number): Promise<void> {
  return new Promise((res, rej) => {
    setTimeout(() => { res(); }, ms);
  });
}

async function go() {
  fs.mkdirSync(path.join(__dirname, `../../puzzles`), { recursive: true });
  for (let i = 21; i <= 25; i++) {
    try {
      const spaceshipPuzzle = await simpleStringReq(`get spaceship${i}`);
      const fpath = path.join(__dirname, `../../puzzles/spaceship${i}.txt`);
      fs.writeFileSync(fpath, spaceshipPuzzle, 'utf8');
      console.log(`wrote: ${fpath}`);

    }
    catch (e) {
      if (e instanceof DecodeError) {
        const fpath = path.join(__dirname, `../../puzzles/spaceship${i}.icfp`);
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

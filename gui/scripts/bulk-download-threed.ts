import * as fs from 'fs';
import * as path from 'path';
import { DecodeError, simpleStringReq } from '../src/simple-string-req';

function delay(ms: number): Promise<void> {
  return new Promise((res, rej) => {
    setTimeout(() => { res(); }, ms);
  });
}

async function go() {
  fs.mkdirSync(path.join(__dirname, `../../puzzles/threed`), { recursive: true });
  for (let i = 1; i <= 12; i++) {
    try {
      const threedPuzzle = await simpleStringReq(`get 3d${i}`);
      const fpath = path.join(__dirname, `../../puzzles/threed/threed${i}.txt`);
      fs.writeFileSync(fpath, threedPuzzle, 'utf8');
      console.log(`wrote: ${fpath}`);

    }
    catch (e) {
      if (e instanceof DecodeError) {
        const fpath = path.join(__dirname, `../../puzzles/threed/threed${i}.icfp`);
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

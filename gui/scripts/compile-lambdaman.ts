import * as compile from '../src/compile';
import * as process from 'process';


function go(name: string) {
  switch (name) {
    case 'lambdaman4': return compile.lambdaman4();
    case 'lambdaman6': return compile.lambdaman6();
    case 'lambdaman8': return compile.lambdaman8();
    case 'lambdaman9': return compile.lambdaman9();
    default: throw new Error(`not found: ${name}`);
  }
}

if (process.argv.length != 3) {
  console.log("usage : ts-node compile-lambdaman.ts PROGRAM_NAME")
} else {
  let result = go(process.argv[2]);
  console.log(result);
}

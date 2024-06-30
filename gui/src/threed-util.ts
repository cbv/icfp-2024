import { ThreedProgram } from "./state";
import { EvalThreedResponse, ThreedItem } from "./types";

// This is able to strip the solve line
export function parseThreedProgram(puzzle: string): ThreedProgram {
  const stripped = puzzle.replace(/solve .*\n/, '');
  return stripped.split('\n').filter(x => x.length).map(line => line.split(/\s+/).filter(x => x.length));
}

// This doesn't include the solve line
export function unparseThreedProgram(prog: ThreedProgram): string {
  return prog.map(line => line.join(' ') + '\n').join('');
}

export function eraseComments(program: string): string {
  return program.replace(/[C-RT-Z]/g, '.');
}

export function framesOfTrace(trace: EvalThreedResponse): (ThreedItem & { t: 'frame' })[] {
  return trace.flatMap(x => x.t == 'frame' ? [x] : []);
}

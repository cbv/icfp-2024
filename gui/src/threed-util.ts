import { ThreedProgram } from "./state";

// This is able to strip the solve line
export function parseThreedProgram(puzzle: string): ThreedProgram {
  const stripped = puzzle.replace(/solve .*\n/, '');
  return stripped.split('\n').filter(x => x.length).map(line => line.split(/\s+/).filter(x => x.length));
}

// This doesn't include the solve line
export function unparseThreedProgram(prog: ThreedProgram): string {
  return prog.map(line => line.join(' ') + '\n').join('');
}

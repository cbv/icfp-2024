export type PuzzleSolution = { name: string, body: string };
export type AppProps = {
  threedSolutions: PuzzleSolution[],
};

export type EvalThreedRpc = { program: string, a: number, b: number };

export type ThreedItem =
  | { t: 'frame', frame: string }
  | { t: 'output', output: number, timetravel?: boolean }
  ;

export type EvalThreedResponse = ThreedItem[];

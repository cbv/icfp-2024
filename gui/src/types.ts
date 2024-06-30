export type PuzzleSolution = { name: string, body: string };
export type AppProps = {
  threedSolutions: PuzzleSolution[],
};

export type EvalThreedRpc = { program: string, a: number, b: number };

export type ThreedItem =
  | {
    t: 'frame',
    time: number, // this is the nonmonotonically evolving clock
    frame: string,
    min: [number, number], // min spatial extent in (x,y)
    max: [number, number], // max spatial extent in (x,y)
  }
  | { t: 'output', output: number, timetravel?: boolean }
  ;

export type EvalThreedResponse = ThreedItem[];

export type Point = { x: number, y: number };
export type Rect = { min: Point, max: Point };

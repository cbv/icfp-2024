import { Effect } from "./effect";
import { AppProps, EvalThreedResponse, Point, PuzzleSolution } from "./types";

// list of list of tokens
export type ThreedProgram = string[][];

export type AppModeState =
  | {
    t: 'evaluate',
    inputText: string,
    outputText: string,
  }
  | {
    t: 'codec',
    inputText: string,
    outputText: string,
  }
  | {
    t: 'communicate',
    inputText: string,
    outputText: string,
  }
  | {
    t: 'lambdaman',
    inputText: string,
    outputText: string,
  }
  | {
    t: 'threed',
    curPuzzleName: string | undefined,
    curProgram: ThreedProgram | undefined,
    executionTrace: EvalThreedResponse | undefined,
    currentFrame: number,
    a: string,
    b: string,
    hoverCell: Point | undefined,
  }
  ;


export type AppMode = AppModeState["t"];

export function mkModeState(mode: AppMode): AppModeState {
  switch (mode) {
    case 'evaluate': return { t: 'evaluate', inputText: '', outputText: '' };
    case 'codec': return { t: 'codec', inputText: '', outputText: '' };
    case 'communicate': return { t: 'communicate', inputText: '', outputText: '' };
    case 'lambdaman': return { t: 'lambdaman', inputText: '', outputText: '' };
    case 'threed': return {
      t: 'threed',
      curPuzzleName: undefined,
      executionTrace: undefined,
      currentFrame: 0,
      a: '1',
      b: '1',
      curProgram: undefined,
      hoverCell: undefined,
    };
  }
}
export type AppState = {
  effects: Effect[],
  mode: AppMode,
  modeState: AppModeState,
  threedSolutions: PuzzleSolution[],
}

export function mkState(props: AppProps, mode: AppMode | undefined): AppState {
  if (mode == undefined)
    mode = 'codec';
  const modeState = mkModeState(mode);
  return { threedSolutions: props.threedSolutions, effects: [], mode, modeState };
}

export function hashOfMode(mode: AppMode): string {
  return '#' + mode;
}

export function modeOfHash(hash: string): AppMode {
  return hash.substring(1) as AppMode;
}

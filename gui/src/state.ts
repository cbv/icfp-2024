import { Effect } from "./effect";
import { EvalThreedResponse } from "./types";

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
    executionTrace: EvalThreedResponse | undefined,
    currentFrame: number,
    a: string,
    b: string,
  }
  ;


export type AppMode = AppModeState["t"];

export function mkModeState(mode: AppMode): AppModeState {
  switch (mode) {
    case 'evaluate': return { t: 'evaluate', inputText: '', outputText: '' };
    case 'codec': return { t: 'codec', inputText: '', outputText: '' };
    case 'communicate': return { t: 'communicate', inputText: '', outputText: '' };
    case 'lambdaman': return { t: 'lambdaman', inputText: '', outputText: '' };
    case 'threed': return { t: 'threed', curPuzzleName: undefined, executionTrace: undefined, currentFrame: 0, a: '1', b: '1' };
  }
}
export type AppState = {
  effects: Effect[],
  mode: AppMode,
  modeState: AppModeState,
}

export function mkState(mode: AppMode | undefined): AppState {
  if (mode == undefined)
    mode = 'codec';
  const modeState = mkModeState(mode);
  return { effects: [], mode, modeState };
}

export function hashOfMode(mode: AppMode): string {
  return '#' + mode;
}

export function modeOfHash(hash: string): AppMode {
  return hash.substring(1) as AppMode;
}

import { Effect } from "./effect";

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
    curPuzzle: number | undefined,
  }
  ;


export type AppMode = AppModeState["t"];

export function mkModeState(mode: AppMode): AppModeState {
  switch (mode) {
    case 'evaluate': return { t: 'evaluate', inputText: '', outputText: '' };
    case 'codec': return { t: 'codec', inputText: '', outputText: '' };
    case 'communicate': return { t: 'communicate', inputText: '', outputText: '' };
    case 'lambdaman': return { t: 'lambdaman', inputText: '', outputText: '' };
    case 'threed': return { t: 'threed', curPuzzle: undefined };
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

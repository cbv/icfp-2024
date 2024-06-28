import { Effect } from "./effect";

export type AppMode =
  | 'evaluate'
  | 'codec'
  | 'communicate'
  ;

export function mkModeState(mode: AppMode): AppModeState {
  switch (mode) {
    case 'evaluate': return { t: 'evaluate', inputText: '', outputText: '' };
    case 'codec': return { t: 'codec', inputText: '', outputText: '' };
    case 'communicate': return { t: 'communicate', inputText: '', outputText: '' };
  }
}

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
  ;

export type AppState = {
  effects: Effect[],
  mode: AppMode,
  modeState: AppModeState,
}

export function mkState(): AppState {
  return { effects: [], mode: 'codec', modeState: { t: 'codec', inputText: '', outputText: '' } };
}

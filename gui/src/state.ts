import { Effect } from "./effect";

export type AppMode =
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
}

export function mkState(): AppState {
  return { effects: [], mode: { t: 'codec', inputText: '', outputText: '' } };
}

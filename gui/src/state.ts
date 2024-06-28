import { Effect } from "./effect";

export type AppState = {
  effects: Effect[],
  inputText: string,
  outputText: string,
}

export function mkState(): AppState {
  return { effects: [], inputText: '', outputText: '' };
}

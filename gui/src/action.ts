import { Point } from "./lib/types";

export type Action =
  | { t: 'setInputText', text: string }
  ;

export type Dispatch = (action: Action) => void;

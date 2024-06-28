import { Point } from "./lib/types";
import { AppMode } from "./state";

export type Action =
  | { t: 'setInputText', text: string }
  | { t: 'setOutputText', text: string }
  | { t: 'sendText', text: string }
  | { t: 'setMode', mode: AppMode }
  ;

export type Dispatch = (action: Action) => void;
import { Effect } from "./effect";
import { Point } from "./lib/types";
import { AppMode } from "./state";

export type Action =
  | { t: 'setInputText', text: string }
  | { t: 'setOutputText', text: string }
  | { t: 'doEffect', effect: Effect }
  | { t: 'setMode', mode: AppMode }
  | { t: 'setCurrentItem', item: string }
  | { t: 'setCurrentFrame', frame: number }
  | { t: 'compile' }
  ;

export type Dispatch = (action: Action) => void;

import { Effect } from "./effect";
import { Point } from "./lib/types";
import { AppMode } from "./state";

export type ThreedAction =
  | { t: 'setCurrentItem', item: string }
  | { t: 'setCurrentFrame', frame: number }
  | { t: 'setValue', which: 'a' | 'b', v: string }
  ;

export type Action =
  | { t: 'setInputText', text: string }
  | { t: 'setOutputText', text: string }
  | { t: 'doEffect', effect: Effect }
  | { t: 'setMode', mode: AppMode }
  | { t: 'threedAction', action: ThreedAction }
  | { t: 'compile' }
  ;

export type Dispatch = (action: Action) => void;

export function threedAction(action: ThreedAction): Action {
  return { t: 'threedAction', action };
}

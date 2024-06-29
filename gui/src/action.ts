import { Effect } from "./effect";
import { Point } from "./lib/types";
import { AppMode } from "./state";
import { EvalThreedResponse } from "./types";

export type ThreedAction =
  | { t: 'setCurrentItem', item: string }
  | { t: 'setCurrentFrame', frame: number }
  | { t: 'setExecutionTrace', trace: EvalThreedResponse | undefined }
  | { t: 'setValue', which: 'a' | 'b', v: string }
  | { t: 'setProgramCell', x: number, y: number, v: string }
  | { t: 'expandProgram' }
  | { t: 'incFrame', dframe: number }
  | { t: 'setHover', p: Point }
  | { t: 'clearHover', p: Point }
  | { t: 'editChar', char: string }
  | { t: 'mouseDown', x: number, y: number, buttons: number }
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

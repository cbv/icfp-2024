import { produce } from 'immer';
import { AppState, mkModeState } from './state';
import { Action } from './action';

export function reduce(state: AppState, action: Action): AppState {
  switch (action.t) {
    case 'setInputText': {
      return produce(state, s => {
        s.modeState.inputText = action.text;
      });
    }
    case 'setOutputText': {
      return produce(state, s => {
        s.modeState.outputText = action.text;
      });
    }
    case 'setMode': {
      return produce(state, s => {
        s.mode = action.mode;
        s.modeState = mkModeState(action.mode);
      });
    }
    case 'doEffect': {
      return produce(state, s => {
        s.effects.push(action.effect);
      });
    }
    case 'compile': {
      return produce(state, s => {
        s.modeState.outputText = 'foo';
      });
    }

  }
}

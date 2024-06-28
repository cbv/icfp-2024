import { produce } from 'immer';
import { AppState, mkModeState } from './state';
import { Action } from './action';
import { repeat } from './lib/util';

function compileExample() {
  let rv = '';
  for (let i = 1; i < 48; i++) {
    rv += repeat('D', 2 * i) + repeat('L', 2 * i);
    rv += repeat('U', 2 * i + 2) + repeat('R', 2 * i + 2);
  }
  rv += repeat('D', 98);
  rv += repeat('L', 97);
  return rv;
}

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
        s.modeState.outputText = compileExample();
      });
    }

  }
}

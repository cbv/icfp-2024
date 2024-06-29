import { produce } from 'immer';
import { AppModeState, AppState, mkModeState } from './state';
import { Action } from './action';
import { compileExample } from './compile';

function setInputText(state: AppState, text: string): AppState {
  if (!(state.modeState.t == 'evaluate' || state.modeState.t == 'codec' || state.modeState.t == 'communicate'
    || state.modeState.t == 'lambdaman')) {
    throw new Error(`can't set input text in this mode`);
  }

  const newModeState = produce(state.modeState, s => {
    s.inputText = text;
  });
  return produce(state, s => { s.modeState = newModeState; })
}

function setOutputText(state: AppState, text: string): AppState {
  if (!(state.modeState.t == 'evaluate' || state.modeState.t == 'codec' || state.modeState.t == 'communicate'
    || state.modeState.t == 'lambdaman')) {
    throw new Error(`can't set output text in this mode`);
  }

  const newModeState = produce(state.modeState, s => {
    s.outputText = text;
  });
  return produce(state, s => { s.modeState = newModeState; })
}

export function reduce(state: AppState, action: Action): AppState {
  switch (action.t) {
    case 'setInputText': {
      return setInputText(state, action.text);
    }
    case 'setOutputText': {
      return produce(state, s => {
        return setOutputText(state, action.text);
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
        return setOutputText(state, compileExample());
      });
    }

  }
}

import { produce } from 'immer';
import { AppModeState, AppState, hashOfMode, mkModeState } from './state';
import { Action } from './action';
import { compileExample } from './compile';
import { EvalThreedResponse } from './types';

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
  if (state.modeState.t == 'threed') {
    const executionTrace = JSON.parse(text) as EvalThreedResponse;
    const newModeState = produce(state.modeState, s => {
      s.executionTrace = executionTrace;
      s.currentFrame = 0;
    });
    return produce(state, s => { s.modeState = newModeState; })
  }

  if (!(state.modeState.t == 'evaluate' || state.modeState.t == 'codec' || state.modeState.t == 'communicate'
    || state.modeState.t == 'lambdaman')) {
    throw new Error(`can't set output text in this mode`);
  }

  const newModeState = produce(state.modeState, s => {
    s.outputText = text;
  });
  return produce(state, s => { s.modeState = newModeState; })
}

function setCurrentItem(state: AppState, item: string): AppState {
  if (!(state.modeState.t == 'threed')) {
    throw new Error(`can't set output text in this mode`);
  }
  const newModeState = produce(state.modeState, s => {
    s.curPuzzleName = item;
    s.executionTrace = undefined;
  });
  return produce(state, s => { s.modeState = newModeState; })
}

function setCurrentFrame(state: AppState, frame: number): AppState {
  if (!(state.modeState.t == 'threed')) {
    throw new Error(`can't set output text in this mode`);
  }
  const newModeState = produce(state.modeState, s => {
    s.currentFrame = frame;
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
        s.effects.push({ t: 'setHash', hash: hashOfMode(action.mode) });
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
    case 'setCurrentItem': {
      return produce(state, s => {
        return setCurrentItem(state, action.item);
      });
    }
    case 'setCurrentFrame': {
      return produce(state, s => {
        return setCurrentFrame(state, action.frame);
      });
    }
  }
}

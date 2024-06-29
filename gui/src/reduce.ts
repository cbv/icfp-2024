import { produce } from 'immer';
import { AppModeState, AppState, ThreedProgram, hashOfMode, mkModeState } from './state';
import { Action, ThreedAction } from './action';
import { compileExample } from './compile';
import { EvalThreedResponse } from './types';
import { framesOfTrace, parseThreedProgram } from './threed-util';

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

function produceMs<T extends AppModeState>(state: AppState, ms: T, f: (ms: T) => void): AppState {
  const newMs = produce(ms, f);
  return produce(state, s => { s.modeState = newMs; });
}

function reduceThreed(state: AppState, ms: AppModeState & { t: 'threed' }, action: ThreedAction): AppState {

  switch (action.t) {
    case 'setCurrentItem': {

      const puzzle = state.threedSolutions.find(p => p.name == action.item);
      let newProgram: ThreedProgram | undefined = undefined;
      if (puzzle != undefined) {
        newProgram = parseThreedProgram(puzzle.body);
      }
      return produceMs(state, ms, s => {
        s.curPuzzleName = action.item;
        s.curProgram = newProgram;
        s.executionTrace = undefined;
      });

    }
    case 'setCurrentFrame': {
      return produceMs(state, ms, s => {
        s.currentFrame = action.frame;
      });
    }
    case 'setValue': {
      return produceMs(state, ms, s => {
        s[action.which] = action.v;
      });
    }
    case 'setProgramCell': {
      return produceMs(state, ms, s => {
        if (s.curProgram != undefined)
          s.curProgram[action.y][action.x] = action.v;
      });
    }
    case 'setExecutionTrace': {
      return produceMs(state, ms, s => {
        s.executionTrace = action.trace;
      });
    }
    case 'expandProgram': {
      const prog = ms.curProgram;
      if (prog == undefined)
        return state;
      const blankLine: () => string[] = () => ['.', ...prog[0].map(x => '.'), '.'];
      const newProgram = [blankLine(), ...prog.map(line => ['.', ...line, '.']), blankLine()];
      return produceMs(state, ms, s => {
        s.curProgram = newProgram;
      });
    }
    case 'incFrame': {
      const trace = ms.executionTrace;
      if (trace == undefined)
        return state;
      const frame = ms.currentFrame;
      const minFrame = 0;
      const maxFrame = framesOfTrace(trace).length - 1;
      const newFrame = Math.min(maxFrame, Math.max(minFrame, ms.currentFrame + action.dframe));
      return produceMs(state, ms, s => {
        s.currentFrame = newFrame;
      });
    }
  }
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
    case 'threedAction': {
      if (state.modeState.t != 'threed') {
        throw new Error(`incompatible state`);
      }
      return reduceThreed(state, state.modeState, action.action);
    }
    case 'compile': {
      return produce(state, s => {
        return setOutputText(state, compileExample());
      });
    }
  }
}

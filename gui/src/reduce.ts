import { produce } from 'immer';
import { AppModeState, AppState, ThreedProgram, hashOfMode, mkModeState } from './state';
import { Action, ThreedAction } from './action';
import { compileExample } from './compile';
import { EvalThreedResponse, Point, Rect } from './types';
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

function keyFold(oldVal: string, keycode: string): string {
  if (keycode == ' ') return '.';
  if (keycode == 'a') return 'A';
  if (keycode == 'b') return 'B';
  if (keycode == 's') return 'S';
  if (keycode == '.') return '.';
  if (keycode.match(/[-<>^v+*/%@=#]/)) return keycode;
  // at this point keycode match [0-9] I think
  if (oldVal == '0') return keycode;
  if (oldVal == '.') return keycode;
  if (oldVal == '-' || oldVal.match(/-?[0-9]+/)) return oldVal + keycode;
  return keycode;
}

function spanRect(p1: Point, p2: Point): Rect {
  return {
    max: { x: Math.max(p1.x, p2.x), y: Math.max(p1.y, p2.y) },
    min: { x: Math.min(p1.x, p2.x), y: Math.min(p1.y, p2.y) },
  }
}

function getContents(prog: string[][], rect: Rect): string[][] {
  const rv: string[][] = [];
  for (let y = rect.min.y; y <= rect.max.y; y++) {
    const row: string[] = [];
    for (let x = rect.min.x; x <= rect.max.x; x++) {
      row.push(prog[y][x]);
    }
    rv.push(row);
  }
  return rv;
}


function eraseContents(prog: string[][], rect: Rect): string[][] {
  return produce(prog, pp => {
    for (let y = rect.min.y; y <= rect.max.y; y++) {
      for (let x = rect.min.x; x <= rect.max.x; x++) {
        pp[y][x] = '.';
      }
    }
  });
}

function putContents(prog: string[][], p: Point, src: string[][]): string[][] {
  const prog_xs = prog[0].length;
  const prog_ys = prog.length;
  return produce(prog, pp => {
    for (let y = 0; y < src.length; y++) {
      for (let x = 0; x < src[0].length; x++) {
        if (p.y + y < prog_ys && p.x + x < prog_xs)
          pp[p.y + y][p.x + x] = src[y][x];
      }
    }
  });
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
        s.selection = undefined;
        // but we leave clipboard intact!
        s.cutRect = undefined;
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
    case 'setHover': {
      if (ms.mouseState.t == 'drag') {
        const p1 = ms.mouseState.p;
        const p2 = action.p;
        const newSel = spanRect(p1, p2);
        return produceMs(state, ms, s => {
          s.selection = newSel;
        });
      }
      return produceMs(state, ms, s => {
        s.hoverCell = action.p;
      });
    }
    case 'clearHover': {
      return produceMs(state, ms, s => {
        s.hoverCell = undefined;
      });
    }
    case 'editChar': {
      if (ms.hoverCell == undefined)
        return state;
      if (ms.curProgram == undefined)
        return state;
      const { x, y } = ms.hoverCell;
      const oldVal = ms.curProgram[y][x];
      const newVal = keyFold(oldVal, action.char);
      const newProgram = produce(ms.curProgram, s => {
        s[y][x] = newVal;
      });
      return produceMs(state, ms, s => {
        s.curProgram = newProgram;
      });
    }
    case 'mouseDown': {
      if (action.buttons == 2) {
        return produceMs(state, ms, s => {
          if (s.curProgram != undefined)
            s.curProgram[action.y][action.x] = '.';
        });
      }
      if (action.buttons == 1) {
        const p = { x: action.x, y: action.y };
        return produceMs(state, ms, s => {
          s.mouseState = { t: 'drag', p };
          s.selection = { min: p, max: p };
        });

      }
      return state;
    }
    case 'mouseUp': {
      return produceMs(state, ms, s => {
        s.mouseState = { t: 'up' };
      });
    }
    case 'clearSelection': {
      return produceMs(state, ms, s => {
        s.selection = undefined;
      });
    }
    case 'cut': {
      const selection = ms.selection;
      if (selection == undefined)
        return state;
      const program = ms.curProgram;
      if (program == undefined)
        return state;

      const clipboard = getContents(program, selection);
      return produceMs(state, ms, s => {
        s.cutRect = selection;
        s.clipboard = clipboard;
      });
    }
    case 'copy': {
      const selection = ms.selection;
      if (selection == undefined)
        return state;
      const program = ms.curProgram;
      if (program == undefined)
        return state;

      const clipboard = getContents(program, selection);
      return produceMs(state, ms, s => {
        s.cutRect = undefined;
        s.clipboard = clipboard;
      });
    }
    case 'paste': {
      const selection = ms.selection;
      if (selection == undefined)
        return state;
      const program = ms.curProgram;
      if (program == undefined)
        return state;
      const clipboard = ms.clipboard;
      if (clipboard == undefined)
        return state;
      const cutRect = ms.cutRect;

      let newProgram = program;
      if (cutRect != undefined) {
        newProgram = eraseContents(newProgram, cutRect);
      }
      newProgram = putContents(newProgram, selection.min, clipboard);
      return produceMs(state, ms, s => {
        s.curProgram = newProgram;
        s.selection = undefined;
        if (cutRect != undefined) {
          s.clipboard = undefined;
          s.cutRect = undefined;
        }
      });
    }
    case 'clear': {
      const selection = ms.selection;
      if (selection == undefined)
        return state;
      const program = ms.curProgram;
      if (program == undefined)
        return state;
      let newProgram = program;
      newProgram = eraseContents(newProgram, selection);
      return produceMs(state, ms, s => {
        s.curProgram = newProgram;
        s.selection = undefined;
      });
    }
    case 'crop': {
      const selection = ms.selection;
      if (selection == undefined)
        return state;
      const program = ms.curProgram;
      if (program == undefined)
        return state;
      let newProgram = program;
      newProgram = getContents(newProgram, selection);
      return produceMs(state, ms, s => {
        s.curProgram = newProgram;
        s.selection = undefined;
      });
    }
    case 'recover': {
      if (localStorage['recover']) {
        const program: string[][] = JSON.parse(localStorage['recover']);
        return produceMs(state, ms, s => {
          s.curProgram = program;
          s.selection = undefined;
        });
      }
      else {
        return state;
      }
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

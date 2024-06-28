import { produce } from 'immer';
import { AppState } from './state';
import { Action } from './action';

export function reduce(state: AppState, action: Action): AppState {
  switch (action.t) {
    case 'setInputText': {
      return produce(state, s => {
        s.inputText = action.text;
      });
    }
  }
}

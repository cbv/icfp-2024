import * as React from 'react';
import { createRoot } from 'react-dom/client';
import { doEffect } from './effect';
import { extractEffects } from './lib/extract-effects';
import { useEffectfulReducer } from './lib/use-effectful-reducer';
import { reduce } from './reduce';
import { AppMode, AppModeState, AppState, mkState } from './state';
import { Dispatch } from './action';
import { toTitleCase } from './lib/util';
import { decodeString, encodeString } from './codec';

export type AppProps = {
};

const modes: AppMode[] = ['codec', 'communicate', 'evaluate'];

export function Navbar(props: { state: AppState, dispatch: Dispatch }): JSX.Element {
  const { state, dispatch } = props;
  const modeButtons = modes.map(mode =>
    <a className={state.mode == mode ? 'active' : undefined} onMouseDown={() => { dispatch({ t: 'setMode', mode }) }}>{toTitleCase(mode)}</a>
  );
  return <div className="topnav">
    <div className="header">ICFP 2024</div>
    <div className="spacer"></div>
    {modeButtons}
    <div className="spacer"></div>
  </div>

}

function renderEvaluate(state: AppState, modeState: AppModeState & { t: 'evaluate' }, dispatch: Dispatch): JSX.Element {
  const onInput: React.FormEventHandler<HTMLTextAreaElement> = (e) => { dispatch({ t: 'setInputText', text: e.currentTarget.value }); };
  return <div className="interface-container">
    <div className="textarea-container">
      <div className="textarea-panel">
        <div className="title">Input</div>
        <textarea spellCheck={false} value={state.modeState.inputText} onInput={onInput}></textarea>
      </div>
      <div className="textarea-panel">
        <div className="title">Output</div>
        <textarea spellCheck={false} value={state.modeState.outputText}></textarea>
      </div>
    </div>
    <div className="action-bar">
      <button>Evaluate</button>
    </div>
  </div>;
}

function renderCodec(state: AppState, modeState: AppModeState & { t: 'codec' }, dispatch: Dispatch): JSX.Element {
  const onInput: React.FormEventHandler<HTMLTextAreaElement> = (e) => { dispatch({ t: 'setInputText', text: e.currentTarget.value }); };
  return <div className="interface-container">
    <div className="textarea-container">
      <div className="textarea-panel">
        <div className="title">Input</div>
        <textarea spellCheck={false} value={modeState.inputText} onInput={onInput}></textarea>
      </div>
      <div className="textarea-panel">
        <div className="title">Output</div>
        <textarea spellCheck={false} value={modeState.outputText}></textarea>
      </div>
    </div>
    <div className="action-bar">
      <button onClick={(e) => { dispatch({ t: 'setOutputText', text: encodeString(modeState.inputText) }) }}>Encode</button>
      <button onClick={(e) => { dispatch({ t: 'setOutputText', text: decodeString(modeState.inputText) }) }}>Decode</button>
    </div>
  </div>;
}

function renderCommunicate(state: AppState, modeState: AppModeState & { t: 'communicate' }, dispatch: Dispatch): JSX.Element {
  const onInput: React.FormEventHandler<HTMLTextAreaElement> = (e) => { dispatch({ t: 'setInputText', text: e.currentTarget.value }); };
  return <div className="interface-container">
    <div className="textarea-container">
      <div className="textarea-panel">
        <div className="title">Input</div>
        <textarea spellCheck={false} value={state.modeState.inputText} onInput={onInput}></textarea>
      </div>
      <div className="textarea-panel">
        <div className="title">Output</div>
        <textarea spellCheck={false} value={state.modeState.outputText}></textarea>
      </div>
    </div>
    <div className="action-bar">
      <button onClick={(e) => { dispatch({ t: 'sendText', text: state.modeState.inputText }) }}>Send</button>
    </div>
  </div>;
}

function renderAppBody(state: AppState, dispatch: Dispatch): JSX.Element {
  const onInput: React.FormEventHandler<HTMLTextAreaElement> = (e) => { dispatch({ t: 'setInputText', text: e.currentTarget.value }); };
  switch (state.mode) {
    case 'evaluate':
      if (state.modeState.t != state.mode) throw new Error(`mode state inconsistency`);
      return renderEvaluate(state, state.modeState, dispatch);
    case 'codec':
      if (state.modeState.t != state.mode) throw new Error(`mode state inconsistency`);
      return renderCodec(state, state.modeState, dispatch);
    case 'communicate':
      if (state.modeState.t != state.mode) throw new Error(`mode state inconsistency`);
      return renderCommunicate(state, state.modeState, dispatch);
  }
}

export function App(props: AppProps): JSX.Element {
  const [state, dispatch] = useEffectfulReducer(mkState(), extractEffects(reduce), doEffect);



  const appBody = renderAppBody(state, dispatch);
  return <>
    <Navbar state={state} dispatch={dispatch} />
    {appBody}
  </>;
}

export function init() {
  const props: AppProps = {
    color: '#f0f',
  };
  const root = createRoot(document.querySelector('.app')!);
  root.render(<App {...props} />);
}

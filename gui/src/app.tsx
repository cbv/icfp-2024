import * as React from 'react';
import { createRoot } from 'react-dom/client';
import { doEffect } from './effect';
import { extractEffects } from './lib/extract-effects';
import { useEffectfulReducer } from './lib/use-effectful-reducer';
import { reduce } from './reduce';
import { AppState, mkState } from './state';
import { Dispatch } from './action';

export type AppProps = {
};

export function Navbar(props: { state: AppState, dispatch: Dispatch }): JSX.Element {
  const { state, dispatch } = props;
  return <div className="topnav">
    <div className="header">ICFP 2024</div>
    <div className="spacer"></div>
    <a className={state.mode.t == 'codec' ? 'active' : undefined} onMouseDown={() => { dispatch({ t: 'setMode', mode: { t: 'codec', inputText: '', outputText: '' } }) }}>Codec</a>
    <a className={state.mode.t == 'evaluate' ? 'active' : undefined} onMouseDown={() => { dispatch({ t: 'setMode', mode: { t: 'evaluate', inputText: '', outputText: '' } }) }}>Evaluate</a>
    <div className="spacer"></div>
  </div>

}

function renderAppBody(state: AppState, dispatch: Dispatch): JSX.Element {

  const onInput: React.FormEventHandler<HTMLTextAreaElement> = (e) => { dispatch({ t: 'setInputText', text: e.currentTarget.value }); };

  switch (state.mode.t) {
    case 'evaluate': return <div className="interface-container">
      <div className="textarea-container">
        <div className="textarea-panel">
          <div className="title">Input</div>
          <textarea spellCheck={false} value={state.mode.inputText} onInput={onInput}></textarea>
        </div>
        <div className="textarea-panel">
          <div className="title">Output</div>
          <textarea spellCheck={false} value={state.mode.outputText}></textarea>
        </div>
      </div>
      <div className="action-bar">
        <button>Evaluate</button>
      </div>
    </div>;
    case 'codec': return <div className="interface-container">
      <div className="textarea-container">
        <div className="textarea-panel">
          <div className="title">Input</div>
          <textarea spellCheck={false} value={state.mode.inputText} onInput={onInput}></textarea>
        </div>
        <div className="textarea-panel">
          <div className="title">Output</div>
          <textarea spellCheck={false} value={state.mode.outputText}></textarea>
        </div>
      </div>
      <div className="action-bar">
        <button>Encode</button>
        <button>Decode</button>
      </div>
    </div>;
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

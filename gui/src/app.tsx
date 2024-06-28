import * as React from 'react';
import { createRoot } from 'react-dom/client';
import { doEffect } from './effect';
import { extractEffects } from './lib/extract-effects';
import { useEffectfulReducer } from './lib/use-effectful-reducer';
import { reduce } from './reduce';
import { mkState } from './state';

export type AppProps = {
};

export function App(props: AppProps): JSX.Element {
  const [state, dispatch] = useEffectfulReducer(mkState(), extractEffects(reduce), doEffect);

  const onInput: React.FormEventHandler<HTMLTextAreaElement> = (e) => { dispatch({ t: 'setInputText', text: e.currentTarget.value }); };

  return <>
    <div className="topnav">
      <div className="header">ICFP 2024</div>
      <div className="spacer"></div>
      <a className="active" href="index.html">Evaluator</a>
      <div className="spacer"></div>
    </div>

    <div className="interface-container">
      <div className="textarea-container">
        <div className="textarea-panel">
          <div className="title">Input</div>
          <textarea spellCheck={false} value={state.inputText} onInput={onInput}></textarea>
        </div>
        <div className="textarea-panel">
          <div className="title">Output</div>
          <textarea spellCheck={false} value={state.outputText}></textarea>
        </div>
      </div>
      <div className="action-bar">
        <button>Evaluate</button>
      </div>
    </div >
  </>;
}

export function init() {
  const props: AppProps = {
    color: '#f0f',
  };
  const root = createRoot(document.querySelector('.app')!);
  root.render(<App {...props} />);
}

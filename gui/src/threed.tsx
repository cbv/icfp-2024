import * as React from 'react';
import { Dispatch, ThreedAction, threedAction } from './action';
import { AppModeState, AppState } from './state';
import { PuzzleSolution } from './types';
import { unparseThreedProgram } from './threed-util';

function arrow(rotate: number): JSX.Element {
  const transform = `
  translate(100 100)
  rotate(${90 * rotate})
  scale(0.7 1)
  translate(-100 -100)
  translate(50 50)`;
  return <svg viewBox="0 0 200 200">
    <g
      transform={transform}>
      <path d="m0,50l50,-50l50,50l-25,0l0,50l-50,0l0,-50l-25,0z" fill="#000000" />
    </g>
  </svg>
};

type LocalDispatch = (action: ThreedAction) => void;

export function renderRow(row: string[], rowIndex: number, dispatch: LocalDispatch): JSX.Element {
  function divwrap(x: JSX.Element | string, cname: string = "threed-cell"): JSX.Element {
    return <div className={cname}>{x}</div>;
  }
  function renderCell(x: string) {
    switch (x) {
      case '.': return divwrap('', "threed-cell-empty");
      case 'v': return divwrap(arrow(2));
      case '^': return divwrap(arrow(0));
      case '>': return divwrap(arrow(1));
      case '<': return divwrap(arrow(3));
      case 'S': return divwrap(<div className="S-chip">S</div>);
      case 'A': return divwrap(<div className="A-chip">A</div>);
      case 'B': return divwrap(<div className="B-chip">B</div>);
      default: return divwrap(x);
    }
  }
  return <tr>{row.map((rowData, x) => <td onMouseDown={() => { dispatch({ t: 'setProgramCell', x, y: rowIndex, v: '.' }) }}>{
    renderCell(rowData)}</td>)
  }</tr>;
}

type Point = { x: number, y: number };
type Rect = { min: Point, max: Point };

export function renderThreedPuzzleArray(array: string[][], dispatch: LocalDispatch): JSX.Element {
  return <table><tbody>{array.map((row, y) => renderRow(row, y, dispatch))}</tbody></table>;
}

export function renderThreedPuzzle(text: string, dispatch: LocalDispatch): JSX.Element {
  const lines = text.split('\n').filter(x => x.length).map(line => line.split(/\s+/).filter(x => x.length));;
  return renderThreedPuzzleArray(lines, dispatch);
}

export function renderThreedPuzzleInRect(text: string, globalRect: Rect, localRect: Rect, dispatch: LocalDispatch): JSX.Element {
  const localArray = text.split('\n').filter(x => x.length).map(line => line.split(/\s+/).filter(x => x.length));

  const globalArray: string[][] = [];
  for (let y = 0; y < globalRect.max.y - globalRect.min.y + 1; y++) {
    const row: string[] = [];
    globalArray.push(row);
    for (let x = 0; x < globalRect.max.x - globalRect.min.x + 1; x++) {
      row.push('.');
    }
  }

  for (let y = 0; y < localRect.max.y - localRect.min.y + 1; y++) {
    for (let x = 0; x < localRect.max.x - localRect.min.x + 1; x++) {
      globalArray[y + localRect.min.y - globalRect.min.y][x + localRect.min.x - globalRect.min.x] = localArray[y][x];
    }
  }

  return renderThreedPuzzleArray(globalArray, dispatch);
}

// return true if we've handled the key
function handleKey(state: AppState, modeState: AppModeState & { t: 'threed' }, dispatch: LocalDispatch, code: string): boolean {
  switch (code) {
    case 'ArrowLeft': dispatch({ t: 'incFrame', dframe: -1 }); return true;
    case 'ArrowRight': dispatch({ t: 'incFrame', dframe: 1 }); return true;
    default: console.log(code); return false;
  }
}

export function RenderThreed(props: { state: AppState, modeState: AppModeState & { t: 'threed' }, dispatch: Dispatch }): JSX.Element {
  const { state, modeState, dispatch } = props;
  const onKeyDown = (e: KeyboardEvent) => {
    if (handleKey(state, modeState, ldis, e.code)) {
      e.preventDefault();
      e.stopPropagation();
    }
  };
  React.useEffect(() => {
    window.addEventListener('keydown', onKeyDown);
    return () => {
      window.removeEventListener('keydown', onKeyDown);
    };
  }, [state]);
  function ldis(action: ThreedAction): void {
    dispatch(threedAction(action));
  }

  const onInput: React.FormEventHandler<HTMLTextAreaElement> = (e) => { dispatch({ t: 'setInputText', text: e.currentTarget.value }); };
  const renderedPuzzleItems = state.threedSolutions.map(puzzle => {
    const klass: string[] = ["puzzle-item"];
    if (puzzle.name == modeState.curPuzzleName) {
      klass.push('puzzle-item-selected');
    }
    return <div className={klass.join(" ")} onMouseDown={(e) => { ldis({ t: 'setCurrentItem', item: puzzle.name }) }}>{puzzle.name}</div>;
  });
  let renderedPuzzle: JSX.Element | undefined;

  let program = modeState.curProgram;
  const trace = modeState.executionTrace;

  if (trace != undefined) {
    const frames = trace.flatMap(x => x.t == 'frame' ? [x] : []);
    const globalRect: Rect = {
      min: { x: Math.min(...frames.map(frame => frame.min[0])), y: Math.min(...frames.map(frame => frame.min[1])) },
      max: { x: Math.max(...frames.map(frame => frame.max[0])), y: Math.max(...frames.map(frame => frame.max[1])) },
    };
    const frame = frames[modeState.currentFrame];
    const localRect: Rect = {
      min: { x: frame.min[0], y: frame.min[1] },
      max: { x: frame.max[0], y: frame.max[1] },
    };
    renderedPuzzle = <div className="vert-stack"><div className="rendered-puzzle">{renderThreedPuzzleInRect(frame.frame, globalRect, localRect, ldis)}</div>
      <input style={{ width: '40em' }} type="range" min={0} max={frames.length - 1} value={modeState.currentFrame} onInput={(e) => {
        ldis({ t: 'setCurrentFrame', frame: parseInt(e.currentTarget.value) })
      }}></input></div>;
  }
  else if (program != undefined) {
    renderedPuzzle = <div className="rendered-puzzle">{renderThreedPuzzleArray(program, ldis)}</div>;
  }

  const runProgram: React.MouseEventHandler = (e) => {
    let a = parseInt(modeState.a);
    if (isNaN(a)) { alert(`bad value for a: ${modeState.a}`); return }
    let b = parseInt(modeState.b);
    if (isNaN(b)) { alert(`bad value for b: ${modeState.b}`); return }
    dispatch({ t: 'doEffect', effect: { t: 'evalThreed', text: unparseThreedProgram(program!), a, b } })
  };

  function runApparatus(): JSX.Element {
    if (modeState.executionTrace != undefined) {
      return <>
        <button onClick={() => { ldis({ t: 'setExecutionTrace', trace: undefined }) }}>Edit</button>
      </>;
    }
    function onChange(which: 'a' | 'b') {
      return (e: React.ChangeEvent<HTMLInputElement>) => {
        ldis({ t: 'setValue', which, v: e.currentTarget.value })
      }
    }
    return <>
      <button onClick={() => ldis({ t: 'expandProgram' })}>Expand</button>
      <b style={{ color: 'white' }}>A</b><input onChange={onChange('a')} className="entry" value={modeState.a} size={2}></input>
      <b style={{ color: 'white' }}>B</b><input onChange={onChange('b')} className="entry" value={modeState.b} size={2}></input>
      <button onClick={runProgram}>Run</button>
    </>;
  }

  return <div className="interface-container">
    <div className="textarea-container">
      <div className="threed-container">
        <div className="threed-puzzlist">
          {renderedPuzzleItems}
        </div>
        {renderedPuzzle}
      </div>
    </div>
    <div className="action-bar">
      {program == undefined ? undefined : runApparatus()}
    </div>
  </div>;
}

import * as React from 'react';
import { Dispatch, threedAction } from './action';
import { AppModeState, AppState } from './state';
import { PuzzleSolution } from './types';

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

export function renderRow(row: string[]): JSX.Element {
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
  return <tr>{row.map(x => <td>{renderCell(x)}</td>)}</tr>;
}

type Point = { x: number, y: number };
type Rect = { min: Point, max: Point };

export function renderThreedPuzzleArray(array: string[][]): JSX.Element {
  return <table><tbody>{array.map(renderRow)}</tbody></table>;
}

export function renderThreedPuzzle(text: string): JSX.Element {
  const lines = text.split('\n').filter(x => x.length).map(line => line.split(/\s+/).filter(x => x.length));;
  return renderThreedPuzzleArray(lines);
}

export function renderThreedPuzzleInRect(text: string, globalRect: Rect, localRect: Rect): JSX.Element {
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

  return renderThreedPuzzleArray(globalArray);
}

export function renderThreed(state: AppState, modeState: AppModeState & { t: 'threed' }, dispatch: Dispatch): JSX.Element {
  const onInput: React.FormEventHandler<HTMLTextAreaElement> = (e) => { dispatch({ t: 'setInputText', text: e.currentTarget.value }); };
  const renderedPuzzleItems = state.threedSolutions.map(puzzle => {
    const klass: string[] = ["puzzle-item"];
    if (puzzle.name == modeState.curPuzzleName) {
      klass.push('puzzle-item-selected');
    }
    return <div className={klass.join(" ")} onMouseDown={(e) => { dispatch(threedAction({ t: 'setCurrentItem', item: puzzle.name })) }}>{puzzle.name}</div>;
  });
  let renderedPuzzle: JSX.Element | undefined;
  // Program is the current program text in case we need to send it to the evaluator
  let program: string | undefined;
  const trace = modeState.executionTrace;

  if (modeState.curPuzzleName != undefined) {
    const puzzle = state.threedSolutions.find(p => p.name == modeState.curPuzzleName);
    if (puzzle != undefined) {
      const stripped = puzzle.body.replace(/solve .*\n/, '');
      program = stripped;
    }
  }

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
    renderedPuzzle = <div className="vert-stack"><div className="rendered-puzzle">{renderThreedPuzzleInRect(frame.frame, globalRect, localRect)}</div>
      <input style={{ width: '40em' }} type="range" min={0} max={frames.length - 1} value={modeState.currentFrame} onInput={(e) => {
        dispatch(threedAction({ t: 'setCurrentFrame', frame: parseInt(e.currentTarget.value) }))
      }}></input></div>;
  }
  else if (program != undefined) {
    renderedPuzzle = <div className="rendered-puzzle">{renderThreedPuzzle(program)}</div>;
  }

  const runProgram: React.MouseEventHandler = (e) => {
    let a = parseInt(modeState.a);
    if (isNaN(a)) { alert(`bad value for a: ${modeState.a}`); return }
    let b = parseInt(modeState.b);
    if (isNaN(b)) { alert(`bad value for b: ${modeState.b}`); return }
    dispatch({ t: 'doEffect', effect: { t: 'evalThreed', text: program!, a, b } })
  };

  function runApparatus(): JSX.Element {
    function onChange(which: 'a' | 'b') {
      return (e: React.ChangeEvent<HTMLInputElement>) => {
        dispatch(threedAction({ t: 'setValue', which, v: e.currentTarget.value }))
      }
    }
    return <>
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

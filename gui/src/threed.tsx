import * as React from 'react';
import { Dispatch, ThreedAction, threedAction } from './action';
import { AppModeState, AppState } from './state';
import { Point, PuzzleSolution, Rect } from './types';
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
type LocalModeState = AppModeState & { t: 'threed' };

function pointInRect(p: Point, rect: Rect): boolean {
  return (p.x >= rect.min.x && p.y >= rect.min.y && p.x <= rect.max.x && p.y <= rect.max.y);
}

function isHighlighted(ms: LocalModeState, p: Point): boolean {
  if (ms.hoverCell != undefined && p.x == ms.hoverCell.x && p.y == ms.hoverCell.y) return true;
  if (ms.selection != undefined && pointInRect(p, ms.selection)) return true;
  return false;
}

export function renderRow(modeState: LocalModeState, row: string[], rowIndex: number, dispatch: LocalDispatch): JSX.Element {
  const y = rowIndex;

  function renderCell(tok: string, x: number) {

    function divwrap(elt: JSX.Element | string, cname: string = "threed-cell"): JSX.Element {
      if (isHighlighted(modeState, { x, y })) {
        cname = "threed-cell-hilite";
      }
      if (typeof elt == 'string' && elt.length > 4) {
        elt = <abbr title={elt} style={{ fontSize: '0.5em' }}>{elt}</abbr>;
      }
      if (typeof elt == 'string' && elt.length > 3) {
        elt = <span style={{ fontSize: '0.5em' }}>{elt}</span>;
      }
      else if (typeof elt == 'string' && elt.length > 2) {
        elt = <span style={{ fontSize: '0.75em' }}>{elt}</span>;
      }

      return <div className={cname}>{elt}</div>;
    }

    function renderAt() {
      const program = modeState.curProgram;
      if (program == undefined) {
        return divwrap('@');
      }
      const sx = program[y][x - 1];
      const sy = program[y][x + 1];
      if (sx == '.' || sy == '.') { return divwrap('@'); }
      const dx = x - parseInt(sx);
      const dy = y - parseInt(sy);
      if (isNaN(dx) || isNaN(dy)) { return divwrap('@'); }
      const canvas_sx = 34 * x + 1 + 16;
      const canvas_sy = 34 * y + 1 + 16;

      const canvas_dx = 34 * dx + 1 + 16;
      const canvas_dy = 34 * dy + 1 + 16;
      return <>
        {divwrap('@')}
        <svg style={{ pointerEvents: "none", position: "absolute", top: 0, left: 0, width: "100%", height: "100%" }}>
          <line x1={canvas_sx} y1={canvas_sy} x2={canvas_dx} y2={canvas_dy}
            stroke="rgba(0,124,240,0.2)" strokeLinecap="round"
            fill="none" stroke-width="15" />
        </svg>
      </>;
    }

    switch (tok) {
      case '@': return renderAt();
      case '.': return divwrap('', "threed-cell-empty");
      case 'v': return divwrap(arrow(2));
      case '^': return divwrap(arrow(0));
      case '>': return divwrap(arrow(1));
      case '<': return divwrap(arrow(3));
      case 'S': return divwrap(<div className="S-chip">S</div>);
      case 'A': return divwrap(<div className="A-chip">A</div>);
      case 'B': return divwrap(<div className="B-chip">B</div>);
      default: if (tok.match(/[A-Z]/)) {
        return divwrap(<div className="other-chip">{tok}</div>);
      }
      else {
        return divwrap(tok);
      }
    }
  }
  return <tr>{row.map((tok, x) => <td
    onMouseEnter={() => dispatch({ t: 'setHover', p: { x, y } })}
    onMouseLeave={() => dispatch({ t: 'clearHover', p: { x, y } })}
    onContextMenu={(e) => { e.preventDefault(); e.stopPropagation(); }}
    onMouseDown={(e) => { dispatch({ t: 'mouseDown', x, y, buttons: e.buttons }); e.preventDefault(); e.stopPropagation(); }}>{

      renderCell(tok, x)}</td>)
  }</tr>;
}

export function renderThreedPuzzleArray(ms: LocalModeState, array: string[][], dispatch: LocalDispatch): JSX.Element {
  return <table><tbody>{array.map((row, y) => renderRow(ms, row, y, dispatch))}</tbody></table>;
}

export function renderThreedPuzzle(ms: LocalModeState, text: string, dispatch: LocalDispatch): JSX.Element {
  const lines = text.split('\n').filter(x => x.length).map(line => line.split(/\s+/).filter(x => x.length));;
  return renderThreedPuzzleArray(ms, lines, dispatch);
}

export function renderThreedPuzzleInRect(ms: LocalModeState, text: string, globalRect: Rect, localRect: Rect, dispatch: LocalDispatch): JSX.Element {
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

  return renderThreedPuzzleArray(ms, globalArray, dispatch);
}

// return true if we've handled the key
function handleKey(state: AppState, modeState: LocalModeState, dispatch: LocalDispatch, code: string, key: string, mods: string): boolean {
  let m;

  if (mods.match(/C/) && code == 'KeyC') {
    dispatch({ t: 'copy' });
    return true;
  }

  if (mods.match(/C/) && code == 'KeyV' && modeState.selection != undefined) {
    dispatch({ t: 'paste' });
    return true;
  }

  if (mods.match(/C/) && code == 'KeyX') {
    dispatch({ t: 'cut' });
    return true;
  }

  if (mods == '' && key.match(/^[-<>^v+*\/%@=#a-z0-9.]$/)) {
    if (modeState.hoverCell != undefined) {
      dispatch({ t: 'editChar', char: key }); return true;
    }
  }

  switch (code) {
    case 'Space':
      if (modeState.hoverCell != undefined) {
        dispatch({ t: 'editChar', char: ' ' }); return true;
      }

    case 'Backspace': // fallthrough intentional
    case 'Delete':
      if (modeState.selection == undefined) {
        dispatch({ t: 'editChar', char: '.' }); return true;
      } else {
        dispatch({ t: 'clear' }); return true;
      }

    case 'ArrowLeft':
      if (modeState.executionTrace == undefined) {
        dispatch({ t: 'editChar', char: '<' }); return true;
      } else {
        dispatch({ t: 'incFrame', dframe: -1 }); return true;
      }
    case 'ArrowRight':
      if (modeState.executionTrace == undefined) {
        dispatch({ t: 'editChar', char: '>' }); return true;
      } else {
        dispatch({ t: 'incFrame', dframe: 1 }); return true;
      }
    case 'ArrowUp': dispatch({ t: 'editChar', char: '^' }); return true;
    case 'ArrowDown': dispatch({ t: 'editChar', char: 'v' }); return true;
    default: console.log(code); return false;
  }
}

export function RenderThreed(props: { state: AppState, modeState: LocalModeState, dispatch: Dispatch }): JSX.Element {
  const { state, modeState, dispatch } = props;
  const onMouseUp = (e: MouseEvent) => {
    ldis({ t: 'mouseUp' });
  };
  const onKeyDown = (e: KeyboardEvent) => {
    if (handleKey(state, modeState, ldis, e.code, e.key, (e.ctrlKey ? 'C' : ''))) {
      e.preventDefault();
      e.stopPropagation();
    }
  };
  React.useEffect(() => {
    window.addEventListener('keydown', onKeyDown);
    window.addEventListener('mouseup', onMouseUp);
    return () => {
      window.removeEventListener('keydown', onKeyDown);
      window.removeEventListener('mouseup', onMouseUp);
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
    renderedPuzzle = <div className="vert-stack"><div className="rendered-puzzle">{renderThreedPuzzleInRect(modeState, frame.frame, globalRect, localRect, ldis)}</div>
      <div><b>time</b>: {frame.time}</div>
      <input style={{ width: '40em' }} type="range" min={0} max={frames.length - 1} value={modeState.currentFrame} onInput={(e) => {
        ldis({ t: 'setCurrentFrame', frame: parseInt(e.currentTarget.value) })
      }}></input></div>;
  }
  else if (program != undefined) {
    renderedPuzzle = <div className="rendered-puzzle" onMouseDown={() => { ldis({ t: 'clearSelection' }) }}>{renderThreedPuzzleArray(modeState, program, ldis)}</div>;
  }

  const runProgram: React.MouseEventHandler = (e) => {
    let a = parseInt(modeState.a);
    if (isNaN(a)) { alert(`bad value for a: ${modeState.a}`); return }
    let b = parseInt(modeState.b);
    if (isNaN(b)) { alert(`bad value for b: ${modeState.b}`); return }
    dispatch({ t: 'doEffect', effect: { t: 'evalThreed', program: program!, a, b } })
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
      <button onClick={() => { ldis({ t: 'recover' }); }}>Recover</button>
      <button onClick={() => { ldis({ t: 'crop' }); }}>Crop</button>
      <button onClick={() => {
        navigator.clipboard.writeText(unparseThreedProgram(modeState.curProgram!));
        alert("copied whole program to clipboard");
      }}>Copy Program</button>
      <button onClick={() => ldis({ t: 'expandProgram' })}>Expand</button>
      <b style={{ color: 'white' }}>A</b><input onChange={onChange('a')} className="entry" value={modeState.a} size={4}></input>
      <b style={{ color: 'white' }}>B</b><input onChange={onChange('b')} className="entry" value={modeState.b} size={4}></input>
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

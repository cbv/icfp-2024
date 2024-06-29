import * as React from 'react';

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

export function renderThreedPuzzle(text: string): JSX.Element {
  const lines = text.split('\n').filter(x => x.length).map(line => line.split(/\s+/).filter(x => x.length));;
  return <table><tbody>{lines.map(renderRow)}</tbody></table>;
}

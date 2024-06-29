import * as React from 'react';

export function renderRow(row: string[]): JSX.Element {
  function divwrap(x: JSX.Element | string, cname: string = "threed-cell"): JSX.Element {
    return <div className={cname}>{x}</div>;
  }
  function renderCell(x: string) {
    switch (x) {
      case '.': return divwrap('', "threed-cell-empty");
      case 'S': return divwrap(<div className="S-chip">S</div>);
      case 'A': return divwrap(<div className="A-chip">A</div>);
      case 'B': return divwrap(<div className="B-chip">B</div>);
      default: return divwrap(x);
    }
  }
  return <tr>{row.map(x => <td>{renderCell(x)}</td>)}</tr>;
}

export function renderThreedPuzzle(text: string): JSX.Element {
  text = text.replace(/solve .*\n/, '');
  const lines = text.split('\n').filter(x => x.length).map(line => line.split(/\s+/).filter(x => x.length));;
  return <table><tbody>{lines.map(renderRow)}</tbody></table>;
}

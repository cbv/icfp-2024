

/*
Make this sort of read-back structure:
 . . . . . . . . . .
 . 0 . 0 . 0 . 0 . .
 0 + . + . + . + . .
 . 0 . 0 . 0 . . . .
 0 + . + . + . + . .
 . 0 . 0 . 0 . . . .
 0 + . + . + . + . .
 . . . . . . . S . .
*/

const grid = [];

let columns = 0;

function write(gx, gy, c) {
	while (grid.length <= gy) grid.push([]);
	while (grid[gy].length <= gx) grid[gy].push('.')
	columns = Math.max(columns, gx+1);
	grid[gy][gx] = c;
}

const SIZE = 101;

for (let y = 0; y < SIZE; ++y) {
	for (let x = 0; x < SIZE; ++x) {
		let gx = (x + 10) * 2 + 19;
		let gy = (y + 10) * 2 + 4;
		write(gx, gy, (x == 0 && y == 0 ? '1' : '0'));
		write(gx, gy+1, '+');
		if (x == 0) {
			write(gx-1,gy+1,'0');
		}
		if (x + 1 == SIZE) {
			write(gx+2, gy+1, '+');
			if (y == 0) {
				write(gx+2, gy, '0');
			}
			if (y + 1 == SIZE) {
				write(gx+2, gy+2, 'S');
			}
		}
	}
}

const fs = require('fs');

const corner = fs.readFileSync('../solutions/threed/threed11-corner.txt', 'utf8');
{
	let y = 0;
	for (const line of corner.split(/\s*[\r\n]+\s*/).slice(1)) {
		let x = 0;
		for (let col of line.split(/\s+/)) {
			if (col == 'DY') col = `${SIZE - 2}`;
			if (col == 'DX') col = `${SIZE - 3}`;
			if (col == 'S1') col = `${Math.floor(SIZE/2)}`;
			if (col == 'S2') col = `${SIZE - Math.floor(SIZE/2)}`;
			write(x,y,col);
			x += 1;
		}
		y += 1;
	}
}


for (const row of grid) {
	while (row.length <= columns) row.push('.');
}

out = 'solve 3d11';
for (let row = 0; row < grid.length; ++row) {
	out += '\n';
	for (let col = 0; col < grid[row].length; ++col) {
		let str = grid[row][col];
		if (str.length == 1) str = ' ' + str + ' ';
		else if (str.length == 2) str = str + ' ';
		else /* hope */;
		out += str;
	}
}
console.log(out);

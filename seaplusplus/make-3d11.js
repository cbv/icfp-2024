

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

// What if read-back looked more like this:


const grid = [];

let columns = 0;

function write(gx, gy, c) {
	while (grid.length <= gy) grid.push([]);
	while (grid[gy].length <= gx) grid[gy].push('.')
	columns = Math.max(columns, gx+1);
	grid[gy][gx] = c;
}

const SIZE = 4;

function extract(str) {
	const got = [];
	for (const line of str.split(/\s*[\r\n]+\s*/)) {
		const row = [];
		for (let col of line.split(/\s+/)) {
			row.push(col);
		}
		got.push(row);
	}
	return got;
}

const TILE = extract(
`.  @  .  .  @  .  .  *  .
 1  *  .  0  *  .  *  #  .
 .  .  .  .  .  .  .  .  .
 X  @  Y  4  @  2  6  @  3
 .  1  .  .  1  .  .  1  .`
);

//console.log(TILE);
//process.exit(1);

const TW = TILE[0].length;
const TH = TILE.length;

const TARGET_X = 21;
const TARGET_Y = 6;

const SOURCE_X = 19;
const SOURCE_Y = 4;

const ORIGIN_X = 17;
const ORIGIN_Y = 27;

for (let y = 0; y < SIZE; ++y) {
	for (let x = 0; x < SIZE; ++x) {
		const gx = x * TW + ORIGIN_X;
		const gy = y * TH + ORIGIN_Y;

		const dx = gx + 4 - TARGET_X;
		const dy = gy + 11 - TARGET_Y;

		const subst = { };
		subst.XA = Math.floor(dx/99);
		subst.XB = 99;
		subst.XC = dx - (subst.XA * subst.XB);

		subst.YA = Math.floor(dy/99);
		subst.YB = 99;
		subst.YC = dy - (subst.YA * subst.YB);
		
		subst.DX = (subst.XA * subst.XB) + subst.XC;
		subst.DY = (subst.YA * subst.YB) + subst.YC;

		subst.S = (x == 0 && y == 0 ? '0' : '1');

		for (let ty = 0; ty < TILE.length; ++ty) {
			for (let tx = 0; tx < TILE[ty].length; ++tx) {
				let cell = TILE[ty][tx];
				if (cell in subst) {
					cell = subst[cell];
				}
				write(gx + tx, gy + ty, cell);
			}
		}
	}
}

const fs = require('fs');

const corner = fs.readFileSync('../solutions/threed/threed11-corner.txt', 'utf8').trim();
{
	const subst = {};

	subst.DX = SIZE-3;
	subst.DY = SIZE-2;

	subst.S1 = Math.floor(SIZE/2);
	subst.S2 = SIZE - subst.S1;

	subst.OX = -ORIGIN_X;
	subst.OY = -ORIGIN_Y;

	subst.OX1 = 7;
	subst.OY1 = 63;

	subst.OX2 = 5;
	subst.OY2 = 76;

	subst.OX3 = 6;
	subst.OY3 = 92;

	/*subst.OX4 = 5;
	subst.OY4 = 76;

	subst.OX5 = 5;
	subst.OY5 = 76;*/

	subst.SX = -TW;
	subst.SY = -TH;

	subst.TX = -10; //TARGET_X - ORIGIN_X;
	subst.TY = -6; //TARGET_Y - ORIGIN_Y;

	subst.NN = 666;

	let y = 0;
	for (const line of corner.split(/\s*[\r\n]+\s*/).slice(1)) {
		let x = 0;
		for (let col of line.split(/\s+/)) {
			if (col in subst) col = `${subst[col]}`;
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
		out += ' ' + str;
		/*
		if (str.length == 1) str = ' ' + str + ' ';
		else if (str.length == 2) str = str + ' ';
		else str = str + ' '; //gotta pad them somehow
		out += str;
		*/
	}
}
console.log(out);

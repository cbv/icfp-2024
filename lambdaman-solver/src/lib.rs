#![allow(dead_code)]

#[derive(Debug, Clone, Copy, PartialEq, Hash, PartialOrd)]
pub enum Item {
    Wall,
    Dot,
    Empty,
    Man,
}

#[derive(Debug, Clone, Copy, PartialEq, Hash, PartialOrd)]
pub enum Move {
    Left,
    Right,
    Up,
    Down,
}

/// (0,0) is upper left
#[derive(Debug, Clone, Copy, PartialEq, Hash, PartialOrd, Default)]
pub struct Coords {
    row : usize,
    col : usize,
}

impl Coords {
    pub fn new(row : usize, col : usize) -> Self {
        Self { row, col }
    }
}


#[derive(Debug, Clone, PartialEq, Hash, PartialOrd)]
pub struct State {
    cells: Vec<Vec<Item>>,
    man: Coords,
}

impl std::ops::Index<Coords> for State {
    type Output = Item;

    fn index(&self, index: Coords) -> &Item {
        &self.cells[index.row as usize][index.col as usize]
    }
}

impl State {
    pub fn new(cells: Vec<Vec<Item>>) -> Self {
        let mut man : Coords = Default::default();
        for ridx in 0..cells.len() {
            let row = &cells[ridx];
            for cidx in 0..row.len() {
                let cell = row[cidx];
                if cell == Item::Man {
                    man = Coords::new(ridx, cidx)
                }
            }
        }
        Self { cells, man }
    }
}


pub fn parse_lambdaman_char(x: char) -> Item {
    match x {
        '#' => Item::Wall,
        '.' => Item::Dot,
        'L' => Item::Man,
        _ => Item::Empty,
        // _ => panic!("unknown lambdaman character '{}'", x),
    }
}

pub fn parse_lambda_puzzle(puzzle: String) -> State {
    let x: Vec<Vec<_>> = puzzle
        .split("\n")
        .map(|line| line.chars().map(parse_lambdaman_char).collect())
        .collect();
    println!("{:?}", x);
    State::new(x)
}

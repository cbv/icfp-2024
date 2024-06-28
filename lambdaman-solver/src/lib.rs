#![allow(dead_code)]

use std::collections::BTreeSet;

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
#[derive(Debug, Clone, Copy, PartialEq, Hash, PartialOrd, Ord, Default, Eq)]
pub struct Coords {
    row : usize,
    col : usize,
}

impl Coords {
    pub fn new(row : usize, col : usize) -> Self {
        Self { row, col }
    }
}


#[derive(Debug, Clone, PartialEq, PartialOrd)]
pub struct State {
    cells: Vec<Vec<Item>>,
    man: Coords,

    /// coordinates where there still is a dot
    dots: BTreeSet<Coords>,
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
        let mut dots = BTreeSet::new();
        for ridx in 0..cells.len() {
            let row = &cells[ridx];
            for cidx in 0..row.len() {
                let cell = row[cidx];
                match cell {
                    Item::Man => {
                        man = Coords::new(ridx, cidx)
                    }
                    Item::Dot => {
                        dots.insert(Coords::new(ridx, cidx));
                    }
                    _ => (),
                }
            }
        }
        Self { cells, man, dots }
    }

    pub fn width(&self) -> usize {
        if self.cells.is_empty() {
            0
        } else {
            self.cells[0].len()
        }
    }

    pub fn height(&self) -> usize {
        self.cells.len()
    }

    pub fn move_is_legal(&self, mv: Move) -> bool {
        match mv {
            Move::Left => self.man.col > 0,
            Move::Right => self.man.col + 1 < self.width(),
            Move::Up => self.man.row > 0,
            Move::Down => self.man.row + 1 < self.height(),
        }
    }

    pub fn coords_after_move(&self, mv: Move) -> Option<Coords> {
        if ! self.move_is_legal(mv) {
            None
        } else {
            match mv {
                Move::Left => Some (Coords { col: self.man.col - 1, ..self.man}),
                Move::Right => Some (Coords { col: self.man.col + 1, ..self.man}),
                Move::Up => Some (Coords { row: self.man.row - 1, ..self.man}),
                Move::Down => Some (Coords { row: self.man.row + 1, ..self.man}),
            }
        }
    }

    pub fn do_move(&mut self, mv: Move) {
        match self.coords_after_move(mv) {
            None => {
                panic!("illegal move")
            }
            Some(man2) => {
                self.man = man2
            }
        }
    }

    pub fn is_done(&self) -> bool {
        self.dots.is_empty()
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

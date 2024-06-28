#![allow(dead_code)]

#[derive(Debug, Clone, Copy, PartialEq, Hash, PartialOrd)]
pub enum Item {
    Wall,
    Dot,
    Empty,
    Man,
}

/// (0,0) is upper left
#[derive(Debug, Clone, Copy, PartialEq, Hash, PartialOrd)]
pub struct Coords {
    row : i32,
    col : i32,
}

#[derive(Debug, Clone, PartialEq, Hash, PartialOrd)]
pub struct State {
    cells: Vec<Vec<Item>>,
}

impl State {
    pub fn new(cells: Vec<Vec<Item>>) -> Self {
        Self { cells }
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

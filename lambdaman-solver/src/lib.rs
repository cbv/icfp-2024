#![allow(dead_code)]

#[derive(Debug)]
pub enum LambdaItem {
    Wall,
    Dot,
    Empty,
    Man,
}

pub fn parse_lambdaman_char(x: char) -> LambdaItem {
    match x {
        '#' => LambdaItem::Wall,
        '.' => LambdaItem::Dot,
        'L' => LambdaItem::Man,
        _ => LambdaItem::Empty,
        // _ => panic!("unknown lambdaman character '{}'", x),
    }
}

pub fn parse_lambda_puzzle(puzzle: String) {
    let x: Vec<Vec<_>> = puzzle
        .split("\n")
        .map(|line| line.chars().map(parse_lambdaman_char).collect())
        .collect();
    println!("{:?}", x);
}

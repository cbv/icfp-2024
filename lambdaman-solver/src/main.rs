#![allow(dead_code)]

use anyhow::Result;
use std::fs;
use std::path::Path;
use std::process::{Command, Stdio};

#[derive(Debug)]
enum LambdaItem {
    Wall,
    Dot,
    Empty,
    Man,
}

fn parse_lambdaman_char(x: char) -> LambdaItem {
    match x {
        '#' => LambdaItem::Wall,
        '.' => LambdaItem::Dot,
        'L' => LambdaItem::Man,
        _ => LambdaItem::Empty,
        // _ => panic!("unknown lambdaman character '{}'", x),
    }
}

fn parse_lambda_puzzle(puzzle: String) {
    let x: Vec<Vec<_>> = puzzle
        .split("\n")
        .map(|line| line.chars().map(parse_lambdaman_char).collect())
        .collect();
    println!("{:?}", x);
}

fn main() -> Result<()> {
    let output = Command::new("git")
        .arg("rev-parse")
        .arg("--show-toplevel")
        .stdout(Stdio::piped())
        .output()
        .unwrap();
    let git_dir = String::from_utf8(output.stdout).unwrap();
    let gd = git_dir.trim();

    let puzzpath = Path::new(gd)
        .join("puzzles/lambdaman/lambdaman2.txt")
        .into_os_string()
        .into_string()
        .unwrap();
    println!("{}", puzzpath);

    let message: String = fs::read_to_string(puzzpath)?;
    println!("{}", message);

    parse_lambda_puzzle(message);
    Ok(())
}

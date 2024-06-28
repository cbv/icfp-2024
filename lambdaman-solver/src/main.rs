#![allow(dead_code)]

use anyhow::{anyhow, Result};
use std::fs;
use std::path::Path;
use std::process::{Command, Stdio};
use std::collections::{BTreeSet, VecDeque};

use lambdaman::{Item, Coords, State, Move, parse_lambda_puzzle};

#[derive(Debug)]
struct SearchNode {
    // the position
    state: State,

    // maves that we took to get here
    moves: Vec<Move>,
}

fn find_next_move (state: State) -> Option<Move> {

    let mut to_visit : VecDeque<SearchNode> = VecDeque::new();
    let mut visited : BTreeSet<Coords> = BTreeSet::new();
    let nd = SearchNode {
        //coords: state.man_coords(),
        state,
        moves: vec![],
    };
    to_visit.push_back(nd);

    while let Some (SearchNode {state, moves}) = to_visit.pop_front() {
        for mv in [Move::Up, Move::Right, Move::Down, Move::Left] {
            if let Some(c1) = state.coords_after_move(mv) {
                if !visited.contains(&c1) {
                    visited.insert(c1);
                    let mut moves1 = moves.clone();
                    moves1.push(mv);
                    if state[c1] == Item::Dot {
                        return Some(moves1[0]);
                    }

                    let mut state1 = state.clone();
                    state1.do_move(mv);
                    to_visit.push_back(SearchNode {
                        state : state1,
                        moves: moves1,
                    })
                }
            }
        }
    }
    None
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

    let args: Vec<String> = std::env::args().collect();
    if args.len() != 2 {
        return Err(anyhow!("usage: {} PUZZLE_NUMBER", args[0]));
    }

    let puzzpath = Path::new(gd)
        .join(format!("puzzles/lambdaman/lambdaman{}.txt", args[1]))
        .into_os_string()
        .into_string()
        .unwrap();
    //println!("{}", puzzpath);

    let message: String = fs::read_to_string(puzzpath)?;
//    println!("{}", message);

    let mut state = parse_lambda_puzzle(message);

    print!("solve lambdaman{} ", args[1]);

    while let Some(mv) = find_next_move(state.clone()) {
        print!("{}", mv);
        state.do_move(mv);
        if state.is_done() {
            break;
        }
    }
    Ok(())
}

use anyhow::anyhow;

use std::fs;
//use std::process;
use std::ffi::OsStr;

pub fn main() -> anyhow::Result<()> {
    // two args: 1. problem file
    //           2. solution file (text)

    let args: Vec<String> = std::env::args().collect();
    if args.len() != 3 {
        eprintln!("usage: {} PROBLEM_FILE SOLUTION_FILE", args[0]);
    }

    let solution_path = std::path::Path::new(&args[2]);
    let solution = fs::read_to_string(&args[2])?;

    let moves = match solution_path.extension().and_then(OsStr::to_str).unwrap() {
        "icfp" => {
            //process::Command::new("../cc/eval.exe");
            // ...
            todo!()
        }
        "txt" => {
            // need to strip away the "solve lambdamanX "
            let moves_str = solution.split_whitespace().nth(2).unwrap();
            let moves = lambdaman::parse_moves(&moves_str)?;
            moves
        }
        e => {
            return Err(anyhow!("unknown extension: {e}"))
        }
    };

    let problem = fs::read_to_string(&args[1])?;
    let mut state = lambdaman::parse_lambda_puzzle(problem);
    let mut idx = 0;
    while !state.is_done() {
        println!("{state}");
        state.do_move(moves[idx]);
        idx += 1;
    }

    Ok(())
}

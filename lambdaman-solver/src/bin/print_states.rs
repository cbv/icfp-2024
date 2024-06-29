use anyhow::anyhow;

use std::fs;
use std::io::{Read, Write};
use std::process;
use std::ffi::OsStr;

pub fn main() -> anyhow::Result<()> {
    // two args: 1. problem file
    //           2. solution file (text)

    let args: Vec<String> = std::env::args().collect();
    if args.len() != 3 {
        eprintln!("usage: {} PROBLEM_FILE SOLUTION_FILE", args[0]);
    }

    let solution_path = std::path::Path::new(&args[2]);

    let solution_str = fs::read_to_string(solution_path)?;

    let moves_str = match solution_path.extension().and_then(OsStr::to_str).unwrap() {
        "icfp" => {
            let mut child = process::Command::new("../cc/eval.exe")
                .stdout(process::Stdio::piped())
                .stdin(process::Stdio::piped())
                .spawn()?;
            let mut stdin = child.stdin.take().unwrap();
            stdin.write_all(solution_str.as_bytes())?;
            drop(stdin);
            let mut stdout = child.stdout.take().unwrap();
            let mut result = String::new();
            stdout.read_to_string(&mut result)?;
            let mut result2 = String::new();
            for c in result.chars() {
                if c != '"' {
                    result2.push(c);
                }
            }
            result2
        }
        "txt" => {
            solution_str
        }
        e => {
            return Err(anyhow!("unknown extension: {e}"))
        }
    };


    // need to strip away the "solve lambdamanX "
    let moves_str = moves_str.split_whitespace().nth(2).unwrap();
    let moves = lambdaman::parse_moves(&moves_str)?;

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

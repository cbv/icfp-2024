#![allow(dead_code)]

use anyhow::Result;
use std::fs;
use std::path::Path;
use std::process::{Command, Stdio};

use lambdaman::parse_lambda_puzzle;

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

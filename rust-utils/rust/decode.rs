//use anyhow::anyhow;

//use std::io;
use std::io::Read;

pub fn main()  -> Result<(), anyhow::Error> {
    let mut input = String::new();
    let stdin = std::io::stdin();
    let mut handle = stdin.lock();
    handle.read_to_string(&mut input)?;
    let toks = icfp::parse_toks(&input)?;
    println!("{:?}", toks);
    Ok(())
}

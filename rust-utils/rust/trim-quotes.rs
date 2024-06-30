use anyhow::anyhow;
use std::io;

pub fn main() -> anyhow::Result<()> {
    let s = io::read_to_string(io::stdin())?;
    let trimmed = s.trim().as_bytes();
    if trimmed.is_empty() {
        return Err(anyhow!("empty after trimming"));
    }
    if trimmed[0] == b'"' && trimmed[trimmed.len() - 1] == b'"' {
        print!("{}", std::str::from_utf8(&trimmed[1..trimmed.len() - 1]).unwrap())
    } else {
        return Err(anyhow!("did not find expected quotes"));
    }
    Ok(())
}

use anyhow::anyhow;

// We'll make this an actual bigint if we need to.
type BigInt = u64;

#[derive(Debug, PartialEq)]
pub enum Tok {
    Bool(bool),
    Int(BigInt),
    Str(String),
    Unop(char),
    Binop(char),
    If,
    Lam(BigInt),
    Var(BigInt),
}

fn parse_base94 (b : &[u8]) -> anyhow::Result<BigInt> {
    let mut result = 0;
    for idx in 0 .. b.len() {
        result *= 94;
        let d = b[idx] - b'!';
        println!("d = {}", d);
        result += d as u64;
    }
    Ok(result)
}

#[test]
fn test_base_94 () {
    assert_eq!(parse_base94(b"/6").unwrap(), 1337)
}

pub fn parse_token (s : &[u8]) -> anyhow::Result<Tok> {
    if s.len() < 1 {
        return Err(anyhow!("empty token!"))
    }
    match s[0] {
        b'T' => Ok(Tok::Bool(true)),
        b'F' => Ok(Tok::Bool(false)),
        b'I' => Ok(Tok::Int(parse_base94(&s[1..])?)),
        c => Err(anyhow!("unrecognized token: {c}")),
    }
}

pub fn main() {
    println!("hello world");
}

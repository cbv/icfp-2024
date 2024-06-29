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

pub fn parse_base94 (b : &[u8]) -> anyhow::Result<BigInt> {
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

// does not include the leading 'I'.
pub fn encode_base94(mut n: BigInt) -> String {
    let mut digits : Vec<char> = vec![];
    while n > 0 {
        digits.push((33 + (n % 94) as u8 ) as char);
        n /= 94;
    }
    digits.reverse();

    let mut result = String::new();
    for d in digits {
        result.push(d);
    }

    result
}

#[test]
fn test_encode_base94 () {
    assert_eq!(encode_base94(1337), "/6".to_string());
}

const SPACE_ENCODING : &[u8] = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ \n";

pub fn parse_str (b : &[u8]) -> anyhow::Result<String> {
    let mut result = String::new();
    for idx in 0 .. b.len() {
        result.push(SPACE_ENCODING[(b[idx] - 33) as usize] as char);
    }
    Ok(result)
}

#[test]
fn test_parse_str() {
    assert_eq!(parse_str(b"B%,,/}Q/2,$_").unwrap(), "Hello World!".to_string());
}

// does not include the leading S
pub fn encode_str (b : &str) -> String {
    let mut result = String::new();
    'outer: for c in b.chars() {
        for ii in 0 .. SPACE_ENCODING.len() {
            if SPACE_ENCODING[ii] as char == c {
                result.push((ii as u8 + 33) as char);
                continue 'outer
            }
        }
        panic!("failed to encode character {c}");
    }
    result
}

#[test]
fn test_encode_str() {
    assert_eq!(encode_str("Hello World!"), "B%,,/}Q/2,$_".to_string())
}

impl std::fmt::Display for Tok {
    fn fmt(&self, f : &mut std::fmt::Formatter) -> std::fmt::Result {
        match self {
            Tok::Bool(true) => write!(f, "T"),
            Tok::Bool(false) => write!(f, "F"),
            Tok::Int(n) => write!(f, "I{}", encode_base94(*n)),
            Tok::Str(s) => write!(f, "S{}", encode_str(s)),
            Tok::Unop(u) => write!(f, "U{}", u),
            Tok::Binop(b) => write!(f, "B{}", b),
            Tok::If => write!(f, "?"),
            Tok::Lam(n) => write!(f, "L{}", encode_base94(*n)),
            Tok::Var(n) => write!(f, "v{}", encode_base94(*n)),
        }
    }
}

pub fn parse_token (s : &[u8]) -> anyhow::Result<Tok> {
    if s.len() < 1 {
        return Err(anyhow!("empty token!"))
    }
    match s[0] {
        b'T' => Ok(Tok::Bool(true)),
        b'F' => Ok(Tok::Bool(false)),
        b'I' => Ok(Tok::Int(parse_base94(&s[1..])?)),
        b'S' => Ok(Tok::Str(parse_str(&s[1..])?)),
        c => Err(anyhow!("unrecognized token: {c}")),
    }
}

pub fn parse_toks (s : &str) -> anyhow::Result<Vec<Tok>> {
    let mut result = vec![];
    for s in s.split_whitespace() {
        result.push(parse_token(s.as_bytes())?);
    }
    Ok(result)
}

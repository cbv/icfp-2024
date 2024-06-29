use anyhow::anyhow;

// We'll make this an actual bigint if we need to.
type BigInt = u64;

#[derive(Debug, PartialEq, Clone)]
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

#[derive(Debug, PartialEq, Clone)]
pub enum Expr {
    Bool(bool),
    Int(BigInt),
    Str(String),
    Unop(char, Box<Expr>),
    Binop(char, Box<Expr>, Box<Expr>),
    If(Box<Expr>, Box<Expr>, Box<Expr>),
    Lam(BigInt, Box<Expr>),
    Var(BigInt),
}

impl Expr {
    pub fn to_toks(&self, toks: &mut Vec<Tok>) {
        match self {
            Expr::Bool(b) => toks.push(Tok::Bool(*b)),
            Expr::Int(n) => toks.push(Tok::Int(*n)),
            Expr::Str(s) => toks.push(Tok::Str(s.clone())),
            Expr::Unop(op, a) => {
                toks.push(Tok::Unop(*op));
                a.to_toks(toks);
            }
            Expr::Binop(op, a, b) => {
                toks.push(Tok::Binop(*op));
                a.to_toks(toks);
                b.to_toks(toks);
            }
            Expr::If(cond, a, b) => {
                toks.push(Tok::If);
                cond.to_toks(toks);
                a.to_toks(toks);
                b.to_toks(toks);
            }
            Expr::Lam(v, body) => {
                toks.push(Tok::Lam(*v));
                body.to_toks(toks);
            }
            Expr::Var(v) => {
                toks.push(Tok::Var(*v));
            }
        }
    }
}

pub fn parse_base94 (b : &[u8]) -> anyhow::Result<BigInt> {
    let mut result = 0;
    for idx in 0 .. b.len() {
        result *= 94;
        let d = b[idx] - b'!';
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
    if n == 0 {
        return "!".into()
    }
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

// helpers for constructing Expr
pub mod expr {
    use crate::{BigInt, Expr};

    pub fn litbool(b: bool) -> Expr {
        Expr::Bool(b)
    }

    pub fn litstr(s : &str) -> Expr {
        Expr::Str(s.into())
    }

    pub fn litnum(n: BigInt) -> Expr {
        Expr::Int(n)
    }

    pub fn add(a: Expr, b: Expr) -> Expr {
        Expr::Binop('+', Box::new(a), Box::new(b))
    }

    pub fn mul(a: Expr, b: Expr) -> Expr {
        Expr::Binop('*', Box::new(a), Box::new(b))
    }

    pub fn sub(a: Expr, b: Expr) -> Expr {
        Expr::Binop('-', Box::new(a), Box::new(b))
    }

    pub fn div(a: Expr, b: Expr) -> Expr {
        Expr::Binop('/', Box::new(a), Box::new(b))
    }

    pub fn modulus(a: Expr, b: Expr) -> Expr {
        Expr::Binop('%', Box::new(a), Box::new(b))
    }

    pub fn concat(a: Expr, b: Expr) -> Expr {
        Expr::Binop('.', Box::new(a), Box::new(b))
    }

    pub fn app(a: Expr, b: Expr) -> Expr {
        Expr::Binop('$', Box::new(a), Box::new(b))
    }

    pub fn equ(a: Expr, b: Expr) -> Expr {
        Expr::Binop('=', Box::new(a), Box::new(b))
    }

    pub fn cond(cnd: Expr, a: Expr, b: Expr) -> Expr {
        Expr::If(Box::new(cnd), Box::new(a), Box::new(b))
    }

    pub fn vuse(v: &str) -> Expr {
        let n = crate::parse_base94(v.as_bytes()).unwrap();
        Expr::Var(n)
    }

    pub fn lam(v: &str, body: Expr) -> Expr {
        let n = crate::parse_base94(v.as_bytes()).unwrap();
        Expr::Lam(n, Box::new(body))
    }

    pub fn app_spine(a: Expr, b: Vec<Expr>) -> Expr {
        b.into_iter().fold(a, |acc, b1| { app(acc, b1) })
    }

    pub fn ycomb() -> Expr {
        lam("f", app(lam("x", app(vuse("f"), app(vuse("x"), vuse("x")))),
                     lam("x", app(vuse("f"), app(vuse("x"), vuse("x"))))))
    }

    pub fn rec(v : &str, body: Expr) -> Expr {
        app(ycomb(), lam(v, body))
    }

    pub fn let1(x: &str, val: Expr, body: Expr) -> Expr {
        app(lam(x, body), val)
    }

    pub fn let_bind(mut bindings: Vec<(&str, Expr)>, mut body: Expr) -> Expr {
        bindings.reverse();
        for (v, b) in bindings {
            body = app(lam(v, body), b)
        }
        body
    }

    pub fn repeat() -> Expr {
        rec("r",
            lam("s",
                lam("n",
                    cond(
                        equ(vuse("n"),
                            litnum(1)),
                        vuse("s"),
                        concat(vuse("s"),
                               app_spine(vuse("r"),
                                         vec![vuse("s"),
                                              sub(vuse("n"),
                                                  litnum(1))]))))))
    }
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

    assert_eq!(parse_str(b"3/,6%},!-\"$!-!.[}").unwrap(), "solve lambdaman6 ".to_string());
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
    assert_eq!(encode_str("Hello World!"), "B%,,/}Q/2,$_".to_string());
    assert_eq!(encode_str("solve lambdaman6 "), "3/,6%},!-\"$!-!.[}".to_string());
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

// Display as token stream
impl std::fmt::Display for Expr {
    fn fmt(&self, f : &mut std::fmt::Formatter) -> std::fmt::Result {
        let mut toks = vec![];
        self.to_toks(&mut toks);
        for ii in 0..toks.len() {
            let tok = &toks[ii];
            write!(f, "{tok}")?;
            if ii + 1 < toks.len() {
                write!(f, " ")?;
            }
        }
        Ok(())
    }
}

#[test]
fn test_display_expr() {
    use expr::*;
    assert_eq!(format!("{}", litstr("Hello World!")), "SB%,,/}Q/2,$_".to_string());
    assert_eq!(format!("{}", add(litnum(2), litnum(3))), "B+ I# I$".to_string());
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
        b'U' => Ok(Tok::Unop(s[1] as char)),
        b'B' => Ok(Tok::Binop(s[1] as char)),
        b'?' => Ok(Tok::Binop(s[1] as char)),
        b'L' => Ok(Tok::Lam(parse_base94(&s[1..])?)),
        b'v' => Ok(Tok::Var(parse_base94(&s[1..])?)),
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

// returns the parsed expression and the new position
pub fn get_expr_from_toks (toks : &[Tok], pos: usize) -> anyhow::Result<(Expr, usize)> {
    if pos >= toks.len() {
        return Err(anyhow!("ran off end of token stream trying to parse"))
    }
    let tok = &toks[pos];
    match tok {
        Tok::Bool(b) => Ok((Expr::Bool(*b), pos + 1)),
        Tok::Int(n) => Ok((Expr::Int(*n), pos + 1)),
        Tok::Str(s) => Ok((Expr::Str(s.clone()), pos + 1)),
        Tok::Unop(u) => {
            let (a, after_a) = get_expr_from_toks(toks, pos + 1)?;
            Ok((Expr::Unop(*u, Box::new(a)), after_a))
        }
        Tok::Binop(op) => {
            let (a, after_a) = get_expr_from_toks(toks, pos + 1)?;
            let (b, after_b) = get_expr_from_toks(toks, after_a)?;
            Ok((Expr::Binop(*op, Box::new(a), Box::new(b)), after_b))
        }
        Tok::If => {
            let (a, after_a) = get_expr_from_toks(toks, pos + 1)?;
            let (b, after_b) = get_expr_from_toks(toks, after_a)?;
            let (c, after_c) = get_expr_from_toks(toks, after_b)?;
            Ok((Expr::If(Box::new(a), Box::new(b), Box::new(c)), after_c))
        }
        Tok::Lam(v) => {
            let (a, after_a) = get_expr_from_toks(toks, pos + 1)?;
            Ok((Expr::Lam(*v, Box::new(a)), after_a))
        }
        Tok::Var(v) => Ok((Expr::Var(*v), pos + 1)),
    }
}

pub fn parse_expr(s: &str) -> anyhow::Result<Expr> {
    let toks = parse_toks(s)?;
    let (expr, _) = get_expr_from_toks(&toks[..], 0)?;
    Ok(expr)
}

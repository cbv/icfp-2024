
#[derive(Debug, PartialEq)]
pub enum Expr {
    Bool(bool),
    Int(u64),
    Str(String),
    Unop(char, Box<Expr>),
    Binop(char, Box<Expr>, Box<Expr>),
    If(Box<Expr>, Box<Expr>, Box<Expr>),
    Lam(u64, Box<Expr>),
    Var(u64),
}

pub fn main() {
    println!("hello world");
}

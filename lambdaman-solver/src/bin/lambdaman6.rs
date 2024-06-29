use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    let e =
        concat(
            litstr("solve lambdaman6 "),
            let1(
                "q",
                lam("a", concat(concat(vuse("a"), vuse("a")),
                                concat(vuse("a"), vuse("a")))),
                app(vuse("q"), app(vuse("q"), app(vuse("q"), litstr("RRRR"))))));
    println!("{e}");
    Ok(())
}

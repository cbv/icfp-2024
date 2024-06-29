use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // 50 x 50 grid with all empties. Start in upper left.

    let zag = let1(
        "R", repeat(),
        rec("S",
            lam("n",
                // have we finished?
                cond(equ(vuse("n"), litnum(25)),
                     // done
                     litstr(""),
                     // Otherwise do some moves and recurse
                     concat(
                         concat(
                             concat(
                                 app_spine(vuse("R"), vec![litstr("R"), litnum(50)]),
                                 litstr("D")),
                             concat(
                                 app_spine(vuse("R"), vec![litstr("L"), litnum(50)]),
                                 litstr("D"))),
                         app(vuse("S"), add(vuse("n"), litnum(1))))))));

    let e =
        concat(
            litstr("solve lambdaman9 "),
            app(zag, litnum(0)));
    println!("{e}");
    Ok(())
}

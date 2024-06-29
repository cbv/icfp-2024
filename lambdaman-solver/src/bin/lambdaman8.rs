use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // spiral

    let spiral = let_bind(
        vec![("R", repeatn(205) )],
        rec("S", lam(
            "n",
            // have we finished?
            cond(equ(vuse("n"), litnum(25)),
                 // done
                 litstr(""),
                 // Otherwise do some moves and recurse
                 concat(
                     concat(
                         app(vuse("R"), litstr("DL")),
                         app(vuse("R"), litstr("UR"))
                     ),
                     app(vuse("S"), add(vuse("n"), litnum(1)))
                 ),
            ))));

    let e =
        concat(
            litstr("solve lambdaman6 "),
            app(spiral, litnum(0)));
    println!("{e}");

    Ok(())
}

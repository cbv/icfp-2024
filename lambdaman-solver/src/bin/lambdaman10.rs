use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // 50 x 50 grid. every eleventh cell has a wall.
    // man starts in upper left.
     let walk =
        rec("S",
            lam("n",
                lam("r",
                    // have we finished?
                    cond(equ(vuse("n"), litnum(92000)),
                         // done
                         litstr(""),
                         // Otherwise do some moves and recurse
                         concat(
                             take(litnum(1),
                                  drop(modulus(div(vuse("r"), litnum(0x3fffffff)), litnum(4)),
                                       litstr("UDLR"))),
                             app_spine(
                                 vuse("S"),
                                 vec![add(vuse("n"), litnum(1)),
                                      modulus(add(mul(vuse("r"), litnum(1664525)),
                                                  litnum(1013904223)),
                                              litnum(0x100000000))]))))));

    let e = concat(litstr("solve lambdaman10 "),
                   app_spine(walk,
                             vec![litnum(0), litnum(43)]));
    println!("{e}");
    Ok(())
}

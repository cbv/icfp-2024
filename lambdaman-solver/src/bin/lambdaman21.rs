use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // 200 x 200 empty grid with "3D" written in walls in the middle

    // use a linear congruential RNG and do a random walk
    // (this doesn't actually work on this problem)
     let walk =
        rec("S",
            lam("n",
                lam("r",
                    // have we finished?
                    cond(equ(vuse("n"), litnum(500000)),
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

    let e = concat(litstr("solve lambdaman7 "),
                   app_spine(walk,
                             vec![litnum(0), litnum(11)]));
    println!("{e}");

    Ok(())
}

use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // 21 x 21 maze
    // use a linear congruential RNG and do a random walk

     let walk =
        rec("S",
            lam("n",
                lam("r",
                    // have we finished?
                    cond(equ(vuse("n"), litnum(42000)),
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

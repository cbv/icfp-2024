use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // big maze
    // man starts in upper left.

    // use a linear congruential RNG and do a random walk
    let walk =
        rec("S",
            lam("n",
                lam("r",
                    // have we finished?
                    cond(equ(vuse("n"), litnum(1000)),
                         // done
                         litstr(""),
                         // Otherwise do some moves and recurse
                         concat(
                             let_bind(
                                 vec![("x",
                                       modulus(div(vuse("r"), litnum(0x0fffffff)), litnum(4))),
                                      ("k",
                                       add(litnum(1), modulus(div(vuse("r"), litnum(0x00ffffff)), litnum(4))))],
                                  cond(equ(litnum(0), vuse("x")),
                                       app_spine(repeat(), vec![litstr("U"), vuse("k")]),
                                       cond(equ(litnum(1), vuse("x")),
                                            app_spine(repeat(), vec![litstr("D"), vuse("k")]),
                                            cond(equ(litnum(2), vuse("x")),
                                                 app_spine(repeat(), vec![litstr("L"), vuse("k")]),
                                                 app_spine(repeat(), vec![litstr("R"), vuse("k")]))))),
                             app_spine(
                                 vuse("S"),
                                 vec![add(vuse("n"), litnum(1)),
                                      modulus(add(mul(vuse("r"), litnum(1664525)),
                                                  litnum(1013904223)),
                                              litnum(0x100000000))]))))));

    let e = concat(litstr("solve lambdaman11 "),
                   app_spine(walk,
                             vec![litnum(0), litnum(42)]));
    println!("{e}");
    Ok(())
}

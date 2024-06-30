use anyhow::anyhow;

use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    let args: Vec<String> = std::env::args().collect();
    if args.len() != 3 {
        return Err(anyhow!("usage: {} PROBLEM_NUMBER RANDOM_SEED", args[0]));
    }

    let problem_num : u64 = args[1].parse().unwrap();
    let seed : u64 = args[2].parse().unwrap();

    // use a linear congruential RNG and do a random walk
    let walk =
        rec("S",
            lam("n",
                lam("r",
                    // have we finished?
                    cond(equ(vuse("n"), litnum(1000000)),
                         // done
                         litstr(""),
                         // Otherwise do some moves and recurse
                         concat(
                             let1("x",
                                  modulus(div(vuse("r"), litnum(0x0fffffff)), litnum(4)),
                                  cond(equ(litnum(0), vuse("x")),
                                       litstr("U"),
                                       cond(equ(litnum(1), vuse("x")),
                                            litstr("D"),
                                            cond(equ(litnum(2), vuse("x")),
                                                 litstr("L"),
                                                 litstr("R"))))),
                             app_spine(
                                 vuse("S"),
                                 vec![add(vuse("n"), litnum(1)),
                                      modulus(add(mul(vuse("r"), litnum(1664525)),
                                                  litnum(1013904223)),
                                              litnum(0x100000000))]))))));

    let e = concat(litstr(&format!("solve lambdaman{problem_num} ")),
                   app_spine(walk,
                             vec![litnum(0), litnum(seed)]));
    println!("{e}");

    Ok(())
}

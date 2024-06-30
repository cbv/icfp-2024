use anyhow::anyhow;

use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    let args: Vec<String> = std::env::args().collect();
    if args.len() != 5 {
        return Err(anyhow!("usage: {} PROBLEM_NUMBER RANDOM_SEED BIAS_STEP ITERS", args[0]));
    }

    let problem_num : u64 = args[1].parse().unwrap();
    let seed : u64 = args[2].parse().unwrap();
    let bias_step : u64 = args[3].parse().unwrap();
    let iters : u64 = args[4].parse().unwrap();


    // use a linear congruential RNG and do a random walk
    let walk =
        rec("S",
            lam("n",
                lam("r",
                    // have we finished?
                    cond(equ(vuse("n"), litnum(iters)),
                         // done
                         litstr(""),
                         // Otherwise do some moves and recurse
                         concat(
                             let_bind(
                                 vec![
                                     ("i",
                                      modulus(div(vuse("r"), litnum(0x0fffffff)), litnum(5))),
                                     ("j",
                                      modulus(div(vuse("n"), litnum(bias_step)), litnum(4))),
                                     ("p",
                                      take(litnum(5),
                                           drop(mul(vuse("j"), litnum(5)),
                                                litstr("UUDLRUDLRRDDLRUUDLLR"))))],
                                 take(litnum(1),
                                      drop(vuse("i"),
                                           vuse("p")))),
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

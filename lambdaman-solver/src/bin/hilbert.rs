use anyhow::anyhow;
use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // three args: 1. problem number
    //             2. number of steps per segment
    //             3. depth of recursion

    let args: Vec<String> = std::env::args().collect();
    if args.len() != 4 {
        return Err(anyhow!("usage: {} PROBLEM NUMBER STEPS_PER_SEGMENT RECURSION_DEPTH", args[0]));
    }

    let problem_num : u64 = args[1].parse().unwrap();
    let steps_per_seg : u64 = args[2].parse().unwrap();
    let depth : u64 = args[3].parse().unwrap();

    // Hilbert curve
    // man starts in upper left.

    /* (wikipedia)
    The Hilbert Curve can be expressed by a rewrite system (L-system).

    Alphabet : A, B
    Constants : F + −
    Axiom : A
    Production rules:

        A → +BF−AFA−FB+
        B → −AF+BFB+FA−
     */

    // a function that does one step of this rewriting.
    let rewrite =
        rec("R",
            lam("s",
                cond(equ(vuse("s"), litstr("")),
                     litstr(""),
                     concat(
                         let_bind(
                             vec![("0", take(litnum(1), vuse("s")))],
                             cond(
                                 equ(vuse("0"), litstr("A")),
                                 litstr("+BF-AFA-FB+"),
                                 cond(equ(vuse("0"), litstr("B")),
                                      litstr("-AF+BFB+FA-"),
                                      vuse("0")))),
                         sapp(vuse("R"), drop(litnum(1), vuse("s")))))));

    // function that consumes the L-system string and produces
    // a string of "UDLR" actions.

    let consume =
        rec("c",
            lam("L", // the L-system string
                lam("d", // the current direction, as a number from 0 to 3.
                    cond(equ(vuse("L"), litstr("")),
                         litstr(""),
                         let_bind(
                             vec![
                                 ("0", take(litnum(1), vuse("L"))),
                                 ("b", // (new dir, emitted moves)
                                  cond(
                                      equ(vuse("0"), litstr("-")),
                                          pair(add(vuse("d"), litnum(3)),
                                               litstr("")),
                                      cond(
                                          equ(vuse("0"), litstr("+")),
                                          pair(add(vuse("d"), litnum(1)),
                                               litstr("")),
                                              cond(equ(vuse("0"), litstr("F")),
                                                   pair(
                                                       vuse("d"),
                                                       repeatna(
                                                           steps_per_seg as usize,
                                                           take(
                                                               litnum(1),
                                                               drop(modulus(vuse("d"),
                                                                            litnum(4)),
                                                                    litstr("RDLU"))))),
                                                   pair(vuse("d"),
                                                        litstr(""))))))],
                             concat(snd(vuse("b")),
                                    sapp_spine(vuse("c"),
                                               vec![drop(litnum(1), vuse("L")),
                                                    fst(vuse("b"))])))))));
/*
    let e = concat(litstr("solve lambdaman16 "),
                   app_spine(
                       iter(),
                       vec![rewrite, litnum(2), litstr("A")]));
*/


    let e = concat(litstr(&format!("solve lambdaman{problem_num} ")),
                   sapp_spine(
                       consume,
                       vec![
                           sapp_spine(
                               iter(),
                               vec![rewrite, litnum(depth), litstr("A")]),
                           litnum(0)]));

    println!("{e}");
    Ok(())
}

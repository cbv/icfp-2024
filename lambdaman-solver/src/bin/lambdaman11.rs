use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // big maze
    // man starts in upper left.

    /*
    // use a linear congruential RNG and do a random walk
    let walk =
        rec("S",
            lam("n",
                lam("r",
                    // have we finished?
                    cond(equ(vuse("n"), litnum(100000)),
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
                                       sapp_spine(repeat(), vec![litstr("U"), vuse("k")]),
                                       cond(equ(litnum(1), vuse("x")),
                                            sapp_spine(repeat(), vec![litstr("D"), vuse("k")]),
                                            cond(equ(litnum(2), vuse("x")),
                                                 app_spine(repeat(), vec![litstr("L"), vuse("k")]),
                                                 app_spine(repeat(), vec![litstr("R"), vuse("k")]))))),
                             sapp_spine(
                                 vuse("S"),
                                 vec![add(vuse("n"), litnum(1)),
                                      modulus(add(mul(vuse("r"), litnum(1664525)),
                                                  litnum(1013904223)),
                                              litnum(0x100000000))]))))));

    let e = concat(litstr("solve lambdaman11 "),
                   sapp_spine(walk,
                             vec![litnum(0), litnum(42)]));
    println!("{e}");

     */

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

    // a function that does one step of the rewriting.
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
                                                           1,
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


    let e = concat(litstr("solve lambdaman11 "),
                   sapp_spine(
                       consume,
                       vec![
                           sapp_spine(
                               iter(),
                               vec![rewrite, litnum(5), litstr("A")]),
                           litnum(0)]));

    println!("{e}");
    Ok(())
}

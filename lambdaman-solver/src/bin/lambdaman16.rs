use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
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

    // let's write a function that does one step of this rewriting.
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
                                      litstr("-FA+BFB+FA-"),
                                      vuse("0")))),
                         app(vuse("R"), drop(litnum(1), vuse("s")))))));

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
                                                           2,
                                                           take(
                                                               litnum(1),
                                                               drop(modulus(vuse("d"),
                                                                            litnum(4)),
                                                                    litstr("RDLU"))))),
                                                   pair(vuse("d"),
                                                        litstr(""))))))],
                             concat(snd(vuse("b")),
                                    app_spine(vuse("c"),
                                              vec![drop(litnum(1), vuse("L")),
                                                   fst(vuse("b"))])))))));


    let e = concat(litstr("solve lambdaman16 "),
                   app_spine(
                       consume,
                       vec![
                           app_spine(
                               iter(),
                               vec![rewrite, litnum(4), litstr("A")]),
                           litnum(0)]));

    println!("{e}");
    Ok(())
}

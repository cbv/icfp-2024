use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // Hilbert curve
    // man starts in upper left.

    /* (wikipedia)
    The Hilbert Curve can be expressed by a rewrite system (L-system).
    Hilbert curve at its sixth iteration

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

    let e = concat(litstr("solve lambdaman16 "),
                   app_spine(
                       iter(),
                       vec![rewrite, litnum(1), litstr("A")]));

    println!("{e}");
    Ok(())
}

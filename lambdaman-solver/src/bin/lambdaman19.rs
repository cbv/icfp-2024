use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    //oblique square fractal thing

    // f(n):
    //  if n = 0 then return
    //  let g s :=
    //    walk n in direction (take 1 s)
    //    f(n/2)
    //    walk n in direction (drop 1 s)
    //  concat(g("UD"),
    //    concat(g("DU"),
    //      concat(g("LR"), g("RL"))))


    let walk =
        rec("f",
            lam("n",
                cond(
                    equ(vuse("n"), litnum(0)),
                    litstr(""),
                    let1(
                        "g",
                        lam("s",
                            concat(
                                sapp_spine(
                                    repeat_of(take(litnum(1), vuse("s"))),
                                    vec![vuse("n")]),
                                concat(
                                    sapp(vuse("f"),
                                         div(vuse("n"), litnum(2))),
                                    sapp_spine(
                                        repeat_of(drop(litnum(1), vuse("s"))),
                                        vec![vuse("n")])))),
                        concat(
                            sapp(vuse("g"), litstr("UD")),
                            concat(
                                sapp(vuse("g"), litstr("DU")),
                                concat(
                                    sapp(vuse("g"), litstr("LR")),
                                    sapp(vuse("g"), litstr("RL")))))))));


    let e = concat(litstr("solve lambdaman19 "),
                   sapp(walk, litnum(64)));

    println!("{e}");

    Ok(())
}

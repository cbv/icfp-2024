use icfp::expr::*;

pub fn main() -> anyhow::Result<()> {
    // spiral
/*
    let r = rec("r",
                lam("s",
                    lam("n",
                        cond(equ(n, litnum(1)), s, concat(s, appSpine(r, [s, sub(n, litnum(1))]))))));
  let repeat = vuse('R');
  const Mbody = litnum(205);
  const M = vuse('M')
  const spiral = letbind(
    [{ v: 'R', body: R }, { v: 'M', body: Mbody }],
    rec(S => lam(n =>
      // have we finished?
      cond(equ(n, litnum(25)),
        // done
        litstr(""),
        // Otherwise do some moves and recurse
        concat(
          concat(
            appSpine(repeat, [litstr("DL"), M]),
            appSpine(repeat, [litstr("UR"), M])
          ),
          app(S, add(n, litnum(1)))
        ),
      ))));

    let e =
        concat(
            litstr("solve lambdaman6 "),
            let1(
                "q",
                lam("a", concat(concat(vuse("a"), vuse("a")),
                                concat(vuse("a"), vuse("a")))),
                app(vuse("q"), app(vuse("q"), app(vuse("q"), litstr("RRRR"))))));
    println!("{e}");
*/
    Ok(())
}

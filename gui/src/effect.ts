import { Dispatch } from "./action";
import { decodeString, encodeString } from "./codec";
import { AppState, ThreedProgram } from "./state";
import { unparseThreedProgram } from "./threed-util";
import { EvalThreedRpc } from "./types";

export type Effect =
  | { t: 'none' }
  | { t: 'sendText', text: string }
  | { t: 'evalText', text: string }
  | { t: 'setHash', hash: string }
  | { t: 'evalThreed', a: number, b: number, program: ThreedProgram }
  ;

export function doEffect(state: AppState, dispatch: Dispatch, effect: Effect): void {
  switch (effect.t) {
    case 'none': {

    } break;
    case 'sendText': {
      (async () => {
        const preq = new Request("/api/space", {
          method: 'POST',
          body: JSON.stringify({ rawString: 'S' + encodeString(effect.text) }),
          headers: { "Content-type": "application/json" },
        });
        const presp = await fetch(preq);
        const ptext = await presp.text();
        if (ptext.match(/^S/)) {
          dispatch({ t: 'setOutputText', text: decodeString(ptext.replace(/^S/, '')) });
        }
        else {
          console.log(`don't know how to handle ${ptext}`);
        }
      })();
    } break;
    case 'evalText': {
      (async () => {
        const preq = new Request("/api/eval", {
          method: 'POST',
          body: JSON.stringify({ rawString: effect.text }),
          headers: { "Content-type": "application/json" },
        });
        const presp = await fetch(preq);
        const ptext = await presp.text();
        dispatch({ t: 'setOutputText', text: ptext });
      })();
    } break;
    case 'evalThreed': {
      localStorage['recover'] = JSON.stringify(effect.program);
      const rpc: EvalThreedRpc = { program: unparseThreedProgram(effect.program), a: effect.a, b: effect.b };
      (async () => {
        const preq = new Request("/api/eval-threed", {
          method: 'POST',
          body: JSON.stringify(rpc),
          headers: { "Content-type": "application/json" },
        });
        const presp = await fetch(preq);
        const ptext = await presp.text();
        dispatch({ t: 'setOutputText', text: ptext });
      })();
    } break;
    case 'setHash': {
      window.location.hash = effect.hash;
    } break;
  }
}

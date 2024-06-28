import { Dispatch } from "./action";
import { decodeString, encodeString } from "./codec";
import { AppState } from "./state";

export type Effect =
  | { t: 'none' }
  | { t: 'sendText', text: string }
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
    }
  }
}

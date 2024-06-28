import * as React from 'react';

// The point of this hook is that we assume we're given some types S ("State"), A ("Action"), E ("Effect"),
// and a reducer of type
//   reduce: (state: S, action: A) => Result<S, E>,
// as well as some
//   initialState: S,
// and an imperative way to evaluate effects
//   doEffect: (effect: E) => void):
//
// The reducer itself is meant to be a pure computation of a
// description of the next state and some side effects we should do,
// as a consequence of an action and the current state.

// Here's a toy example of its intended use.
// It's a counter that, whenever it reaches 5, has a side effect that it resets 1000ms later.
//
// type Effect = 'delayedReset';
// type Action = 'inc' | 'reset';

// function (): JSX.Element {
//   const [counter, dispatch] = useEffectfulReducer(0, reducer, doEffect);

//   function reducer(x: number, action: Action): { state: number, effects?: Effect[] } {
//     switch (action) {
//       case 'inc': return { state: x + 1, effects: x + 1 == 5 ? ['delayedReset'] : undefined };
//       case 'reset': return { state: 0 };
//     }
//   }

//   function doEffect(effect: Effect) {
//     if (effect == 'delayedReset') {
//       setTimeout(() => dispatch('reset'), 1000);
//     }
//   }

//   return <div><button onClick={() => dispatch('inc')}>inc</button>{counter}</div>;
// }

export type Result<S, E> = { state: S, effects?: E[] };

export function useEffectfulReducer<A, S, E>(
  initialState: S,
  reduce: (state: S, action: A) => Result<S, E>,
  doEffect: (state: S, dispatch: (action: A) => void, effect: E) => void):
  [S, (action: A) => void] {
  const [state, setState] = React.useState<S>(initialState);
  function dispatch(action: A) {
    const resultProm = new Promise<Result<S, E>>((res, _rej) => {
      setState(prevState => {
        const result = reduce(prevState, action);
        res(result);
        return result.state;
      });
    });
    resultProm.then(result => {
      if (result.effects !== undefined) {
        for (const effect of result.effects) {
          doEffect(
            result.state,
            dispatch, // Notice there is some recursion happening here!
            effect
          );
        }
      }
    });
  }
  return [state, dispatch];
}

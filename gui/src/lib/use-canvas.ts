import { useEffect, useRef } from 'react';
import { Point } from './types';

export type RawCanvasInfo = {
  c: HTMLCanvasElement,
  size: Point,
};

export type CanvasInfo = {
  c: HTMLCanvasElement,
  d: CanvasRenderingContext2D,
  size: Point,
};

export type CanvasGlInfo = {
  c: HTMLCanvasElement,
  d: WebGL2RenderingContext,
  size: Point,
};

export function useRawCanvas<S, CI>(
  state: S,
  render: (ci: CI, state: S) => void,
  deps: any[],
  onLoad: (ci: RawCanvasInfo) => CI,
): [
    React.RefCallback<HTMLCanvasElement>,
    React.MutableRefObject<CI | undefined>,
  ] {
  const infoRef = useRef<CI | undefined>(undefined);
  useEffect(() => {
    const ci = infoRef.current;
    if (ci != null) {
      render(ci, state);
    }
  }, deps ?? [state]);

  const ref: React.RefCallback<HTMLCanvasElement> = canvas => {
    if (infoRef.current === undefined) {
      if (canvas !== null) {
        const width = Math.floor(canvas.getBoundingClientRect().width);
        const height = Math.floor(canvas.getBoundingClientRect().height);
        canvas.width = width * devicePixelRatio;
        canvas.height = height * devicePixelRatio;
        const rawInfo = { c: canvas, size: { x: width, y: height } };
        infoRef.current = onLoad(rawInfo);
      }
    }
  };
  return [ref, infoRef];
}

export function useCanvas<S>(
  state: S,
  render: (ci: CanvasInfo, state: S) => void,
  deps: any[],
  onLoad: (ci: CanvasInfo) => void,
): [
    React.RefCallback<HTMLCanvasElement>,
    React.MutableRefObject<CanvasInfo | undefined>,
  ] {
  function dprRender(ci: CanvasInfo, state: S): void {
    const { d } = ci;
    d.save();
    d.scale(devicePixelRatio, devicePixelRatio);
    render(ci, state);
    d.restore();
  }
  return useRawCanvas<S, CanvasInfo>(state, dprRender, deps, rci => {
    const ci = { ...rci, d: rci.c.getContext('2d')! };
    onLoad(ci);
    return ci;
  });
}

export function useCanvasGl<S, ENV>(
  state: S,
  render: (ci: CanvasGlInfo, env: ENV, state: S) => void,
  deps: any[],
  onLoad: (ci: CanvasGlInfo) => ENV,
): [
    React.RefCallback<HTMLCanvasElement>,
    React.MutableRefObject<{ ci: CanvasGlInfo, env: ENV } | undefined>,
  ] {
  return useRawCanvas<S, { ci: CanvasGlInfo, env: ENV }>(state, ({ ci, env }, s) => { render(ci, env, s); }, deps, rci => {
    const d = rci.c.getContext('webgl2', { premultipliedAlpha: false, antialias: false });
    if (!d) {
      throw `This environment does not support WebGL2`;
    }
    const ci = { ...rci, d };
    const env = onLoad(ci);
    return { ci, env };
  });
}

export function useNonreactiveCanvasGl<ENV>(
  onLoad: (ci: CanvasGlInfo) => ENV,
): [
    React.RefCallback<HTMLCanvasElement>,
    React.MutableRefObject<{ ci: CanvasGlInfo, env: ENV } | undefined>,
  ] {
  return useRawCanvas<{}, { ci: CanvasGlInfo, env: ENV }>({}, () => { }, [], rci => {
    const d = rci.c.getContext('webgl2', { premultipliedAlpha: false, antialias: false });
    if (!d) {
      throw `This environment does not support WebGL2`;
    }
    const ci = { ...rci, d };
    return { ci, env: onLoad(ci) };
  });
}

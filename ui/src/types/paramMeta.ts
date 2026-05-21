export type { Addr, Confidence, ParamMeta } from './paramMeta.generated';

export function formatRawRange(rawMin: number, rawMax: number): string {
  return `${rawMin}…${rawMax}`;
}

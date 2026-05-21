import type { ParamMeta } from '../types/paramMeta';

export function uniqueSortedGroups(params: ParamMeta[]): string[] {
  return [...new Set(params.map((param) => param.group).filter(Boolean))].sort();
}

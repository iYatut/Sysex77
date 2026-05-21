import type { ParamMeta } from '../types/paramMeta';
import type { ParamDumpRow, ParamDumpSource } from '../types/paramDump';

export interface DumpTableRow extends ParamDumpRow {
  tag: string;
  uiLabel: string;
  group: string;
}

export function metaByIdMap(params: ParamMeta[]): Map<string, ParamMeta> {
  return new Map(params.map((param) => [param.id, param]));
}

export function joinDumpWithMeta(dump: ParamDumpRow[], params: ParamMeta[]): DumpTableRow[] {
  const byId = metaByIdMap(params);

  return dump
    .map((row) => {
      const meta = byId.get(row.id);
      return {
        ...row,
        tag: meta?.tag ?? '',
        uiLabel: meta?.uiLabel ?? '',
        group: meta?.group ?? '',
      };
    })
    .sort((a, b) => a.id.localeCompare(b.id) || a.elementIndex - b.elementIndex);
}

export function uniqueDumpGroups(rows: DumpTableRow[]): string[] {
  return [...new Set(rows.map((row) => row.group).filter(Boolean))].sort();
}

export function matchesDumpGroup(row: DumpTableRow, groupQuery: string): boolean {
  const needle = groupQuery.trim();
  return !needle || row.group === needle;
}

export function matchesDumpSource(row: DumpTableRow, sourceQuery: string): boolean {
  const needle = sourceQuery.trim();
  if (!needle) {
    return true;
  }

  if (needle === 'live') {
    return row.source === 'Live';
  }

  if (needle === 'bulk') {
    return row.source === 'Bulk8101' || row.source === 'Bulk0040';
  }

  return row.source === (needle as ParamDumpSource);
}

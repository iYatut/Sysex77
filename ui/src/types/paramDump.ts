export type ParamDumpSource = 'Live' | 'Bulk8101' | 'Bulk0040' | 'None';

export interface ParamDumpRow {
  id: string;
  elementIndex: number;
  raw: number | null;
  ui: number | null;
  source: ParamDumpSource;
}

/** Как LiveSynthState::paramSourceTag — короткий тег источника. */
export function paramSourceTag(source: ParamDumpSource): string {
  switch (source) {
    case 'Live':
      return 'live';
    case 'Bulk8101':
      return '8101';
    case 'Bulk0040':
      return '0040';
    default:
      return '-';
  }
}

export function formatDumpValue(value: number | null): string {
  return value === null || value === undefined ? '—' : String(value);
}

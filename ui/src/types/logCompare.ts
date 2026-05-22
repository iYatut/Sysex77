import type { LogRowOverlay } from './paramValueMap';
import type { LogDuplicateEntry } from '../utils/parseSysexLog';
import type { ParsedLogLine } from '../utils/parseSysexLog';

export type LogCompareResult = {
  paramId: string;
  elementIndex: number;
  parsedCount: number;
  duplicates: LogDuplicateEntry[];
  overlaysByRaw: Record<string, LogRowOverlay>;
  extras: ParsedLogLine[];
};

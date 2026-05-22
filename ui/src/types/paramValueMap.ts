export type PresentationKind = 'numeric' | 'namedEnum' | 'decoded' | 'text';

export const PRESENTATION_KIND_OPTIONS: { value: PresentationKind; label: string }[] = [
  { value: 'numeric', label: 'numeric — число raw/ui' },
  { value: 'decoded', label: 'decoded — из codec' },
  { value: 'namedEnum', label: 'namedEnum — имя из enum' },
  { value: 'text', label: 'text — как на LCD SY99' },
];

export type ParamValueMapRow = {
  index: number;
  raw: number;
  ui: number;
  sysexHex: string;
  presentationKind: PresentationKind;
  programLabel: string;
  registryLabel: string | null;
  labelMismatch: boolean;
};

export type ParamValueMap = {
  paramId: string;
  elementIndex: number;
  rawMin: number;
  rawMax: number;
  /** Диапазон UI для параметров с codec (напр. ATPBR −12…+12) */
  uiMin?: number;
  uiMax?: number;
  rows: ParamValueMapRow[];
};

export type LogMatchStatus =
  | 'match'
  | 'sysexMismatch'
  | 'labelMismatch'
  | 'extraInLog'
  | 'duplicateInLog';

export type LogRowOverlay = {
  logIndex?: number;
  logLabel?: string;
  logSysexHex?: string;
  status?: LogMatchStatus;
};

export type { LogDuplicateEntry, LogDuplicateReason } from '../utils/parseSysexLog';

/** Строка карты с учётом правок редактора */
export type DisplayValueMapRow = ParamValueMapRow & {
  sysexHex: string;
  programLabel: string;
  ui: number;
  isEdited: boolean;
  isVerified: boolean;
};

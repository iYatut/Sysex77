export type PresentationKind = 'numeric' | 'namedEnum' | 'decoded';

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

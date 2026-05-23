export interface LibraryPageInfo {
  id: string;
  label: string;
  slotCount: number;
  isMulti: boolean;
  available: boolean;
  captureFile: string | null;
}

export interface LibraryTree {
  id: string;
  label: string;
  active: boolean;
  capturesDir: string;
  pages: LibraryPageInfo[];
}

export interface LibraryVoiceSummary {
  mm: number;
  slotCode: string;
  name: string;
  elmodeRaw?: number | null;
}

export interface LibraryVoicesResponse {
  pageId: string;
  pageLabel: string;
  slotCount: number;
  captureFile: string | null;
  available: boolean;
  voices: LibraryVoiceSummary[];
}

export interface LibraryVoiceParam {
  id: string;
  elementIndex: number;
  sysexGroup?: number;
  group: string;
  uiLabel: string;
  paramTag: string;
  addressHex: string;
  bindStatus?: string;
  sy99LcdPage?: number | null;
  registryId?: string | null;
  raw8101: number | null;
  raw0040: number | null;
  uiValue: number | null;
  valueLabel: string;
  inDump: boolean;
  autoCheckStatus?: string;
  autoCheckMessage?: string;
  expectedUi?: number | null;
  fixtureId?: string;
}

export interface LibraryCatalogStats {
  totalRows: number;
  catalogParams: number;
  inDump: number;
  confirmed: number;
  manualOnly: number;
  candidate: number;
}

export interface LibraryVoiceDetail {
  pageId: string;
  pageLabel: string;
  mm: number;
  slotCode: string;
  voiceName: string;
  captureFile: string;
  catalogStats?: LibraryCatalogStats;
  params: LibraryVoiceParam[];
}

export function formatLibraryValue(value: number | null): string {
  return value === null || value === undefined ? '—' : String(value);
}

import type { ParamMeta as ParamMetaCore } from './paramMeta.generated';

export type { Addr, Confidence } from './paramMeta.generated';
export type { ParamCatalogEntry, ParamCodecKind } from './paramCatalog';

import type { PresentationKind } from './paramValueMap';

export type RowOverride = {
  sysexHex?: string;
  programLabel?: string;
  ui?: number;
  /** Вид отображения подписи (напр. text + «off» для ui=0) */
  presentationKind?: PresentationKind;
};

/** Строка своего списка (params_meta.json). null = использовать эталон API. */
export type StoredValueMapRow = {
  raw: number;
  ui: number;
  sysexHex: string;
  programLabel: string;
  presentationKind?: PresentationKind;
  registryLabel?: string | null;
};

/**
 * Состояние сверки с SY99 (сессия / legacy params_meta).
 * @deprecated Не часть чистого каталога params_meta.json — игнорируется при load каталога,
 * не сериализуется в ParamCatalogEntry.
 */
export type Sy99ElementVerification = {
  /** @deprecated Truth-поле; не хранить в params_meta catalog. */
  confirmed: boolean;
  /** @deprecated Truth-поле; не хранить в params_meta catalog. */
  confirmedAt: string | null;
  /** @deprecated Truth-поле (canonicalRaw); не хранить в params_meta catalog. */
  canonicalRaw?: number | null;
  /** @deprecated Truth-поле; не хранить в params_meta catalog. */
  verifiedRaws: number[];
  /** @deprecated Truth-поле; не хранить в params_meta catalog. */
  customRows?: StoredValueMapRow[] | null;
  /** @deprecated Truth-поле; не хранить в params_meta catalog. */
  rowOverrides: Record<string, RowOverride>;
  /** @deprecated Truth-поле; не хранить в params_meta catalog. */
  observationLog?: string;
};

/** Keys are element indices as strings: "0" … "3". */
export type Sy99VerificationByElement = Record<string, Sy99ElementVerification>;

export type ParamMeta = ParamMetaCore & {
  /**
   * @deprecated Сверка/правда SY99 — не часть params_meta catalog.
   * API по-прежнему может отдавать блок; при экспорте каталога отбрасывается.
   */
  sy99Verification?: Sy99VerificationByElement;
  /** @deprecated Справочно; не часть params_meta catalog. */
  sy99LcdPage?: number;
};

export function formatRawRange(rawMin: number, rawMax: number): string {
  return `${rawMin}…${rawMax}`;
}

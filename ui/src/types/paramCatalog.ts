import { z } from 'zod';

/** Кодек raw ↔ UI в каталоге параметров (params_meta.json). */
export type ParamCodecKind = 'identity' | 'signed' | 'enum';

/**
 * Одна строка чистого каталога параметров (params_meta.json).
 * Без полей «правды» / последнего пресета (canonicalRaw, sy99Verification, …).
 */
export interface ParamCatalogEntry {
  id: string;
  groupId: string;
  /** 0…3 для per-element; для common не задаётся. */
  elementIndex?: number;
  rawMin: number;
  rawMax: number;
  uiMin: number;
  uiMax: number;
  decode: ParamCodecKind;
  encode: ParamCodecKind;
  /** Подпись enum → raw (индекс в enumNames API). */
  enumMap?: Record<string, number>;
  /** Шаблон live SysEx: `F0 43 10 34 … xx F7`, xx = значение. */
  sysexTemplate?: string;
}

export const paramCodecKindSchema = z.enum(['identity', 'signed', 'enum']);

export const paramCatalogEntrySchema = z.object({
  id: z.string().min(1),
  groupId: z.string().min(1),
  elementIndex: z.number().int().min(0).max(3).optional(),
  rawMin: z.number().finite(),
  rawMax: z.number().finite(),
  uiMin: z.number().finite(),
  uiMax: z.number().finite(),
  decode: paramCodecKindSchema,
  encode: paramCodecKindSchema,
  enumMap: z.record(z.string(), z.number().int()).optional(),
  sysexTemplate: z.string().optional(),
});

export const paramCatalogSchema = z.array(paramCatalogEntrySchema);

/**
 * Поля верхнего уровня params_meta.json, которые не входят в каталог
 * (игнорируются при load, не пишутся при save каталога).
 */
export const PARAM_CATALOG_DEPRECATED_TOP_LEVEL_KEYS = [
  'sy99Verification',
  'sy99LcdPage',
  'confidence',
  'canonicalRaw',
  'truthRaw',
  'baselineRaw',
  'lastKnownRaw',
  'verifiedRaws',
  'customRows',
  'rowOverrides',
  'observationLog',
  'confirmed',
  'confirmedAt',
] as const;

/** Устаревшие поля внутри sy99Verification.* (если JSON ещё не очищен). */
export const PARAM_CATALOG_DEPRECATED_VERIFICATION_KEYS = [
  'canonicalRaw',
  'truthRaw',
  'baselineRaw',
  'lastKnownRaw',
  'verifiedRaws',
  'customRows',
  'rowOverrides',
  'observationLog',
  'confirmed',
  'confirmedAt',
  'hiddenRaws',
] as const;

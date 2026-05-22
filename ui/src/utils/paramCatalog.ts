import type { ParamMeta } from '../types/paramMeta';
import type { ParamCatalogEntry, ParamCodecKind } from '../types/paramCatalog';
import {
  PARAM_CATALOG_DEPRECATED_TOP_LEVEL_KEYS,
  paramCatalogEntrySchema,
} from '../types/paramCatalog';
import type { ParamMetaPatch } from './paramForm';
import { alignEnumNamesToRawRange } from './paramForm';
import { storedRawBoundsToFormFields } from './paramCodec';

const TRUTH_TOP_LEVEL = new Set<string>(PARAM_CATALOG_DEPRECATED_TOP_LEVEL_KEYS);

/** Ключ каталога: id + element (common = без индекса). */
export function paramCatalogEntryKey(entry: Pick<ParamCatalogEntry, 'id' | 'elementIndex'>): string {
  return entry.elementIndex === undefined ? entry.id : `${entry.id}:${entry.elementIndex}`;
}

function isPlainObject(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}

/** Удаляет truth/verification поля из сырой JSON-строки каталога. */
export function stripTruthFieldsFromCatalogRecord(raw: Record<string, unknown>): Record<string, unknown> {
  const out: Record<string, unknown> = {};
  for (const [key, value] of Object.entries(raw)) {
    if (TRUTH_TOP_LEVEL.has(key)) {
      continue;
    }
    out[key] = value;
  }
  return out;
}

function apiDecodeToCatalogCodec(
  decode: string,
  encode: string,
  enumNames: string[],
): { decode: ParamCodecKind; encode: ParamCodecKind } {
  const hasEnum = enumNames.some((name) => name.trim().length > 0);
  if (hasEnum) {
    return { decode: 'enum', encode: 'enum' };
  }

  const signedPattern =
    /signed|Detune|NoteShift|MixerEffectSend/i.test(decode) ||
    /signed|Detune|NoteShift|MixerEffectSend/i.test(encode);

  if (signedPattern) {
    return { decode: 'signed', encode: 'signed' };
  }

  return { decode: 'identity', encode: 'identity' };
}

function enumNamesToMap(enumNames: string[], rawMin: number, rawMax: number): Record<string, number> | undefined {
  const map: Record<string, number> = {};
  const lo = Math.max(0, rawMin);
  const hi = Math.min(rawMax, enumNames.length - 1);

  for (let raw = lo; raw <= hi; raw += 1) {
    const label = enumNames[raw]?.trim();
    if (label) {
      map[label] = raw;
    }
  }

  return Object.keys(map).length > 0 ? map : undefined;
}

function enumMapToNames(
  enumMap: Record<string, number> | undefined,
  rawMin: number,
  rawMax: number,
): string[] {
  if (!enumMap) {
    return [];
  }

  const names: string[] = [];
  for (const [label, raw] of Object.entries(enumMap)) {
    if (typeof raw !== 'number' || !Number.isFinite(raw)) {
      continue;
    }
    names[raw] = label;
  }

  return alignEnumNamesToRawRange(names, rawMin, rawMax);
}

/** Live SysEx-шаблон из addr MetaRegistry (`F0 43 10 34 b3 b4 b5 b6 00 xx F7`). */
export function sysexTemplateFromAddr(
  addr: ParamMeta['addr'],
  perElement: boolean,
  elementIndex = 0,
): string {
  const b4 = perElement ? (elementIndex & 0x7f) * 0x20 : addr.b4;
  const parts = [
    'F0',
    '43',
    '10',
    '34',
    addr.b3.toString(16).toUpperCase().padStart(2, '0'),
    b4.toString(16).toUpperCase().padStart(2, '0'),
    addr.b5.toString(16).toUpperCase().padStart(2, '0'),
    addr.b6.toString(16).toUpperCase().padStart(2, '0'),
    '00',
    'xx',
    'F7',
  ];
  return parts.join(' ');
}

/** API ParamMeta → чистая строка каталога (без sy99Verification и пр.). */
export function paramMetaToCatalogEntry(
  param: ParamMeta,
  options?: { elementIndex?: number; sysexDevice?: number },
): ParamCatalogEntry {
  void options?.sysexDevice;

  const { rawMin: uiMin, rawMax: uiMax } = storedRawBoundsToFormFields(
    param,
    param.rawMin,
    param.rawMax,
  );
  const codec = apiDecodeToCatalogCodec(param.decode, param.encode, param.enumNames);
  const elementIndex = options?.elementIndex;

  return {
    id: param.id,
    groupId: param.group,
    ...(elementIndex !== undefined ? { elementIndex } : {}),
    rawMin: param.rawMin,
    rawMax: param.rawMax,
    uiMin,
    uiMax,
    decode: codec.decode,
    encode: codec.encode,
    enumMap: enumNamesToMap(param.enumNames, param.rawMin, param.rawMax),
    sysexTemplate: sysexTemplateFromAddr(
      param.addr,
      param.perElement,
      elementIndex ?? 0,
    ),
  };
}

/** ParamCodecKind → строки decode/encode для PUT /params/:id. */
export function catalogCodecToApiPatch(
  param: Pick<ParamMeta, 'decode' | 'encode'>,
  decode: ParamCodecKind,
  encode: ParamCodecKind,
): Pick<ParamMetaPatch, 'decode' | 'encode'> {
  if (decode === 'identity' && encode === 'identity') {
    return { decode: 'identity', encode: 'identity' };
  }

  if (decode === 'signed' || encode === 'signed') {
    const signedPattern = /signed|Detune|NoteShift|MixerEffectSend/i;
    if (signedPattern.test(param.decode) && signedPattern.test(param.encode)) {
      return { decode: param.decode, encode: param.encode };
    }
    return {};
  }

  if (decode === 'enum' || encode === 'enum') {
    return {};
  }

  return { decode: 'identity', encode: 'identity' };
}

/** Каталог → PATCH для PUT /params/:id (форма Meta, без truth). */
export function catalogEntryToMetaPatch(entry: ParamCatalogEntry): ParamMetaPatch {
  return {
    group: entry.groupId,
    rawMin: entry.rawMin,
    rawMax: entry.rawMax,
    enumNames: enumMapToNames(entry.enumMap, entry.rawMin, entry.rawMax),
    decode: entry.decode === 'identity' ? 'identity' : undefined,
    encode: entry.encode === 'identity' ? 'identity' : undefined,
  };
}

/** Парсит одну запись каталога из JSON, игнорируя truth-поля и legacy API-поля. */
export function parseParamCatalogEntry(raw: unknown): ParamCatalogEntry | null {
  if (!isPlainObject(raw)) {
    return null;
  }

  const stripped = stripTruthFieldsFromCatalogRecord(raw);

  // Legacy API row → catalog (group/tag/uiLabel/confidence/addr/enumNames игнорируются при strict parse)
  if (typeof stripped.id === 'string' && typeof stripped.groupId !== 'string') {
    if (typeof stripped.group === 'string') {
      stripped.groupId = stripped.group;
    }
  }

  if (
    typeof stripped.id === 'string' &&
    typeof stripped.groupId === 'string' &&
    typeof stripped.rawMin === 'number' &&
    typeof stripped.rawMax === 'number'
  ) {
    if (stripped.uiMin === undefined || stripped.uiMax === undefined) {
      const uiMin = typeof stripped.rawMin === 'number' ? stripped.rawMin : 0;
      const uiMax = typeof stripped.rawMax === 'number' ? stripped.rawMax : 127;
      stripped.uiMin = uiMin;
      stripped.uiMax = uiMax;
    }

    if (stripped.decode === undefined) {
      stripped.decode = 'identity';
    }
    if (stripped.encode === undefined) {
      stripped.encode = stripped.decode;
    }

    if (stripped.enumMap === undefined && Array.isArray(stripped.enumNames)) {
      const names = stripped.enumNames as string[];
      const map = enumNamesToMap(names, stripped.rawMin as number, stripped.rawMax as number);
      if (map) {
        stripped.enumMap = map;
      }
    }

    if (stripped.sysexTemplate === undefined && isPlainObject(stripped.addr)) {
      const addrObj = stripped.addr;
      const addr = {
        b3: Number(addrObj.b3 ?? 0),
        b4: Number(addrObj.b4 ?? 0),
        b5: Number(addrObj.b5 ?? 0),
        b6: Number(addrObj.b6 ?? 0),
        perElement: Boolean(addrObj.perElement),
      };
      const perElement = Boolean(stripped.perElement);
      stripped.sysexTemplate = sysexTemplateFromAddr(
        addr,
        perElement,
        typeof stripped.elementIndex === 'number' ? stripped.elementIndex : 0,
      );
    }
  }

  const parsed = paramCatalogEntrySchema.safeParse(stripped);
  return parsed.success ? parsed.data : null;
}

/** Парсит params_meta.json как массив каталога; truth-поля отбрасываются. */
export function parseParamCatalogJson(raw: unknown): ParamCatalogEntry[] {
  if (!Array.isArray(raw)) {
    return [];
  }

  const out: ParamCatalogEntry[] = [];
  for (const row of raw) {
    const entry = parseParamCatalogEntry(row);
    if (entry) {
      out.push(entry);
    }
  }
  return out;
}

/** Сериализация каталога: только descriptor-поля ParamCatalogEntry. */
export function serializeParamCatalog(entries: ParamCatalogEntry[]): ParamCatalogEntry[] {
  return entries.map((entry) => {
    const clean: ParamCatalogEntry = {
      id: entry.id,
      groupId: entry.groupId,
      rawMin: entry.rawMin,
      rawMax: entry.rawMax,
      uiMin: entry.uiMin,
      uiMax: entry.uiMax,
      decode: entry.decode,
      encode: entry.encode,
    };

    if (entry.elementIndex !== undefined) {
      clean.elementIndex = entry.elementIndex;
    }
    if (entry.enumMap && Object.keys(entry.enumMap).length > 0) {
      clean.enumMap = { ...entry.enumMap };
    }
    if (entry.sysexTemplate) {
      clean.sysexTemplate = entry.sysexTemplate;
    }

    return clean;
  });
}

/**
 * Проверка каталога: уникальность id+elementIndex, обязательные rawMin/rawMax.
 * Ошибки пишутся в console.error; исключения не бросает.
 */
export function validateParamCatalog(entries: ParamCatalogEntry[]): void {
  const seen = new Map<string, number>();

  entries.forEach((entry, index) => {
    const key = paramCatalogEntryKey(entry);

    if (seen.has(key)) {
      console.error(
        `[paramCatalog] duplicate id+elementIndex "${key}" at rows ${seen.get(key)} and ${index}`,
      );
    } else {
      seen.set(key, index);
    }

    if (!Number.isFinite(entry.rawMin) || !Number.isFinite(entry.rawMax)) {
      console.error(
        `[paramCatalog] missing or invalid rawMin/rawMax for "${key}" at row ${index}`,
      );
    } else if (entry.rawMax < entry.rawMin) {
      console.error(
        `[paramCatalog] rawMax < rawMin for "${key}" at row ${index} (${entry.rawMin}…${entry.rawMax})`,
      );
    }
  });
}

/** API-ответ → каталог (для экспорта / миграции params_meta.json). */
export function paramMetaListToCatalog(params: ParamMeta[]): ParamCatalogEntry[] {
  const entries = params.map((param) => paramMetaToCatalogEntry(param));
  validateParamCatalog(entries);
  return serializeParamCatalog(entries);
}

export {
  SY99_CATALOG_PARAM_IDS,
  sy99ParamCatalogEntries,
  validateSy99CatalogCompleteness,
} from '../data/sy99ParamCatalogEntries';
export type { Sy99CatalogParamId, Sy99CatalogValidationResult } from '../data/sy99ParamCatalogEntries';

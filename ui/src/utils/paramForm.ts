import type { ParamCodecKind } from '../types/paramCatalog';
import type { ParamMeta, Sy99VerificationByElement } from '../types/paramMeta';
import { catalogCodecToApiPatch, paramMetaToCatalogEntry } from './paramCatalog';
import {
  META_RAW_INPUT_MAX,
  META_RAW_INPUT_MIN,
  formRawBoundsToStored,
  storedRawBoundsToFormFields,
} from './paramCodec';

export type ParamMetaPatch = {
  uiLabel?: string;
  group?: string;
  perElement?: boolean;
  rawMin?: number;
  rawMax?: number;
  decode?: string;
  encode?: string;
  enumNames?: string[];
  sy99Verification?: Sy99VerificationByElement;
};

/** Поля каталога params_meta.json (ParamCatalogEntry). */
export type ParamCatalogForm = {
  groupId: string;
  uiMin: number;
  uiMax: number;
  decode: ParamCodecKind;
  encode: ParamCodecKind;
  enumNames: string[];
  sysexTemplate: string;
};

/** @deprecated Используйте ParamCatalogForm */
export type ParamMetaForm = {
  uiLabel: string;
  group: string;
  perElement: boolean;
  rawMin: number;
  rawMax: number;
  enumNames: string[];
};

/** Макс. raw для enumNames в params_meta (C++ StringArray, индекс = raw). */
export const ENUM_NAMES_RAW_MAX = 127;

/** enumNames[raw] = подпись для SysEx raw; индекс = raw, не порядковый номер строки. */
export function alignEnumNamesToRawRange(
  enumNames: string[],
  rawMin: number,
  rawMax: number,
): string[] {
  const out: string[] = [];
  const lo = Math.max(0, rawMin);
  const hi = Math.min(rawMax, ENUM_NAMES_RAW_MAX);
  for (let raw = lo; raw <= hi; raw++) {
    out[raw] = enumNames[raw] ?? '';
  }
  return out;
}

export function enumNamesEditorRange(
  rawMin: number,
  rawMax: number,
): { rawMin: number; rawMax: number; capped: boolean } {
  const lo = Math.max(0, rawMin);
  const hi = Math.min(rawMax, ENUM_NAMES_RAW_MAX);
  return {
    rawMin: lo,
    rawMax: hi,
    capped: rawMax > ENUM_NAMES_RAW_MAX || rawMin < 0,
  };
}

/** Параметры Assign/Range с тем же списком MIDI CC (0…127). */
export const MIDI_CC_ENUM_COPY_TARGETS = [
  'PMASN',
  'PMRNG',
  'AMASN',
  'AMRNG',
  'FMASN',
  'FMRNG',
  'PNLASN',
  'PNLRNG',
  'COASN',
  'CORNG',
  'PNBASN',
  'PNBRNG',
  'EGBASN',
  'EGBRNG',
  'WLASN',
] as const;

export function paramToCatalogForm(param: ParamMeta): ParamCatalogForm {
  const entry = paramMetaToCatalogEntry(param);
  const { rawMin: uiMin, rawMax: uiMax } = storedRawBoundsToFormFields(
    param,
    param.rawMin,
    param.rawMax,
  );
  return {
    groupId: param.group,
    uiMin,
    uiMax,
    decode: entry.decode,
    encode: entry.encode,
    enumNames: alignEnumNamesToRawRange(param.enumNames, uiMin, uiMax),
    sysexTemplate: entry.sysexTemplate ?? '',
  };
}

/** PATCH только descriptor-полей каталога (без truth / uiLabel / perElement). */
export function catalogFormToPatch(form: ParamCatalogForm, param: ParamMeta): ParamMetaPatch {
  const stored = formRawBoundsToStored(param, form.uiMin, form.uiMax);
  return {
    group: form.groupId,
    rawMin: stored.rawMin,
    rawMax: stored.rawMax,
    enumNames: alignEnumNamesToRawRange(form.enumNames, form.uiMin, form.uiMax),
    ...catalogCodecToApiPatch(param, form.decode, form.encode),
  };
}

export function paramToForm(param: ParamMeta): ParamMetaForm {
  const catalog = paramToCatalogForm(param);
  return {
    uiLabel: param.uiLabel,
    group: catalog.groupId,
    perElement: param.perElement,
    rawMin: catalog.uiMin,
    rawMax: catalog.uiMax,
    enumNames: catalog.enumNames,
  };
}

export function formToPatch(form: ParamMetaForm, param: ParamMeta): ParamMetaPatch {
  return catalogFormToPatch(
    {
      groupId: form.group,
      uiMin: form.rawMin,
      uiMax: form.rawMax,
      decode: paramMetaToCatalogEntry(param).decode,
      encode: paramMetaToCatalogEntry(param).encode,
      enumNames: form.enumNames,
      sysexTemplate: paramMetaToCatalogEntry(param).sysexTemplate ?? '',
    },
    param,
  );
}

export function isEnumParam(param: ParamMeta | ParamMetaForm): boolean {
  return param.enumNames.length > 0;
}

/** Без NaN; ограничение по min/max (для полей Meta обычно −128…127). */
export function clampRawBound(value: number, min: number, max: number): number {
  if (!Number.isFinite(value)) {
    return min;
  }
  const n = Math.trunc(value);
  return Math.max(min, Math.min(max, n));
}

export function rawSpanCount(rawMin: number, rawMax: number): number {
  if (!Number.isFinite(rawMin) || !Number.isFinite(rawMax) || rawMax < rawMin) {
    return 0;
  }
  return rawMax - rawMin + 1;
}

export function parseRawBoundInput(
  text: string,
  fallback: number,
  min: number,
  max: number,
): number {
  const trimmed = text.trim();
  if (trimmed === '' || trimmed === '-') {
    return fallback;
  }
  return clampRawBound(Number(trimmed), min, max);
}

export { META_RAW_INPUT_MIN, META_RAW_INPUT_MAX };

/** Диапазон и подписи ELMODE из встроенного Sy99ParamRegistry (0…10). */
export const ELMODE_BUILTIN_RANGE = {
  rawMin: 0,
  rawMax: 10,
  enumNames: alignEnumNamesToRawRange(
    [
      '1 AFM MONO',
      '2 AFM MONO',
      '4 AFM MONO',
      '1 AFM POLY',
      '2 AFM POLY',
      '1 AWM POLY',
      '2 AWM POLY',
      '4 AWM POLY',
      '1 AFM & 1 AWM POLY',
      '2 AFM & 2 AWM POLY',
      'DRUM SET',
    ],
    0,
    10,
  ),
} as const;

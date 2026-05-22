import type { ParamCodecKind } from '../types/paramCatalog';
import type { ParamMeta } from '../types/paramMeta';

/** Допустимый ввод rawMin/rawMax в Meta (до конвертации в SysEx). */
export const META_RAW_INPUT_MIN = -128;
export const META_RAW_INPUT_MAX = 127;

/** Короткие подсказки для decode/encode в форме каталога. */
export const CODEC_KIND_HINTS: Record<
  ParamCodecKind,
  { label: string; lcd: string; example: string }
> = {
  identity: {
    label: 'identity',
    lcd: 'цифра на LCD = байт VV',
    example: 'Level 124, MCTUN 99',
  },
  signed: {
    label: 'signed',
    lcd: 'на LCD ±, в SysEx особая кодировка',
    example: 'Detune −7…+7, ATPBR −12…+12',
  },
  enum: {
    label: 'enum',
    lcd: 'пункт меню, не «просто число»',
    example: 'AFTMD all/top…, ELMODE, EFMODE',
  },
};

export function describeCodecPair(decode: ParamCodecKind, encode: ParamCodecKind): string {
  if (decode === encode) {
    const h = CODEC_KIND_HINTS[decode];
    return `decode и encode = ${decode}: ${h.lcd}. Пример: ${h.example}.`;
  }
  return `decode (${decode}) — читать dump/лог; encode (${encode}) — писать на SY99. Обычно ставят одинаково.`;
}

export function paramUsesUiCodec(
  param: Pick<ParamMeta, 'decode' | 'encode'>,
): boolean {
  return param.decode !== 'identity' || param.encode !== 'identity';
}

/** C++ uiFromElementDetuneSysex */
export function decodeElementDetuneSysex(sysexVal: number): number {
  let raw = sysexVal & 0x7f;
  const delta = 9;

  if (raw > delta - 2) {
    raw -= delta;
    raw = ~raw;
  }

  return Math.max(-7, Math.min(7, raw));
}

/** C++ sysexFromElementDetuneUi */
export function encodeElementDetuneUi(uiVal: number): number {
  const ui = Math.max(-7, Math.min(7, Math.trunc(uiVal)));
  if (ui >= 0) {
    return ui;
  }
  return (~ui) + 9;
}

/** C++ sysexFromElementNoteShiftUi */
export function encodeElementNoteShiftUi(uiVal: number): number {
  const ui = Math.max(-64, Math.min(63, Math.trunc(uiVal)));
  return Math.max(0, Math.min(127, ui + 64));
}

function eldtBoundsFromStored(storedMin: number, storedMax: number): { rawMin: number; rawMax: number } {
  if (storedMin < 0 || storedMax < 0) {
    return { rawMin: storedMin, rawMax: storedMax };
  }

  const lo = Math.min(storedMin, storedMax);
  const hi = Math.max(storedMin, storedMax);

  if (hi - lo > 20) {
    return { rawMin: -7, rawMax: 7 };
  }

  const uiA = decodeElementDetuneSysex(lo);
  const uiB = decodeElementDetuneSysex(hi);

  if (encodeElementDetuneUi(uiA) === lo && encodeElementDetuneUi(uiB) === hi) {
    return { rawMin: Math.min(uiA, uiB), rawMax: Math.max(uiA, uiB) };
  }

  return { rawMin: storedMin, rawMax: storedMax };
}

function eldtBoundsToStored(formMin: number, formMax: number): { rawMin: number; rawMax: number } {
  const lo = Math.min(formMin, formMax);
  const hi = Math.max(formMin, formMax);

  if (lo < 0 || hi <= 7) {
    const r0 = encodeElementDetuneUi(lo);
    const r1 = encodeElementDetuneUi(hi);
    return { rawMin: Math.min(r0, r1), rawMax: Math.max(r0, r1) };
  }

  return { rawMin: lo, rawMax: hi };
}

function elnsBoundsFromStored(storedMin: number, storedMax: number): { rawMin: number; rawMax: number } {
  const lo = Math.min(storedMin, storedMax);
  const hi = Math.max(storedMin, storedMax);

  if (lo >= 0 && hi <= 127 && hi - lo <= 127) {
    const uiA = lo - 64;
    const uiB = hi - 64;
    if (uiA >= -64 && uiB <= 63) {
      return { rawMin: Math.min(uiA, uiB), rawMax: Math.max(uiA, uiB) };
    }
  }

  return { rawMin: storedMin, rawMax: storedMax };
}

function elnsBoundsToStored(formMin: number, formMax: number): { rawMin: number; rawMax: number } {
  const lo = Math.min(formMin, formMax);
  const hi = Math.max(formMin, formMax);

  if (lo < 0 || hi <= 63) {
    const r0 = encodeElementNoteShiftUi(lo);
    const r1 = encodeElementNoteShiftUi(hi);
    return { rawMin: Math.min(r0, r1), rawMax: Math.max(r0, r1) };
  }

  return { rawMin: lo, rawMax: hi };
}

/** Показ в форме: SysEx raw из файла → UI (−7…+7 для ELDT). */
export function storedRawBoundsToFormFields(
  param: Pick<ParamMeta, 'id' | 'decode' | 'encode'>,
  storedRawMin: number,
  storedRawMax: number,
): { rawMin: number; rawMax: number } {
  if (param.id === 'ELDT' || param.decode === 'uiFromElementDetuneSysex') {
    return eldtBoundsFromStored(storedRawMin, storedRawMax);
  }

  if (param.id === 'ELNS' || param.decode === 'uiFromElementNoteShiftSysex') {
    return elnsBoundsFromStored(storedRawMin, storedRawMax);
  }

  return { rawMin: storedRawMin, rawMax: storedRawMax };
}

/** Сохранение в params_meta: UI в полях → байты SysEx для codec-параметров. */
export function formRawBoundsToStored(
  param: Pick<ParamMeta, 'id' | 'decode' | 'encode'>,
  formRawMin: number,
  formRawMax: number,
): { rawMin: number; rawMax: number; convertedFromUi: boolean } {
  if (param.id === 'ELDT' || param.decode === 'uiFromElementDetuneSysex') {
    const out = eldtBoundsToStored(formRawMin, formRawMax);
    return { ...out, convertedFromUi: formRawMin < 0 || formRawMax <= 7 };
  }

  if (param.id === 'ELNS' || param.decode === 'uiFromElementNoteShiftSysex') {
    const out = elnsBoundsToStored(formRawMin, formRawMax);
    return { ...out, convertedFromUi: formRawMin < 0 };
  }

  const lo = Math.min(formRawMin, formRawMax);
  const hi = Math.max(formRawMin, formRawMax);
  return { rawMin: lo, rawMax: hi, convertedFromUi: false };
}

export function rawBoundsFieldLabels(
  param: Pick<ParamMeta, 'decode' | 'encode'> | null,
): { min: string; max: string } {
  if (param && paramUsesUiCodec(param)) {
    return { min: 'rawMin (UI на SY99)', max: 'rawMax (UI на SY99)' };
  }
  return { min: 'rawMin (байт SysEx)', max: 'rawMax (байт SysEx)' };
}

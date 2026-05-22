import type {
  ParamMeta,
  RowOverride,
  StoredValueMapRow,
  Sy99ElementVerification,
  Sy99VerificationByElement,
} from '../types/paramMeta';
import { normalizeStoredRows } from './customValueMap';
import { remapSysexHexForElement } from './parseSysexLog';

export const VOICE_ELEMENT_INDICES = [0, 1, 2, 3] as const;

export type { RowOverride };

export type ParamVerificationState = {
  /** Параметр (element) прошёл проверку соответствия SY99 */
  confirmed: boolean;
  confirmedAt: string | null;
  /** Режим/значение с LCD SY99 сейчас (приоритет над bulk). null = не задано. */
  canonicalRaw: number | null;
  /** raw, сверенные вручную или по логу */
  verifiedRaws: number[];
  /** null = эталон API; [] или строки = свой список */
  customRows: StoredValueMapRow[] | null;
  /** Правки поверх эталона API (только в режиме эталона) */
  rowOverrides: Record<string, RowOverride>;
  /** Журнал наблюдений SysEx (для парсинга и сверки) */
  observationLog: string;
};

const STORAGE_PREFIX = 'sy99-param-verify:';

function storageKey(paramId: string, elementIndex: number): string {
  return `${STORAGE_PREFIX}${paramId}:${elementIndex}`;
}

export function dedupeVerifiedRaws(raws: number[]): number[] {
  const seen = new Set<number>();
  const out: number[] = [];

  for (const n of raws) {
    if (typeof n !== 'number' || seen.has(n)) {
      continue;
    }
    seen.add(n);
    out.push(n);
  }

  return out.sort((a, b) => a - b);
}

/** Запомнить raw с SY99/dump; журнал и таблица не трогаются. */
export function applyCanonicalRaw(
  state: ParamVerificationState,
  raw: number,
  savedAt?: string | null,
): ParamVerificationState {
  return {
    ...state,
    confirmed: true,
    confirmedAt: savedAt ?? new Date().toISOString(),
    canonicalRaw: raw,
  };
}

/** Полный сброс сверки (только «Стереть и начать заново»). */
export function toTruthOnlyPersistState(
  raw: number,
  savedAt?: string | null,
): ParamVerificationState {
  return {
    confirmed: true,
    confirmedAt: savedAt ?? new Date().toISOString(),
    canonicalRaw: raw,
    verifiedRaws: [],
    customRows: null,
    rowOverrides: {},
    observationLog: '',
  };
}

export function clearCanonicalRawOnly(state: ParamVerificationState): ParamVerificationState {
  return {
    ...state,
    confirmed: false,
    confirmedAt: null,
    canonicalRaw: null,
  };
}

export function normalizeParamVerificationState(
  state: ParamVerificationState,
): ParamVerificationState {
  const canonicalRaw =
    typeof state.canonicalRaw === 'number' && Number.isFinite(state.canonicalRaw)
      ? state.canonicalRaw
      : null;

  if (canonicalRaw !== null) {
    return applyCanonicalRaw(
      {
        ...state,
        verifiedRaws: dedupeVerifiedRaws(state.verifiedRaws),
        customRows:
          state.customRows === null ? null : normalizeStoredRows(state.customRows) ?? [],
        rowOverrides: { ...state.rowOverrides },
        observationLog: state.observationLog,
      },
      canonicalRaw,
      state.confirmedAt,
    );
  }

  return {
    ...state,
    canonicalRaw: null,
    verifiedRaws: dedupeVerifiedRaws(state.verifiedRaws),
    customRows:
      state.customRows === null ? null : normalizeStoredRows(state.customRows) ?? [],
    rowOverrides: { ...state.rowOverrides },
    observationLog: state.observationLog,
  };
}

function normalizeElementState(
  partial: Partial<Sy99ElementVerification> | undefined,
): ParamVerificationState {
  const empty: ParamVerificationState = {
    confirmed: false,
    confirmedAt: null,
    canonicalRaw: null,
    verifiedRaws: [],
    customRows: null,
    rowOverrides: {},
    observationLog: '',
  };

  if (!partial) {
    return empty;
  }

  let customRows =
    partial.customRows === null || partial.customRows === undefined
      ? null
      : normalizeStoredRows(partial.customRows) ?? [];

  if (customRows !== null && customRows.length === 0) {
    customRows = null;
  }

  const canonicalRaw =
    typeof partial.canonicalRaw === 'number' && Number.isFinite(partial.canonicalRaw)
      ? partial.canonicalRaw
      : null;

  if (canonicalRaw !== null) {
    return applyCanonicalRaw(
      {
        confirmed: Boolean(partial.confirmed),
        confirmedAt: typeof partial.confirmedAt === 'string' ? partial.confirmedAt : null,
        canonicalRaw,
        verifiedRaws: dedupeVerifiedRaws(
          Array.isArray(partial.verifiedRaws)
            ? partial.verifiedRaws.filter((n) => typeof n === 'number')
            : [],
        ),
        customRows,
        rowOverrides:
          partial.rowOverrides && typeof partial.rowOverrides === 'object'
            ? partial.rowOverrides
            : {},
        observationLog:
          typeof partial.observationLog === 'string' ? partial.observationLog : '',
      },
      canonicalRaw,
      typeof partial.confirmedAt === 'string' ? partial.confirmedAt : null,
    );
  }

  // Без canonicalRaw: галочки/строки/журнал остаются (ваша сверка), в списке — не «правда».
  return {
    confirmed: false,
    confirmedAt: null,
    canonicalRaw: null,
    verifiedRaws: dedupeVerifiedRaws(
      Array.isArray(partial.verifiedRaws)
        ? partial.verifiedRaws.filter((n) => typeof n === 'number')
        : [],
    ),
    customRows,
    rowOverrides:
      partial.rowOverrides && typeof partial.rowOverrides === 'object'
        ? partial.rowOverrides
        : {},
    observationLog:
      typeof partial.observationLog === 'string' ? partial.observationLog : '',
  };
}

export function verificationFromParam(
  param: ParamMeta | null | undefined,
  elementIndex: number,
): ParamVerificationState {
  const key = String(elementIndex);
  return normalizeElementState(param?.sy99Verification?.[key]);
}

/**
 * PATCH для API: полная замена списка правды для element.
 * Common-параметры (WPBR, WOL…): сервер заменяет весь sy99Verification.
 * per-element: заменяется только этот element, остальные element не трогаются.
 */
/** Обрезает сверку под новый rawMin…rawMax (canonical, customRows, ✓). */
export function adjustVerificationForRawRange(
  state: ParamVerificationState,
  rawMin: number,
  rawMax: number,
): ParamVerificationState {
  let canonicalRaw = state.canonicalRaw;
  let confirmed = state.confirmed;
  let confirmedAt = state.confirmedAt;

  if (canonicalRaw !== null && (canonicalRaw < rawMin || canonicalRaw > rawMax)) {
    canonicalRaw = null;
    confirmed = false;
    confirmedAt = null;
  }

  let customRows: StoredValueMapRow[] | null = state.customRows;
  if (customRows !== null) {
    const filtered = customRows.filter((row) => row.raw >= rawMin && row.raw <= rawMax);
    customRows = filtered.length > 0 ? filtered : null;
  }

  const rowOverrides: Record<string, RowOverride> = {};
  for (const [key, value] of Object.entries(state.rowOverrides)) {
    const raw = Number(key);
    if (Number.isFinite(raw) && raw >= rawMin && raw <= rawMax) {
      rowOverrides[key] = value;
    }
  }

  return normalizeParamVerificationState({
    ...state,
    confirmed,
    confirmedAt,
    canonicalRaw,
    customRows,
    verifiedRaws: dedupeVerifiedRaws(
      state.verifiedRaws.filter((raw) => raw >= rawMin && raw <= rawMax),
    ),
    rowOverrides,
  });
}

function copyCustomRowsToElement(
  rows: StoredValueMapRow[],
  targetElementIndex: number,
): StoredValueMapRow[] {
  return rows.map((row) => ({
    ...row,
    sysexHex: remapSysexHexForElement(row.sysexHex, targetElementIndex),
  }));
}

/**
 * PATCH Meta при смене диапазона у per-element: тот же rawMin/rawMax и таблица
 * (если есть) с текущего элемента — на эл. 0…3, с пересчётом b4 в SysEx.
 */
export function verificationPatchSyncRangeAllElements(
  param: ParamMeta,
  rawMin: number,
  rawMax: number,
  sourceElementIndex: number,
  sourceState: ParamVerificationState,
): { sy99Verification: Sy99VerificationByElement } {
  const sourceCustom =
    sourceState.customRows !== null && sourceState.customRows.length > 0
      ? sourceState.customRows
      : null;
  const out: Sy99VerificationByElement = {};

  for (const el of VOICE_ELEMENT_INDICES) {
    let state =
      el === sourceElementIndex
        ? sourceState
        : verificationFromParam(param, el);

    if (sourceCustom) {
      state = {
        ...state,
        customRows:
          el === sourceElementIndex
            ? sourceCustom
            : copyCustomRowsToElement(sourceCustom, el),
      };
    }

    out[String(el)] = adjustVerificationForRawRange(state, rawMin, rawMax);
  }

  return { sy99Verification: out };
}

export function verificationPatchForElement(
  elementIndex: number,
  state: ParamVerificationState,
): { sy99Verification: Sy99VerificationByElement } {
  const normalized = normalizeParamVerificationState(state);
  return {
    sy99Verification: {
      [String(elementIndex)]: normalized,
    },
  };
}

/**
 * PATCH для сохранения сверки.
 * Пустая сверка у common-параметра → `{}` (полное удаление блока на сервере).
 */
export function verificationPatchForPersist(
  param: Pick<ParamMeta, 'perElement'> | null | undefined,
  elementIndex: number,
  state: ParamVerificationState,
): { sy99Verification: Sy99VerificationByElement } {
  const normalized = normalizeParamVerificationState(state);

  if (normalized.canonicalRaw === null && !hasVerificationData(normalized) && !param?.perElement) {
    return { sy99Verification: {} };
  }

  return verificationPatchForElement(elementIndex, normalized);
}

/** Есть сохранённая правда (последнее «Сохранить правду»). */
export function elementHasSavedTruth(state: ParamVerificationState): boolean {
  return state.canonicalRaw !== null && state.confirmedAt !== null;
}

export function hasVerificationData(state: ParamVerificationState): boolean {
  return (
    state.canonicalRaw !== null ||
    (state.customRows !== null && state.customRows.length > 0) ||
    Object.keys(state.rowOverrides).length > 0 ||
    state.observationLog.trim().length > 0
  );
}

/** Однократный перенос из localStorage, если на сервере пусто. */
export function loadLegacyParamVerification(
  paramId: string,
  elementIndex: number,
): ParamVerificationState {
  const empty: ParamVerificationState = {
    confirmed: false,
    confirmedAt: null,
    canonicalRaw: null,
    verifiedRaws: [],
    customRows: null,
    rowOverrides: {},
    observationLog: '',
  };

  try {
    const raw = localStorage.getItem(storageKey(paramId, elementIndex));
    if (!raw) {
      return empty;
    }
    const parsed = JSON.parse(raw) as Partial<ParamVerificationState>;
    return normalizeElementState(parsed);
  } catch {
    return empty;
  }
}

export function clearLegacyParamVerification(paramId: string, elementIndex: number): void {
  localStorage.removeItem(storageKey(paramId, elementIndex));
}

export function emptyParamVerificationState(): ParamVerificationState {
  return {
    confirmed: false,
    confirmedAt: null,
    canonicalRaw: null,
    verifiedRaws: [],
    customRows: null,
    rowOverrides: {},
    observationLog: '',
  };
}

/** Полный сброс таблицы и журнала — пустой список, без 128 строк из API. */
export function wipedParamVerificationState(): ParamVerificationState {
  return {
    confirmed: false,
    confirmedAt: null,
    canonicalRaw: null,
    verifiedRaws: [],
    customRows: null,
    rowOverrides: {},
    observationLog: '',
  };
}

/** Статус правды для списка параметров (только canonicalRaw + confirmedAt). */
export function paramSy99VerificationSummary(
  param: ParamMeta,
): { fullyConfirmed: boolean; label: string; title: string } {
  const byElement = param.sy99Verification ?? {};

  if (!param.perElement) {
    const state = normalizeElementState(byElement['0']);
    if (elementHasSavedTruth(state)) {
      const when = state.confirmedAt ? new Date(state.confirmedAt).toLocaleString() : '';
      return {
        fullyConfirmed: true,
        label: `raw ${state.canonicalRaw}`,
        title: when
          ? `Правда: raw ${state.canonicalRaw}, сохранено ${when}`
          : `Правда: raw ${state.canonicalRaw}`,
      };
    }
    return { fullyConfirmed: false, label: '—', title: 'Правда не задана' };
  }

  const indices = [0, 1, 2, 3];
  const withTruth = indices.filter((el) =>
    elementHasSavedTruth(normalizeElementState(byElement[String(el)])),
  );
  const label = `${withTruth.length}/4`;
  const title =
    withTruth.length === 0
      ? 'Правда не задана ни для одного элемента'
      : `Правда задана для элементов: ${withTruth.join(', ')} (raw: ${withTruth
          .map((el) => {
            const s = normalizeElementState(byElement[String(el)]);
            return `${el}→${s.canonicalRaw}`;
          })
          .join(', ')})`;

  return {
    fullyConfirmed: withTruth.length === 4,
    label,
    title,
  };
}

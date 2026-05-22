import type { ParamMeta, StoredValueMapRow } from '../types/paramMeta';
import type { DisplayValueMapRow, ParamValueMap, ParamValueMapRow, PresentationKind } from '../types/paramValueMap';
import type { ParamVerificationState } from './paramVerification';
import {
  dedupeLogText,
  findLogRawConflicts,
  logLineMatchesParamAddress,
  parseSysexLogText,
  type ParsedLogLine,
} from './parseSysexLog';

export type ImportFromLogResult = {
  rows: StoredValueMapRow[];
  dedupedLog: string;
  removedDuplicateLines: number;
  skippedWrongAddress: number;
  rawConflicts: ReturnType<typeof findLogRawConflicts>;
};

/** Пустой массив [] в файле = «списка нет», показываем журнал / Meta. */
export function effectiveCustomRows(
  state: ParamVerificationState,
): StoredValueMapRow[] | null {
  if (state.customRows === null || state.customRows.length === 0) {
    return null;
  }
  return state.customRows;
}

export function isCustomValueMap(state: ParamVerificationState): boolean {
  return effectiveCustomRows(state) !== null;
}

/** Больше этого — не копируем всю таблицу Meta в customRows без явного действия. */
export const META_FULL_TABLE_THRESHOLD = 24;

export type ValueMapTableMode = 'custom' | 'log' | 'verified' | 'meta-full';

export type ValueMapTableContext = {
  param: Pick<ParamMeta, 'id' | 'addr' | 'perElement'>;
  elementIndex: number;
  valueMap: ParamValueMap;
};

function storedToParamRows(rows: StoredValueMapRow[]): ParamValueMapRow[] {
  return rows.map((row, index) => ({
    index,
    raw: row.raw,
    ui: row.ui,
    sysexHex: row.sysexHex,
    programLabel: row.programLabel,
    presentationKind: (row.presentationKind ?? 'numeric') as PresentationKind,
    registryLabel: row.registryLabel ?? null,
    labelMismatch: false,
  }));
}

/** Какие строки показывать: сохранённый список → журнал → ✓ сверено → иначе полный Meta. */
export function rowsForValueMapTable(
  valueMap: ParamValueMap,
  verification: ParamVerificationState,
  ctx: ValueMapTableContext,
): { rows: ParamValueMapRow[]; mode: ValueMapTableMode } {
  const custom = effectiveCustomRows(verification);
  if (custom !== null) {
    return { rows: storedToParamRows(custom), mode: 'custom' };
  }

  const log = verification.observationLog.trim();
  if (log) {
    const imported = importRowsFromObservationLog(log, {
      param: ctx.param,
      elementIndex: ctx.elementIndex,
      valueMap: ctx.valueMap,
    });
    if (imported.rows.length > 0) {
      return { rows: storedToParamRows(imported.rows), mode: 'log' };
    }
  }

  if (verification.verifiedRaws.length > 0) {
    const want = new Set(verification.verifiedRaws);
    const rows = valueMap.rows.filter((row) => want.has(row.raw));
    if (rows.length > 0) {
      return { rows, mode: 'verified' };
    }
  }

  return { rows: valueMap.rows, mode: 'meta-full' };
}

export function baseRowsFromVerification(
  valueMap: ParamValueMap,
  verification: ParamVerificationState,
  ctx?: ValueMapTableContext,
): ParamValueMapRow[] {
  if (ctx) {
    return rowsForValueMapTable(valueMap, verification, ctx).rows;
  }

  const custom = effectiveCustomRows(verification);
  if (custom === null) {
    return valueMap.rows;
  }

  return storedToParamRows(custom);
}

/** Перед «Сохранить»: короткий список из журнала / ✓ / выбранных — не все 128 из Meta. */
export function verificationForPersist(
  state: ParamVerificationState,
  displayRows: DisplayValueMapRow[],
  ctx: ValueMapTableContext & { selectedRaws?: ReadonlySet<number> },
): ParamVerificationState {
  if (effectiveCustomRows(state) !== null) {
    return state;
  }

  const log = state.observationLog.trim();
  if (log) {
    const imported = importRowsFromObservationLog(log, {
      param: ctx.param,
      elementIndex: ctx.elementIndex,
      valueMap: ctx.valueMap,
    });
    if (imported.rows.length > 0) {
      return {
        ...state,
        customRows: imported.rows,
        rowOverrides: {},
        observationLog: imported.dedupedLog,
      };
    }
  }

  if (state.verifiedRaws.length > 0) {
    const want = new Set(state.verifiedRaws);
    const rows = displayRowsToStored(displayRows.filter((row) => want.has(row.raw)));
    if (rows.length > 0) {
      return { ...state, customRows: rows, rowOverrides: {} };
    }
  }

  if (ctx.selectedRaws && ctx.selectedRaws.size > 0) {
    const rows = displayRowsToStored(
      displayRows.filter((row) => ctx.selectedRaws!.has(row.raw)),
    );
    if (rows.length > 0) {
      return { ...state, customRows: rows, rowOverrides: {} };
    }
  }

  if (displayRows.length > 0 && displayRows.length <= META_FULL_TABLE_THRESHOLD) {
    return { ...state, customRows: displayRowsToStored(displayRows), rowOverrides: {} };
  }

  return state;
}

export function displayRowsToStored(rows: DisplayValueMapRow[]): StoredValueMapRow[] {
  return rows.map(
    ({ index: _index, isEdited: _e, isVerified: _v, labelMismatch: _m, ...row }) => ({
      raw: row.raw,
      ui: row.ui,
      sysexHex: row.sysexHex,
      programLabel: row.programLabel,
      presentationKind: row.presentationKind,
      registryLabel: row.registryLabel,
    }),
  );
}

export function normalizeStoredRow(partial: Partial<StoredValueMapRow>): StoredValueMapRow | null {
  if (typeof partial.raw !== 'number' || typeof partial.ui !== 'number') {
    return null;
  }
  if (typeof partial.sysexHex !== 'string' || typeof partial.programLabel !== 'string') {
    return null;
  }

  return {
    raw: partial.raw,
    ui: partial.ui,
    sysexHex: partial.sysexHex,
    programLabel: partial.programLabel,
    presentationKind: partial.presentationKind,
    registryLabel: partial.registryLabel ?? null,
  };
}

export function normalizeStoredRows(rows: unknown): StoredValueMapRow[] | null {
  if (rows === null || rows === undefined) {
    return null;
  }
  if (!Array.isArray(rows)) {
    return null;
  }

  const out: StoredValueMapRow[] = [];
  for (const item of rows) {
    if (item && typeof item === 'object') {
      const normalized = normalizeStoredRow(item as Partial<StoredValueMapRow>);
      if (normalized) {
        out.push(normalized);
      }
    }
  }
  return out;
}

export function createBlankStoredRow(
  valueMap: ParamValueMap | null,
  existing: StoredValueMapRow[],
): StoredValueMapRow {
  const usedRaws = new Set(existing.map((r) => r.raw));
  let raw = 0;
  while (usedRaws.has(raw) && raw < 127) {
    raw += 1;
  }

  const template =
    existing[existing.length - 1]?.sysexHex ??
    valueMap?.rows[0]?.sysexHex ??
    'F0 43 10 34 00 00 00 00 00 00 F7';

  return {
    raw,
    ui: raw,
    sysexHex: template,
    programLabel: String(raw),
    presentationKind: 'numeric',
    registryLabel: null,
  };
}

export function updateStoredRow(
  rows: StoredValueMapRow[],
  raw: number,
  patch: Partial<StoredValueMapRow>,
): StoredValueMapRow[] {
  return rows.map((row) => (row.raw === raw ? { ...row, ...patch } : row));
}

/** Журнал наблюдений из сохранённой таблицы (истина после «Подтвердить»). */
export function observationLogFromStoredRows(rows: StoredValueMapRow[]): string {
  return rows
    .map((row, index) => {
      const label = row.programLabel.trim();
      const labelSuffix =
        label.length > 0 && label !== String(row.raw) ? ` ${label}` : '';
      return `[#${index + 1}] SysEx: ${row.sysexHex}${labelSuffix}`;
    })
    .join('\n');
}

/** Импорт таблицы из журнала: dedupe, фильтр по addr, первое вхождение raw. */
export function importRowsFromObservationLog(
  logText: string,
  options: {
    param: Pick<ParamMeta, 'id' | 'addr' | 'perElement'>;
    elementIndex: number;
    valueMap: ParamValueMap | null;
  },
): ImportFromLogResult {
  const { text: dedupedLog, removedCount: removedDuplicateLines } = dedupeLogText(logText);
  const lines = parseSysexLogText(dedupedLog);
  const templateByRaw = new Map<number, ParamValueMapRow>();

  if (options.valueMap) {
    for (const row of options.valueMap.rows) {
      templateByRaw.set(row.raw, row);
    }
  }

  let skippedWrongAddress = 0;
  const relevant: ParsedLogLine[] = [];

  for (const line of lines) {
    if (
      !logLineMatchesParamAddress(
        line,
        options.param.addr,
        options.elementIndex,
        options.param.perElement,
      )
    ) {
      skippedWrongAddress += 1;
      continue;
    }
    relevant.push(line);
  }

  const rawConflicts = findLogRawConflicts(relevant);
  const byRaw = new Map<number, StoredValueMapRow>();

  for (const line of relevant) {
    if (byRaw.has(line.raw)) {
      continue;
    }
    byRaw.set(line.raw, logLineToStoredRow(line, templateByRaw.get(line.raw)));
  }

  return {
    rows: [...byRaw.values()].sort((a, b) => a.raw - b.raw),
    dedupedLog,
    removedDuplicateLines,
    skippedWrongAddress,
    rawConflicts,
  };
}

/** @deprecated Используйте importRowsFromObservationLog */
export function storedRowsFromObservationLog(logText: string): StoredValueMapRow[] {
  const lines = parseSysexLogText(logText);
  const byRaw = new Map<number, StoredValueMapRow>();

  for (const line of lines) {
    if (!byRaw.has(line.raw)) {
      byRaw.set(line.raw, logLineToStoredRow(line));
    }
  }

  return [...byRaw.values()].sort((a, b) => a.raw - b.raw);
}

function logLineToStoredRow(
  line: ParsedLogLine,
  template?: ParamValueMapRow,
): StoredValueMapRow {
  const label = line.trailingLabel?.trim();
  const programLabel =
    label && label.length > 0 ? label : (template?.programLabel ?? String(line.raw));

  return {
    raw: line.raw,
    ui: template?.ui ?? line.raw,
    sysexHex: line.sysexHex,
    programLabel,
    presentationKind: template?.presentationKind ?? 'numeric',
    registryLabel: template?.registryLabel ?? null,
  };
}

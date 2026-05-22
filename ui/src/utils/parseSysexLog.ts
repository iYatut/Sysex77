import type { Addr } from '../types/paramMeta';
import type { LogMatchStatus, LogRowOverlay, ParamValueMap, ParamValueMapRow } from '../types/paramValueMap';

export type ParsedLogLine = {
  lineIndex: number;
  logIndex?: number;
  sysexHex: string;
  raw: number;
  trailingLabel?: string;
};

function normalizeSysexHex(hex: string): string {
  return hex
    .trim()
    .replace(/\s+/g, ' ')
    .toUpperCase();
}

function rawFromSysexBytes(bytes: number[]): number | null {
  if (bytes.length < 2) {
    return null;
  }

  const f0Index = bytes[0] === 0xf0 ? 0 : -1;
  const dataStart = f0Index >= 0 ? 1 : 0;
  const data = f0Index >= 0 && bytes[bytes.length - 1] === 0xf7 ? bytes.slice(dataStart, -1) : bytes.slice(dataStart);

  if (data.length >= 9) {
    return data[8] & 0x7f;
  }

  if (data.length >= 1) {
    return data[data.length - 1] & 0x7f;
  }

  return null;
}

export function sysexHexToBytes(sysexHex: string): number[] {
  const tokens = normalizeSysexHex(sysexHex).split(/\s+/).filter(Boolean);
  const bytes: number[] = [];
  for (const token of tokens) {
    const byte = parseHexToken(token);
    if (byte !== null) {
      bytes.push(byte);
    }
  }
  return bytes;
}

/** Payload `43 … 34 …` без обёртки F0/F7. */
export function sysexParamChangeDataBytes(sysexHex: string): number[] {
  const bytes = sysexHexToBytes(sysexHex);
  if (bytes.length === 0) {
    return bytes;
  }

  const f0Index = bytes[0] === 0xf0 ? 0 : -1;
  const dataStart = f0Index >= 0 ? 1 : 0;
  const dataEnd =
    f0Index >= 0 && bytes[bytes.length - 1] === 0xf7 ? bytes.length - 1 : bytes.length;

  return bytes.slice(dataStart, dataEnd);
}

/** Для per-element: b4 = elementIndex × 0x20 в кадре `F0 43 10 34 …`. */
export function remapSysexHexForElement(sysexHex: string, elementIndex: number): string {
  const bytes = sysexHexToBytes(sysexHex);
  const data = sysexParamChangeDataBytes(sysexHex);
  if (bytes.length === 0 || data.length < 5 || data[0] !== 0x43 || data[2] !== 0x34) {
    return sysexHex;
  }

  const expectedB4 = (elementIndex & 0x7f) * 0x20;
  const out = [...bytes];
  const b4Index = bytes[0] === 0xf0 ? 5 : 4;
  if (b4Index < out.length) {
    out[b4Index] = expectedB4;
  }

  return out.map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
}

/** Кадр `43 … 34 b3 b4 b5 b6 … VV` относится к параметру и element. */
export function logLineMatchesParamAddress(
  line: ParsedLogLine,
  addr: Addr,
  elementIndex: number,
  perElement: boolean,
): boolean {
  const bytes = sysexParamChangeDataBytes(line.sysexHex);
  if (bytes.length < 9 || bytes[0] !== 0x43 || bytes[2] !== 0x34) {
    return false;
  }

  if (bytes[3] !== addr.b3 || bytes[5] !== addr.b5 || bytes[6] !== addr.b6) {
    return false;
  }

  if (perElement) {
    const expectedB4 = (elementIndex & 0x7f) * 0x20;
    return bytes[4] === expectedB4;
  }

  return bytes[4] === addr.b4;
}

function parseHexToken(token: string): number | null {
  const cleaned = token.replace(/[^0-9A-Fa-f]/g, '');
  if (!cleaned) {
    return null;
  }
  const value = Number.parseInt(cleaned, 16);
  return Number.isNaN(value) ? null : value;
}

export function parseSysexLogLine(line: string, lineIndex: number): ParsedLogLine | null {
  const trimmed = line.trim();
  if (!trimmed || trimmed.startsWith('#')) {
    return null;
  }

  const indexMatch = /^\[#(\d+)\]/i.exec(trimmed);
  const logIndex = indexMatch ? Number.parseInt(indexMatch[1], 10) : undefined;

  const sysexMarker = trimmed.search(/SysEx\s*:/i);
  if (sysexMarker < 0) {
    return null;
  }

  const afterMarker = trimmed.slice(sysexMarker).replace(/^SysEx\s*:/i, '').trim();
  const f0Pos = afterMarker.search(/\bF0\b/i);
  const payload = f0Pos >= 0 ? afterMarker.slice(f0Pos) : afterMarker;

  const f7Pos = payload.search(/\bF7\b/i);
  const hexPart = f7Pos >= 0 ? payload.slice(0, f7Pos + 2) : payload;
  const tailPart = f7Pos >= 0 ? payload.slice(f7Pos + 2).trim() : '';

  const tokens = hexPart.split(/\s+/).filter(Boolean);
  const bytes: number[] = [];

  for (const token of tokens) {
    const byte = parseHexToken(token);
    if (byte === null) {
      continue;
    }
    bytes.push(byte);
  }

  if (bytes.length === 0) {
    return null;
  }

  const sysexHex = normalizeSysexHex(
    bytes.map((b) => b.toString(16).toUpperCase().padStart(2, '0')).join(' '),
  );

  let raw = rawFromSysexBytes(bytes);
  let trailingLabel: string | undefined;

  if (tailPart) {
    trailingLabel = tailPart;
    // Число в хвосте — подпись/ui, не подменяем raw из байта SysEx (иначе импорт ≠ кадр).
    if (raw === null) {
      const tailNumbers = tailPart.match(/-?\d+/g);
      if (tailNumbers && tailNumbers.length > 0) {
        const last = Number.parseInt(tailNumbers[tailNumbers.length - 1], 10);
        if (!Number.isNaN(last)) {
          raw = last;
        }
      }
    }
  }

  if (raw === null) {
    return null;
  }

  return { lineIndex, logIndex, sysexHex, raw, trailingLabel };
}

export type LogDuplicateReason = 'duplicateSysex' | 'duplicateLogIndex';

export type LogRawConflict = {
  raw: number;
  keptLineIndex: number;
  droppedLineIndex: number;
  keptSysexHex: string;
  droppedSysexHex: string;
};

export type LogDuplicateEntry = {
  line: ParsedLogLine;
  reason: LogDuplicateReason;
  firstSeenLineIndex: number;
};

/** Один raw — разные SysEx (подмена при импорте «последняя строка победила»). */
export function findLogRawConflicts(logLines: ParsedLogLine[]): LogRawConflict[] {
  const byRaw = new Map<number, ParsedLogLine>();
  const conflicts: LogRawConflict[] = [];

  for (const line of logLines) {
    const prev = byRaw.get(line.raw);
    if (prev === undefined) {
      byRaw.set(line.raw, line);
      continue;
    }
    if (prev.sysexHex === line.sysexHex) {
      continue;
    }
    conflicts.push({
      raw: line.raw,
      keptLineIndex: prev.lineIndex,
      droppedLineIndex: line.lineIndex,
      keptSysexHex: prev.sysexHex,
      droppedSysexHex: line.sysexHex,
    });
  }

  return conflicts;
}

/** Повторы в журнале (двойной щелчок, повтор [#N] или тот же SysEx). */
export function findLogDuplicates(logLines: ParsedLogLine[]): LogDuplicateEntry[] {
  const duplicates: LogDuplicateEntry[] = [];
  const seenSysex = new Map<string, number>();
  const seenLogIndex = new Map<number, number>();

  for (const line of logLines) {
    if (line.logIndex !== undefined) {
      const first = seenLogIndex.get(line.logIndex);
      if (first !== undefined) {
        duplicates.push({
          line,
          reason: 'duplicateLogIndex',
          firstSeenLineIndex: first,
        });
      } else {
        seenLogIndex.set(line.logIndex, line.lineIndex);
      }
    }

    const firstHex = seenSysex.get(line.sysexHex);
    if (firstHex !== undefined) {
      duplicates.push({
        line,
        reason: 'duplicateSysex',
        firstSeenLineIndex: firstHex,
      });
    } else {
      seenSysex.set(line.sysexHex, line.lineIndex);
    }
  }

  return duplicates;
}

export function duplicateReasonLabel(reason: LogDuplicateReason): string {
  switch (reason) {
    case 'duplicateLogIndex':
      return 'повтор [#N]';
    case 'duplicateSysex':
      return 'тот же SysEx';
    default:
      return reason;
  }
}

/** Оставляет в тексте журнала только первое вхождение каждого кадра SysEx. */
export function dedupeLogText(text: string): { text: string; removedCount: number } {
  const lines = text.split(/\r?\n/);
  const kept: string[] = [];
  const seenSysex = new Set<string>();
  let removedCount = 0;

  for (let i = 0; i < lines.length; i += 1) {
    const line = lines[i];
    const parsed = parseSysexLogLine(line, i);

    if (!parsed) {
      if (line.trim().length > 0) {
        kept.push(line);
      }
      continue;
    }

    if (seenSysex.has(parsed.sysexHex)) {
      removedCount += 1;
      continue;
    }

    seenSysex.add(parsed.sysexHex);
    kept.push(line);
  }

  return {
    text: kept.join('\n'),
    removedCount,
  };
}

export function parseSysexLogText(text: string): ParsedLogLine[] {
  const lines = text.split(/\r?\n/);
  const parsed: ParsedLogLine[] = [];

  for (let i = 0; i < lines.length; i += 1) {
    const row = parseSysexLogLine(lines[i], i);
    if (row) {
      parsed.push(row);
    }
  }

  return parsed;
}

export function compareLogWithValueMap(
  logLines: ParsedLogLine[],
  valueMap: ParamValueMap,
): {
  overlaysByRaw: Map<number, LogRowOverlay>;
  extras: ParsedLogLine[];
} {
  const rowByRaw = new Map<number, ParamValueMapRow>();
  for (const row of valueMap.rows) {
    rowByRaw.set(row.raw, row);
  }

  const overlaysByRaw = new Map<number, LogRowOverlay>();
  const seenRaw = new Set<number>();
  const extras: ParsedLogLine[] = [];

  for (const logLine of logLines) {
    const mapRow = rowByRaw.get(logLine.raw);

    if (!mapRow) {
      extras.push(logLine);
      continue;
    }

    const normalizedLogHex = normalizeSysexHex(logLine.sysexHex);
    const normalizedMapHex = normalizeSysexHex(mapRow.sysexHex);

    let status: LogMatchStatus = 'match';

    if (seenRaw.has(logLine.raw)) {
      status = 'duplicateInLog';
    } else if (normalizedLogHex !== normalizedMapHex) {
      status = 'sysexMismatch';
    } else if (
      logLine.trailingLabel !== undefined &&
      logLine.trailingLabel.trim() !== mapRow.programLabel
    ) {
      status = 'labelMismatch';
    }

    seenRaw.add(logLine.raw);

    const existing = overlaysByRaw.get(logLine.raw);
    if (existing?.status === 'duplicateInLog' || status === 'duplicateInLog') {
      overlaysByRaw.set(logLine.raw, {
        logIndex: logLine.logIndex,
        logLabel: logLine.trailingLabel ?? String(logLine.raw),
        logSysexHex: normalizedLogHex,
        status: 'duplicateInLog',
      });
      continue;
    }

    overlaysByRaw.set(logLine.raw, {
      logIndex: logLine.logIndex,
      logLabel: logLine.trailingLabel ?? String(logLine.raw),
      logSysexHex: normalizedLogHex,
      status,
    });
  }

  return { overlaysByRaw, extras };
}

export function logMatchStatusLabel(status: LogMatchStatus): string {
  switch (status) {
    case 'match':
      return 'OK';
    case 'sysexMismatch':
      return 'SysEx ≠';
    case 'labelMismatch':
      return 'метка ≠';
    case 'duplicateInLog':
      return 'дубликат';
    case 'extraInLog':
      return 'лишняя';
    default:
      return status;
  }
}

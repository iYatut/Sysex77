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
    const tailNumbers = tailPart.match(/-?\d+/g);
    if (tailNumbers && tailNumbers.length > 0) {
      const last = Number.parseInt(tailNumbers[tailNumbers.length - 1], 10);
      if (!Number.isNaN(last)) {
        raw = last;
        trailingLabel = tailPart;
      }
    }
  }

  if (raw === null) {
    return null;
  }

  return { lineIndex, logIndex, sysexHex, raw, trailingLabel };
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

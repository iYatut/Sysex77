/**
 * Regression: import from log — filter by addr, dedupe, no last-wins substitution.
 * Run: node scripts/test-log-import.mjs
 */
import assert from 'node:assert/strict';

const EFMODE = {
  id: 'EFMODE',
  perElement: false,
  addr: { b3: 0x08, b4: 0x00, b5: 0x00, b6: 0x20, perElement: false },
};

const log = [
  '[#1] SysEx: F0 43 10 34 08 00 00 20 00 00 F7 Off',
  '[#2] SysEx: F0 43 10 34 08 00 00 20 00 01 F7 Serial',
  '[#3] SysEx: F0 43 10 34 08 00 00 20 00 01 F7',
  '[#4] SysEx: F0 43 10 34 02 00 00 2B 00 7E F7',
  '[#5] SysEx: F0 43 10 34 08 00 00 20 00 02 F7 Parallel',
].join('\n');

function normalizeSysexHex(hex) {
  return hex.trim().replace(/\s+/g, ' ').toUpperCase();
}

function parseHexToken(token) {
  const cleaned = token.replace(/[^0-9A-Fa-f]/g, '');
  if (!cleaned) return null;
  const value = Number.parseInt(cleaned, 16);
  return Number.isNaN(value) ? null : value;
}

function sysexHexToBytes(sysexHex) {
  return normalizeSysexHex(sysexHex)
    .split(/\s+/)
    .filter(Boolean)
    .map((t) => parseHexToken(t))
    .filter((b) => b !== null);
}

function sysexParamChangeDataBytes(sysexHex) {
  const bytes = sysexHexToBytes(sysexHex);
  if (bytes.length === 0) return bytes;
  const dataStart = bytes[0] === 0xf0 ? 1 : 0;
  const dataEnd =
    bytes[0] === 0xf0 && bytes[bytes.length - 1] === 0xf7 ? bytes.length - 1 : bytes.length;
  return bytes.slice(dataStart, dataEnd);
}

function logLineMatchesParamAddress(line, addr, elementIndex, perElement) {
  const bytes = sysexParamChangeDataBytes(line.sysexHex);
  if (bytes.length < 9 || bytes[0] !== 0x43 || bytes[2] !== 0x34) return false;
  if (bytes[3] !== addr.b3 || bytes[5] !== addr.b5 || bytes[6] !== addr.b6) return false;
  if (perElement) return bytes[4] === (elementIndex & 0x7f) * 0x20;
  return bytes[4] === addr.b4;
}

function parseSysexLogLine(line, lineIndex) {
  const trimmed = line.trim();
  if (!trimmed) return null;
  const sysexMarker = trimmed.search(/SysEx\s*:/i);
  if (sysexMarker < 0) return null;
  const afterMarker = trimmed.slice(sysexMarker).replace(/^SysEx\s*:/i, '').trim();
  const f0Pos = afterMarker.search(/\bF0\b/i);
  const payload = f0Pos >= 0 ? afterMarker.slice(f0Pos) : afterMarker;
  const f7Pos = payload.search(/\bF7\b/i);
  const hexPart = f7Pos >= 0 ? payload.slice(0, f7Pos + 2) : payload;
  const tailPart = f7Pos >= 0 ? payload.slice(f7Pos + 2).trim() : '';
  const bytes = hexPart.split(/\s+/).filter(Boolean).map(parseHexToken).filter((b) => b !== null);
  if (bytes.length === 0) return null;
  const sysexHex = normalizeSysexHex(bytes.map((b) => b.toString(16).toUpperCase().padStart(2, '0')).join(' '));
  const raw = rawFromSysexBytes(bytes);
  if (raw === null) return null;
  return { lineIndex, sysexHex, raw, trailingLabel: tailPart || undefined };
}

function rawFromSysexBytes(bytes) {
  if (bytes.length < 2) return null;
  const dataStart = bytes[0] === 0xf0 ? 1 : 0;
  const dataEnd = bytes[0] === 0xf0 && bytes[bytes.length - 1] === 0xf7 ? bytes.length - 1 : bytes.length;
  const data = bytes.slice(dataStart, dataEnd);
  if (data.length >= 9) return data[8] & 0x7f;
  if (data.length >= 1) return data[data.length - 1] & 0x7f;
  return null;
}

function dedupeLogText(text) {
  const lines = text.split(/\r?\n/);
  const kept = [];
  const seenSysex = new Set();
  let removedCount = 0;
  for (let i = 0; i < lines.length; i += 1) {
    const parsed = parseSysexLogLine(lines[i], i);
    if (!parsed) {
      if (lines[i].trim()) kept.push(lines[i]);
      continue;
    }
    if (seenSysex.has(parsed.sysexHex)) {
      removedCount += 1;
      continue;
    }
    seenSysex.add(parsed.sysexHex);
    kept.push(lines[i]);
  }
  return { text: kept.join('\n'), removedCount };
}

function importRowsFromObservationLog(logText, options) {
  const { text: dedupedLog, removedCount: removedDuplicateLines } = dedupeLogText(logText);
  const lines = dedupedLog.split(/\r?\n/).map((l, i) => parseSysexLogLine(l, i)).filter(Boolean);
  let skippedWrongAddress = 0;
  const relevant = [];
  for (const line of lines) {
    if (!logLineMatchesParamAddress(line, options.param.addr, options.elementIndex, options.param.perElement)) {
      skippedWrongAddress += 1;
      continue;
    }
    relevant.push(line);
  }
  const byRaw = new Map();
  for (const line of relevant) {
    if (!byRaw.has(line.raw)) {
      byRaw.set(line.raw, { raw: line.raw, sysexHex: line.sysexHex });
    }
  }
  return {
    rows: [...byRaw.values()].sort((a, b) => a.raw - b.raw),
    dedupedLog,
    removedDuplicateLines,
    skippedWrongAddress,
  };
}

const result = importRowsFromObservationLog(log, { param: EFMODE, elementIndex: 0, valueMap: null });

assert.equal(result.rows.length, 3, 'expect 3 unique EFMODE raws');
assert.deepEqual(
  result.rows.map((r) => r.raw),
  [0, 1, 2],
);
assert.equal(result.rows[0].sysexHex, 'F0 43 10 34 08 00 00 20 00 00 F7');
assert.equal(result.removedDuplicateLines, 1, 'duplicate Serial sysex removed');
assert.equal(result.skippedWrongAddress, 1, 'PMRNG line skipped');

console.log('test-log-import.mjs: PASS');

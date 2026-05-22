#!/usr/bin/env node
/**
 * Проверка params_meta.json как чистого каталога (ParamCatalogEntry).
 * Usage: node scripts/check-params-meta-duplicates.mjs [path/to/params_meta.json]
 */
import { readFileSync } from 'node:fs';

const path = process.argv[2] ?? process.env.PARAMS_META_JSON;
if (!path) {
  console.error('Укажите путь: node scripts/check-params-meta-duplicates.mjs <params_meta.json>');
  process.exit(1);
}

const TRUTH_KEYS = new Set([
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
]);

const SY99_CATALOG_PARAM_IDS = [
  'ELMODE', 'WOL', 'WPBR', 'ATPBR', 'PMASN', 'PMRNG', 'AMASN', 'AMRNG', 'FMASN', 'FMRNG',
  'PNLASN', 'PNLRNG', 'COASN', 'CORNG', 'PNBASN', 'PNBRNG', 'EGBASN', 'EGBRNG', 'WLASN',
  'WLLML', 'MCTUN', 'RNDP', 'AFTMD', 'SPTPNT', 'ELVL', 'ELDT', 'ELNS', 'ENLL', 'ENLH',
  'EVLL', 'EVLH', 'OUTSEL', 'EFLN1EL', 'EFSDLV', 'EFSDVSNS', 'EFSDSCL', 'EFMODE',
];

function catalogKey(entry) {
  return entry.elementIndex === undefined ? entry.id : `${entry.id}:${entry.elementIndex}`;
}

function stripTruth(row) {
  const out = {};
  for (const [key, value] of Object.entries(row)) {
    if (!TRUTH_KEYS.has(key)) {
      out[key] = value;
    }
  }
  return out;
}

function legacyToCatalog(row) {
  const stripped = stripTruth(row);
  if (typeof stripped.groupId !== 'string' && typeof stripped.group === 'string') {
    stripped.groupId = stripped.group;
  }
  if (stripped.uiMin === undefined) {
    stripped.uiMin = stripped.rawMin ?? 0;
  }
  if (stripped.uiMax === undefined) {
    stripped.uiMax = stripped.rawMax ?? 127;
  }
  if (stripped.decode === undefined) {
    stripped.decode = 'identity';
  }
  if (stripped.encode === undefined) {
    stripped.encode = stripped.decode;
  }
  return stripped;
}

const raw = JSON.parse(readFileSync(path, 'utf8'));
if (!Array.isArray(raw)) {
  console.error('Ожидается JSON-массив');
  process.exit(1);
}

const seen = new Map();
let dupKeys = 0;
let invalidBounds = 0;
let truthFieldRows = 0;

for (let i = 0; i < raw.length; i += 1) {
  const row = raw[i];
  if (!row || typeof row !== 'object') {
    continue;
  }

  const hasTruth = Object.keys(row).some((key) => TRUTH_KEYS.has(key));
  if (hasTruth) {
    truthFieldRows += 1;
  }

  const entry = legacyToCatalog(row);
  const id = entry.id ?? entry.tag;
  if (!id) {
    continue;
  }
  entry.id = id;

  const key = catalogKey(entry);
  if (seen.has(key)) {
    dupKeys += 1;
    console.log(`Дубликат id+elementIndex "${key}": строки ${seen.get(key)} и ${i}`);
  } else {
    seen.set(key, i);
  }

  if (
    typeof entry.rawMin !== 'number' ||
    typeof entry.rawMax !== 'number' ||
    !Number.isFinite(entry.rawMin) ||
    !Number.isFinite(entry.rawMax)
  ) {
    invalidBounds += 1;
    console.log(`  строка ${i} (${key}): нет rawMin/rawMax`);
  } else if (entry.rawMax < entry.rawMin) {
    invalidBounds += 1;
    console.log(`  строка ${i} (${key}): rawMax < rawMin (${entry.rawMin}…${entry.rawMax})`);
  }
}

console.log(`\nЗаписей: ${raw.length}, с truth-полями (legacy): ${truthFieldRows}`);
console.log(`Дубликатов id+elementIndex: ${dupKeys}, некорректных rawMin/rawMax: ${invalidBounds}`);

const seenIds = new Set([...seen.keys()].map((key) => key.split(':')[0]));
const missingIds = SY99_CATALOG_PARAM_IDS.filter((id) => !seenIds.has(id));
const extraIds = [...seenIds].filter((id) => !SY99_CATALOG_PARAM_IDS.includes(id)).sort();

if (missingIds.length) {
  console.log(`\nОтсутствуют id из Sy99ParamRegistry (${missingIds.length}): ${missingIds.join(', ')}`);
}
if (extraIds.length) {
  console.log(`\nЛишние id (${extraIds.length}): ${extraIds.join(', ')}`);
}

const catalogOk = dupKeys + invalidBounds === 0 && missingIds.length === 0 && extraIds.length === 0;
process.exit(catalogOk ? 0 : 1);

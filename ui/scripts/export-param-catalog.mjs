#!/usr/bin/env node
/**
 * Экспорт ui/fixtures/paramCatalog.json из канонических specs.
 * Usage: node scripts/export-param-catalog.mjs
 */
import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const root = join(__dirname, '..');
const specsPath = join(root, 'fixtures', 'sy99CatalogSpecs.json');
const outPath = join(root, 'fixtures', 'paramCatalog.json');

const SY99_CATALOG_PARAM_IDS = [
  'ELMODE', 'WOL', 'WPBR', 'ATPBR', 'PMASN', 'PMRNG', 'AMASN', 'AMRNG', 'FMASN', 'FMRNG',
  'PNLASN', 'PNLRNG', 'COASN', 'CORNG', 'PNBASN', 'PNBRNG', 'EGBASN', 'EGBRNG', 'WLASN',
  'WLLML', 'MCTUN', 'RNDP', 'AFTMD', 'SPTPNT', 'ELVL', 'ELDT', 'ELNS', 'ENLL', 'ENLH',
  'EVLL', 'EVLH', 'OUTSEL', 'EFLN1EL', 'EFSDLV', 'EFSDVSNS', 'EFSDSCL', 'EFMODE',
];

function sysexTemplateFromAddr(addr, perElement, elementIndex = 0) {
  const b4 = perElement ? (elementIndex & 0x7f) * 0x20 : addr.b4;
  return [
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
  ].join(' ');
}

function catalogKey(entry) {
  return entry.elementIndex === undefined ? entry.id : `${entry.id}:${entry.elementIndex}`;
}

function validate(entries) {
  const seen = new Set();
  const duplicateKeys = [];
  const invalidRanges = [];
  const seenIds = new Set();

  for (const entry of entries) {
    const key = catalogKey(entry);
    if (seen.has(key)) {
      duplicateKeys.push(key);
    }
    seen.add(key);
    seenIds.add(entry.id);

    if (entry.rawMax < entry.rawMin) {
      invalidRanges.push(`${key} (${entry.rawMin}…${entry.rawMax})`);
    }
  }

  const missingIds = SY99_CATALOG_PARAM_IDS.filter((id) => !seenIds.has(id));
  const extraIds = [...seenIds].filter((id) => !SY99_CATALOG_PARAM_IDS.includes(id)).sort();

  return { duplicateKeys, invalidRanges, missingIds, extraIds };
}

const specs = JSON.parse(readFileSync(specsPath, 'utf8'));
if (!Array.isArray(specs)) {
  console.error('sy99CatalogSpecs.json: ожидается массив');
  process.exit(1);
}

const entries = specs.map((spec) => {
  const entry = {
    id: spec.id,
    groupId: spec.groupId,
    rawMin: spec.rawMin,
    rawMax: spec.rawMax,
    uiMin: spec.uiMin,
    uiMax: spec.uiMax,
    decode: spec.decode,
    encode: spec.encode,
  };

  if (spec.enumMap) {
    entry.enumMap = spec.enumMap;
  }

  if (spec.sysexTemplate) {
    entry.sysexTemplate = spec.sysexTemplate;
  } else if (spec.addr) {
    entry.sysexTemplate = sysexTemplateFromAddr(
      { ...spec.addr, perElement: spec.perElement },
      spec.perElement,
      0,
    );
  }

  return entry;
});

const report = validate(entries);

writeFileSync(outPath, `${JSON.stringify(entries, null, 2)}\n`, 'utf8');

console.log(`Записано ${entries.length} записей → ${outPath}`);
console.log(
  'id | groupId | elementIndex | rawMin…rawMax | sysexTemplate',
);
for (const e of entries) {
  const el = e.elementIndex === undefined ? '—' : String(e.elementIndex);
  console.log(
    `${e.id} | ${e.groupId} | ${el} | ${e.rawMin}…${e.rawMax} | ${e.sysexTemplate ?? ''}`,
  );
}

if (report.missingIds.length) {
  console.error('\nОтсутствуют id:', report.missingIds.join(', '));
}
if (report.extraIds.length) {
  console.error('\nЛишние id:', report.extraIds.join(', '));
}
if (report.duplicateKeys.length) {
  console.error('\nДубликаты id+elementIndex:', report.duplicateKeys.join(', '));
}
if (report.invalidRanges.length) {
  console.error('\nНекорректные диапазоны:', report.invalidRanges.join(', '));
}

const ok =
  report.missingIds.length === 0 &&
  report.extraIds.length === 0 &&
  report.duplicateKeys.length === 0 &&
  report.invalidRanges.length === 0;

process.exit(ok ? 0 : 1);

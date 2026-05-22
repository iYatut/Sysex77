import type { ParamCatalogEntry, ParamCodecKind } from '../types/paramCatalog';
import catalogSpecs from '../../fixtures/sy99CatalogSpecs.json';
import { sysexTemplateFromAddr, validateParamCatalog } from '../utils/paramCatalog';

/** 37 параметров Sy99ParamRegistry::Id (без Count). */
export const SY99_CATALOG_PARAM_IDS = [
  'ELMODE',
  'WOL',
  'WPBR',
  'ATPBR',
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
  'WLLML',
  'MCTUN',
  'RNDP',
  'AFTMD',
  'SPTPNT',
  'ELVL',
  'ELDT',
  'ELNS',
  'ENLL',
  'ENLH',
  'EVLL',
  'EVLH',
  'OUTSEL',
  'EFLN1EL',
  'EFSDLV',
  'EFSDVSNS',
  'EFSDSCL',
  'EFMODE',
] as const;

export type Sy99CatalogParamId = (typeof SY99_CATALOG_PARAM_IDS)[number];

type CatalogSpec = {
  id: Sy99CatalogParamId;
  groupId: '8101VC' | '0040VC';
  perElement: boolean;
  rawMin: number;
  rawMax: number;
  uiMin: number;
  uiMax: number;
  decode: ParamCodecKind;
  encode: ParamCodecKind;
  enumMap?: Record<string, number>;
  addr: { b3: number; b4: number; b5: number; b6: number };
};

const SY99_CATALOG_SPECS = catalogSpecs as CatalogSpec[];

function specToEntry(spec: CatalogSpec): ParamCatalogEntry {
  const entry: ParamCatalogEntry = {
    id: spec.id,
    groupId: spec.groupId,
    rawMin: spec.rawMin,
    rawMax: spec.rawMax,
    uiMin: spec.uiMin,
    uiMax: spec.uiMax,
    decode: spec.decode,
    encode: spec.encode,
    sysexTemplate: sysexTemplateFromAddr(
      { ...spec.addr, perElement: spec.perElement },
      spec.perElement,
      0,
    ),
  };

  if (spec.enumMap) {
    entry.enumMap = { ...spec.enumMap };
  }

  return entry;
}

/** Канонический каталог 37 параметров SY99. */
export function sy99ParamCatalogEntries(): ParamCatalogEntry[] {
  return SY99_CATALOG_SPECS.map(specToEntry);
}

export type Sy99CatalogValidationResult = {
  ok: boolean;
  missingIds: string[];
  extraIds: string[];
  duplicateKeys: string[];
  invalidRanges: string[];
};

/** Сверка произвольного каталога с 37 id из Sy99ParamRegistry. */
export function validateSy99CatalogCompleteness(
  entries: ParamCatalogEntry[],
): Sy99CatalogValidationResult {
  const expected = new Set<string>(SY99_CATALOG_PARAM_IDS);
  const seenIds = new Set<string>();
  const duplicateKeys: string[] = [];
  const invalidRanges: string[] = [];
  const keyCounts = new Map<string, number>();

  for (const entry of entries) {
    const key =
      entry.elementIndex === undefined ? entry.id : `${entry.id}:${entry.elementIndex}`;
    keyCounts.set(key, (keyCounts.get(key) ?? 0) + 1);
    seenIds.add(entry.id);

    if (entry.rawMax < entry.rawMin) {
      invalidRanges.push(`${key} (${entry.rawMin}…${entry.rawMax})`);
    }
  }

  for (const [key, count] of keyCounts) {
    if (count > 1) {
      duplicateKeys.push(key);
    }
  }

  const missingIds = SY99_CATALOG_PARAM_IDS.filter((id) => !seenIds.has(id));
  const extraIds = [...seenIds].filter((id) => !expected.has(id)).sort();

  validateParamCatalog(entries);

  return {
    ok:
      missingIds.length === 0 &&
      extraIds.length === 0 &&
      duplicateKeys.length === 0 &&
      invalidRanges.length === 0,
    missingIds: [...missingIds],
    extraIds,
    duplicateKeys,
    invalidRanges,
  };
}

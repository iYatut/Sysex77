/** API elementIndex 0…3 → подпись как на SY99 (элементы 1…4). */
export function elementDisplayNumber(elementIndex: number): number {
  return elementIndex + 1;
}

export function formatElementLabel(elementIndex: number): string {
  return String(elementDisplayNumber(elementIndex));
}

/** Пункт меню на LCD SY99: raw 0 → «01», raw 10 → «11» (только ELMODE). */
export function sy99LcdItemNumber(raw: number): string {
  const n = raw + 1;
  return n < 10 ? `0${n}` : String(n);
}

const TRUTH_SELECT_MAX_OPTIONS = 32;

/** Допустимые VV для OUTSEL (биты & 0x06), не подряд 1…4. */
export const OUTSEL_SY99_RAWS = [0, 2, 4, 6] as const;

const OUTSEL_DEFAULT_LABELS: Record<number, string> = {
  0: 'Off',
  2: 'Group 1',
  4: 'Group 2',
  6: 'Both',
};

export function truthSelectUsesDropdown(paramId: string, rawMin: number, rawMax: number): boolean {
  if (paramId === 'ELMODE' || paramId === 'OUTSEL') {
    return true;
  }
  return rawMax - rawMin + 1 <= TRUTH_SELECT_MAX_OPTIONS;
}

/** Какие raw показывать в выпадающем списке «Сверка» (реальные VV, не порядковый 1…N). */
export type TruthSelectOption = {
  raw: number;
  /** Как в колонке Program таблицы ниже. */
  label: string;
};

function truthLabelForRaw(
  paramId: string,
  raw: number,
  enumNames: readonly string[],
  tableProgramLabel?: string,
): string {
  const fromTable = tableProgramLabel?.trim();
  if (fromTable) {
    return fromTable;
  }
  const fromEnum = enumNames[raw]?.trim();
  if (fromEnum) {
    return fromEnum;
  }
  if (paramId === 'OUTSEL') {
    return OUTSEL_DEFAULT_LABELS[raw] ?? '';
  }
  return '';
}

/** Список «Сверка»: те же raw/подписи, что в нижней таблице (журнал / ✓ / custom). */
export function buildTruthSelectOptions(input: {
  paramId: string;
  rawMin: number;
  rawMax: number;
  enumNames: readonly string[];
  /** Строки из rowsForValueMapTable (без meta-full 128). */
  tableRows: readonly { raw: number; programLabel?: string }[];
  tableFromShortList: boolean;
  extraRaws?: readonly number[];
}): TruthSelectOption[] {
  const { paramId, rawMin, rawMax, enumNames, tableRows, tableFromShortList } = input;
  const labelByRaw = new Map<number, string>();

  if (tableFromShortList && tableRows.length > 0) {
    for (const row of tableRows) {
      const label = truthLabelForRaw(paramId, row.raw, enumNames, row.programLabel);
      labelByRaw.set(row.raw, label);
    }
    const raws = [...labelByRaw.keys()].sort((a, b) => a - b);
    return raws.map((raw) => ({
      raw,
      label: formatCanonicalRawOption(paramId, raw, labelByRaw.get(raw)),
    }));
  }

  const raws = truthRawValuesForParam(
    paramId,
    rawMin,
    rawMax,
    enumNames,
    input.extraRaws ?? [],
  );

  return raws.map((raw) => ({
    raw,
    label: formatCanonicalRawOption(
      paramId,
      raw,
      truthLabelForRaw(paramId, raw, enumNames),
    ),
  }));
}

export function truthRawValuesForParam(
  paramId: string,
  rawMin: number,
  rawMax: number,
  enumNames: readonly string[],
  extraRaws: readonly number[] = [],
): number[] {
  let base: number[];

  if (paramId === 'ELMODE') {
    base = Array.from({ length: 11 }, (_, raw) => raw);
  } else if (paramId === 'OUTSEL') {
    base = [...OUTSEL_SY99_RAWS];
  } else {
    const fromNames: number[] = [];
    for (let raw = 0; raw < enumNames.length; raw++) {
      if (enumNames[raw]?.trim()) {
        fromNames.push(raw);
      }
    }
    if (fromNames.length > 0) {
      base = fromNames;
    } else {
      base = [];
      for (let raw = rawMin; raw <= rawMax; raw++) {
        base.push(raw);
      }
    }
  }

  const merged = new Set(base);
  for (const raw of extraRaws) {
    if (paramId === 'OUTSEL') {
      if ((OUTSEL_SY99_RAWS as readonly number[]).includes(raw)) {
        merged.add(raw);
      }
    } else if (raw >= rawMin && raw <= rawMax) {
      merged.add(raw);
    } else if (paramId === 'ELMODE' && raw >= 0 && raw <= 10) {
      merged.add(raw);
    }
  }

  return [...merged].sort((a, b) => a - b);
}

export function outselMetaRangeLooksWrong(rawMin: number, rawMax: number): boolean {
  return rawMin !== 0 || rawMax < 6;
}

/** Подпись в «Сохранить правду» — без лишнего «SY99» на каждой строке. */
export function formatCanonicalRawOption(
  paramId: string,
  raw: number,
  enumName?: string,
): string {
  const name = enumName?.trim();
  if (paramId === 'ELMODE') {
    const lcd = sy99LcdItemNumber(raw);
    return name ? `${lcd} · ${name} (raw ${raw})` : `${lcd} (raw ${raw})`;
  }
  if (paramId === 'OUTSEL') {
    const label = name || OUTSEL_DEFAULT_LABELS[raw];
    return label ? `raw ${raw} · ${label}` : `raw ${raw}`;
  }
  return name ? `raw ${raw} · ${name}` : `raw ${raw}`;
}

export function truthRawRangeForUi(
  paramId: string,
  rawMin: number,
  rawMax: number,
): { min: number; max: number } {
  if (paramId === 'ELMODE') {
    return { min: 0, max: 10 };
  }
  return { min: rawMin, max: rawMax };
}

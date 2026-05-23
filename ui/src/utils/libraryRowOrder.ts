import type { LibraryVoiceParam } from '../types/library';

/** v2: порядок LCD SY-99 Common (#200…); смена ключа сбрасывает старый localStorage. */
export const LIBRARY_ROW_ORDER_STORAGE_KEY = 'sy99-library-row-order-v2';

export function paramRowKey(row: Pick<LibraryVoiceParam, 'id' | 'elementIndex'>): string {
  return `${row.id}:${row.elementIndex}`;
}

function elementRowKeys(paramId: string, count = 4): string[] {
  return Array.from({ length: count }, (_, elementIndex) => `${paramId}:${elementIndex}`);
}

/**
 * Порядок экранов Common на SY-99 (по lcd_reference + scroll на железе):
 * ELMODE → WOL → mixer strip (ELVL…OUTSEL) → Random Pitch → Pitch Bend →
 * Effect → Micro Tune → Mod Depth (cntrlset) → After Touch / Pan / Other (#230…).
 * ELDPAN (Dynamic Pan) в каталоге пока нет — пропускаем.
 */
export const SY99_LCD_ROW_ORDER: string[] = [
  'ELMODE:0',
  'WOL:0',
  ...elementRowKeys('ELVL'),
  ...elementRowKeys('ELDT'),
  ...elementRowKeys('ELNS'),
  ...elementRowKeys('ENLL'),
  ...elementRowKeys('ENLH'),
  ...elementRowKeys('EVLL'),
  ...elementRowKeys('EVLH'),
  ...elementRowKeys('OUTSEL'),
  'RNDP:0',
  'WPBR:0',
  'EFMODE:0',
  'EFLN1EL:0',
  'EFSDLV:0',
  'EFSDVSNS:0',
  'EFSDSCL:0',
  'MCTUN:0',
  'PMASN:0',
  'PMRNG:0',
  'AMASN:0',
  'AMRNG:0',
  'FMASN:0',
  'FMRNG:0',
  'SPTPNT:0',
  'AFTMD:0',
  'ATPBR:0',
  'PNLASN:0',
  'PNLRNG:0',
  'PNBASN:0',
  'PNBRNG:0',
  'COASN:0',
  'CORNG:0',
  'EGBASN:0',
  'EGBRNG:0',
  'WLASN:0',
  'WLLML:0',
];

/** @deprecated alias — используйте SY99_LCD_ROW_ORDER */
export const SYNTH_COMMON_ROW_ORDER = SY99_LCD_ROW_ORDER;

export function loadSavedRowOrder(): string[] | null {
  try {
    const raw = localStorage.getItem(LIBRARY_ROW_ORDER_STORAGE_KEY);
    if (!raw) {
      return null;
    }
    const parsed = JSON.parse(raw) as unknown;
    if (!Array.isArray(parsed)) {
      return null;
    }
    return parsed.filter((key): key is string => typeof key === 'string');
  } catch {
    return null;
  }
}

export function saveRowOrder(order: string[]): void {
  localStorage.setItem(LIBRARY_ROW_ORDER_STORAGE_KEY, JSON.stringify(order));
}

export function mergeRowOrder(allKeys: string[], saved: string[] | null): string[] {
  if (!saved || saved.length === 0) {
    return [...allKeys];
  }
  const keySet = new Set(allKeys);
  const ordered: string[] = [];
  for (const key of saved) {
    if (keySet.has(key)) {
      ordered.push(key);
      keySet.delete(key);
    }
  }
  for (const key of allKeys) {
    if (keySet.has(key)) {
      ordered.push(key);
    }
  }
  return ordered;
}

export function sortRowsByOrder(rows: LibraryVoiceParam[], order: string[]): LibraryVoiceParam[] {
  const index = new Map(order.map((key, i) => [key, i] as const));
  return [...rows].sort((a, b) => {
    const ia = index.get(paramRowKey(a)) ?? 99999;
    const ib = index.get(paramRowKey(b)) ?? 99999;
    return ia - ib;
  });
}

export function moveRowKey(order: string[], dragKey: string, targetKey: string): string[] {
  if (dragKey === targetKey) {
    return order;
  }
  const next = order.filter((key) => key !== dragKey);
  const targetIndex = next.indexOf(targetKey);
  if (targetIndex < 0) {
    return order;
  }
  next.splice(targetIndex, 0, dragKey);
  return next;
}

export function moveRowKeyByStep(order: string[], rowKey: string, delta: -1 | 1): string[] {
  const index = order.indexOf(rowKey);
  if (index < 0) {
    return order;
  }
  const targetIndex = index + delta;
  if (targetIndex < 0 || targetIndex >= order.length) {
    return order;
  }
  const next = [...order];
  next.splice(index, 1);
  next.splice(targetIndex, 0, rowKey);
  return next;
}

export function buildSy99LcdOrder(allKeys: string[]): string[] {
  return mergeRowOrder(allKeys, SY99_LCD_ROW_ORDER);
}

/** @deprecated alias */
export function buildSynthCommonOrder(allKeys: string[]): string[] {
  return buildSy99LcdOrder(allKeys);
}

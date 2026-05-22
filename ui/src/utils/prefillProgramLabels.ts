import type { ParamMeta, RowOverride } from '../types/paramMeta';
import type { ParamValueMap } from '../types/paramValueMap';
import type { ParamVerificationState } from './paramVerification';
import { verificationFromParam } from './paramVerification';

type LabelTemplate = {
  programLabel: string;
  presentationKind?: RowOverride['presentationKind'];
};

/** Подпись не считается «своей», если совпадает с raw или registry (дефолт API). */
export function isMeaningfulProgramLabel(
  raw: number,
  label: string | undefined,
  registryLabel?: string | null,
): boolean {
  const trimmed = label?.trim();
  if (!trimmed) {
    return false;
  }
  if (trimmed === String(raw)) {
    return false;
  }
  const reg = registryLabel?.trim();
  if (reg && trimmed === reg) {
    return false;
  }
  return true;
}

function countMeaningfulLabels(state: ParamVerificationState): number {
  let count = 0;

  if (state.customRows) {
    for (const row of state.customRows) {
      if (isMeaningfulProgramLabel(row.raw, row.programLabel, row.registryLabel)) {
        count += 1;
      }
    }
  }

  for (const [rawKey, ov] of Object.entries(state.rowOverrides)) {
    const raw = Number.parseInt(rawKey, 10);
    if (Number.isNaN(raw)) {
      continue;
    }
    if (isMeaningfulProgramLabel(raw, ov.programLabel)) {
      count += 1;
    }
  }

  return count;
}

function elementHasMeaningfulProgramLabels(state: ParamVerificationState): boolean {
  return countMeaningfulLabels(state) > 0;
}

function labelMapFromVerification(state: ParamVerificationState): Map<number, LabelTemplate> {
  const map = new Map<number, LabelTemplate>();

  const apply = (raw: number, label: string | undefined, kind?: RowOverride['presentationKind']) => {
    const trimmed = label?.trim();
    if (!trimmed || !isMeaningfulProgramLabel(raw, trimmed)) {
      return;
    }
    map.set(raw, {
      programLabel: trimmed,
      presentationKind: kind ?? map.get(raw)?.presentationKind,
    });
  };

  if (state.customRows) {
    for (const row of state.customRows) {
      apply(row.raw, row.programLabel, row.presentationKind);
    }
  }

  for (const [rawKey, ov] of Object.entries(state.rowOverrides)) {
    const raw = Number.parseInt(rawKey, 10);
    if (Number.isNaN(raw)) {
      continue;
    }
    apply(raw, ov.programLabel, ov.presentationKind);
  }

  return map;
}

/** Element с наибольшим числом осмысленных подписей; при равенстве — больший индекс. */
function findTemplateElementIndex(param: ParamMeta, skipElement: number): number | null {
  let bestElement: number | null = null;
  let bestCount = 0;

  for (let el = 0; el < 4; el++) {
    if (el === skipElement) {
      continue;
    }

    const count = countMeaningfulLabels(verificationFromParam(param, el));
    if (count > bestCount || (count > 0 && count === bestCount && (bestElement === null || el > bestElement))) {
      bestCount = count;
      bestElement = el;
    }
  }

  return bestCount > 0 ? bestElement : null;
}

/** Можно ли автоматически подставлять подписи при открытии страницы. */
export function shouldAutoPrefillProgramLabels(state: ParamVerificationState): boolean {
  if (state.confirmed) {
    return false;
  }
  if (state.customRows !== null) {
    return false;
  }
  if (state.observationLog.trim().length > 0) {
    return false;
  }
  if (state.verifiedRaws.length > 0) {
    return false;
  }
  if (elementHasMeaningfulProgramLabels(state)) {
    return false;
  }
  if (Object.keys(state.rowOverrides).length > 0) {
    return false;
  }
  return true;
}

/**
 * Только Program + Вид — SysEx/ui/список строк не трогаем.
 * Новые строки не добавляются.
 */
export function prefillProgramLabelsFromTemplate(
  param: ParamMeta,
  elementIndex: number,
  verification: ParamVerificationState,
  valueMap: ParamValueMap,
): { state: ParamVerificationState; changed: boolean; templateElementIndex: number | null } {
  if (verification.observationLog.trim().length > 0) {
    return { state: verification, changed: false, templateElementIndex: null };
  }

  if (elementHasMeaningfulProgramLabels(verification)) {
    return { state: verification, changed: false, templateElementIndex: null };
  }

  const templateElementIndex = findTemplateElementIndex(param, elementIndex);
  if (templateElementIndex === null) {
    return { state: verification, changed: false, templateElementIndex: null };
  }

  const templateMap = labelMapFromVerification(verificationFromParam(param, templateElementIndex));
  if (templateMap.size === 0) {
    return { state: verification, changed: false, templateElementIndex: null };
  }

  if (verification.customRows !== null) {
    if (verification.customRows.length === 0) {
      return { state: verification, changed: false, templateElementIndex: null };
    }

    let changed = false;
    const customRows = verification.customRows.map((row) => {
      if (isMeaningfulProgramLabel(row.raw, row.programLabel, row.registryLabel)) {
        return row;
      }
      const tpl = templateMap.get(row.raw);
      if (!tpl) {
        return row;
      }
      changed = true;
      return {
        ...row,
        programLabel: tpl.programLabel,
        ...(tpl.presentationKind !== undefined ? { presentationKind: tpl.presentationKind } : {}),
      };
    });

    if (!changed) {
      return { state: verification, changed: false, templateElementIndex: null };
    }

    return {
      state: {
        ...verification,
        customRows,
        confirmed: false,
        confirmedAt: null,
      },
      changed: true,
      templateElementIndex,
    };
  }

  const rowOverrides: Record<string, RowOverride> = { ...verification.rowOverrides };
  let changed = false;

  for (const row of valueMap.rows) {
    const tpl = templateMap.get(row.raw);
    if (!tpl) {
      continue;
    }

    const key = String(row.raw);
    rowOverrides[key] = {
      ...rowOverrides[key],
      programLabel: tpl.programLabel,
      ...(tpl.presentationKind !== undefined ? { presentationKind: tpl.presentationKind } : {}),
    };
    changed = true;
  }

  if (!changed) {
    return { state: verification, changed: false, templateElementIndex: null };
  }

  return {
    state: {
      ...verification,
      rowOverrides,
      confirmed: false,
      confirmedAt: null,
    },
    changed: true,
    templateElementIndex,
  };
}

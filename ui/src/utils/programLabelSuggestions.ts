import type { ParamMeta, Sy99ElementVerification } from '../types/paramMeta';
import type { DisplayValueMapRow } from '../types/paramValueMap';
import type { ParamVerificationState } from './paramVerification';

function addLabel(seen: Set<string>, out: string[], value: string | undefined) {
  if (value === undefined) {
    return;
  }
  const trimmed = value.trim();
  if (!trimmed) {
    return;
  }
  const key = trimmed.toLowerCase();
  if (seen.has(key)) {
    return;
  }
  seen.add(key);
  out.push(trimmed);
}

function collectFromElementState(
  seen: Set<string>,
  out: string[],
  state: Partial<Sy99ElementVerification> | undefined,
) {
  if (!state) {
    return;
  }

  if (state.customRows) {
    for (const row of state.customRows) {
      addLabel(seen, out, row.programLabel);
    }
  }

  if (state.rowOverrides) {
    for (const ov of Object.values(state.rowOverrides)) {
      addLabel(seen, out, ov.programLabel);
    }
  }
}

/** Уникальные подписи Program: все element + enum + текущая таблица. */
export function collectProgramLabelSuggestions(
  param: ParamMeta | null | undefined,
  verification: ParamVerificationState,
  displayRows: DisplayValueMapRow[],
): string[] {
  const seen = new Set<string>();
  const out: string[] = [];

  for (const row of displayRows) {
    addLabel(seen, out, row.programLabel);
  }

  collectFromElementState(seen, out, {
    customRows: verification.customRows ?? undefined,
    rowOverrides: verification.rowOverrides,
  });

  if (param?.sy99Verification) {
    for (const state of Object.values(param.sy99Verification)) {
      collectFromElementState(seen, out, state);
    }
  }

  if (param?.enumNames) {
    for (const name of param.enumNames) {
      addLabel(seen, out, name);
    }
  }

  return out.sort((a, b) => a.localeCompare(b, undefined, { sensitivity: 'base' }));
}

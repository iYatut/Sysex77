import { ELMODE_BUILTIN_RANGE } from './paramForm';

/** Компактная подпись ELMODE для карточек Library (как на LCD SY-99). */
const ELMODE_CARD_LABELS: Record<number, string> = {
  0: '1AFM MONO',
  1: '2AFM MONO',
  2: '4AFM MONO',
  3: '1AFM',
  4: '2AFM',
  5: '1AWM',
  6: '2AWM',
  7: '4AWM',
  8: '1AFM+1AWM',
  9: '2AFM+2AWM',
  10: 'DRUM',
};

export function formatElmodeCardLabel(raw: number | null | undefined): string {
  if (raw === null || raw === undefined || raw < 0 || raw > 10) {
    return '';
  }

  const compact = ELMODE_CARD_LABELS[raw];
  if (compact) {
    return compact;
  }

  return ELMODE_BUILTIN_RANGE.enumNames[raw]?.trim() ?? '';
}

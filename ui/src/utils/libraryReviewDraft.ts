import type { LibraryVoiceParam } from '../types/library';
import type {
  LibraryAutoCheckSnapshot,
  LibraryAutoCheckStatus,
  LibraryReviewField,
  LibraryReviewItem,
} from '../types/libraryReview';
import { reviewItemKey } from '../types/libraryReview';
import type { LibraryColumnDef } from './libraryParamsColumns';

export function isAutoSuspect(status: LibraryAutoCheckStatus | string | undefined): boolean {
  return status === 'decode' || status === 'source' || status === 'fixture';
}

export function getFieldAppValue(row: LibraryVoiceParam, field: LibraryReviewField): string {
  switch (field) {
    case 'raw8101':
      return row.raw8101 === null ? '—' : String(row.raw8101);
    case 'raw0040':
      return row.raw0040 === null ? '—' : String(row.raw0040);
    case 'uiValue':
      return row.uiValue === null ? '—' : String(row.uiValue);
    case 'valueLabel':
      return row.valueLabel || (row.uiValue !== null ? String(row.uiValue) : '—');
    default:
      return '—';
  }
}

export function buildAutoChecksSnapshot(rows: LibraryVoiceParam[]): LibraryAutoCheckSnapshot[] {
  return rows
    .filter((row) => isAutoSuspect(row.autoCheckStatus))
    .map((row) => ({
      paramId: row.id,
      elementIndex: row.elementIndex,
      autoStatus: row.autoCheckStatus as LibraryAutoCheckStatus,
      appUi: row.uiValue,
      expectedUi: row.expectedUi ?? undefined,
      fixtureId: row.fixtureId,
    }));
}

export function computeResolvedManualItems(
  manualItems: LibraryReviewItem[],
  rows: LibraryVoiceParam[],
): Set<string> {
  const resolved = new Set<string>();
  const rowByKey = new Map(
    rows.map((row) => [`${row.id}:${row.elementIndex}`, row] as const),
  );

  for (const item of manualItems) {
    if (item.source !== 'manual') {
      continue;
    }
    const row = rowByKey.get(`${item.paramId}:${item.elementIndex}`);
    if (!row) {
      continue;
    }
    const current = getFieldAppValue(row, item.field);
    if (current === item.synthValue) {
      resolved.add(reviewItemKey(item.paramId, item.elementIndex, item.field));
    }
  }

  return resolved;
}

export function formatAgentPrompt(
  voiceName: string,
  slotCode: string,
  reviewUrl: string,
  manualItems: LibraryReviewItem[],
  autoItems: LibraryAutoCheckSnapshot[],
): string {
  const lines = [
    `Library review: ${slotCode} ${voiceName}`,
    reviewUrl,
    '',
    'Manual LCD mismatches:',
  ];

  if (manualItems.length === 0) {
    lines.push('- (none)');
  } else {
    for (const item of manualItems) {
      lines.push(
        `- ${item.paramId} El.${item.elementIndex} ${item.field}: app "${item.appValue}" vs synth "${item.synthValue}"`,
      );
    }
  }

  lines.push('', 'Auto suspects (unconfirmed):');
  if (autoItems.length === 0) {
    lines.push('- (none)');
  } else {
    for (const item of autoItems) {
      lines.push(
        `- ${item.paramId} El.${item.elementIndex} ${item.autoStatus}: app UI ${item.appUi ?? '—'}${item.expectedUi !== undefined ? ` expected ${item.expectedUi}` : ''}`,
      );
    }
  }

  return lines.join('\n');
}

export function defaultSynthHint(
  row: LibraryVoiceParam,
  column: LibraryColumnDef,
): string {
  if (column.field === 'uiValue' && row.expectedUi !== null && row.expectedUi !== undefined) {
    return String(row.expectedUi);
  }
  return '';
}

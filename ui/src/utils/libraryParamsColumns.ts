import { formatLibraryValue } from '../types/library';
import type { LibraryVoiceParam } from '../types/library';
import type { LibraryReviewField } from '../types/libraryReview';

export type LibraryColumnId =
  | 'group'
  | 'uiLabel'
  | 'paramTag'
  | 'elementIndex'
  | 'addressHex'
  | 'bindStatus'
  | 'raw8101'
  | 'raw0040'
  | 'uiValue'
  | 'valueLabel';

export const DEFAULT_LIBRARY_COLUMN_ORDER: LibraryColumnId[] = [
  'group',
  'uiLabel',
  'paramTag',
  'elementIndex',
  'addressHex',
  'bindStatus',
  'raw8101',
  'raw0040',
  'uiValue',
  'valueLabel',
];

export const LIBRARY_COLUMN_ORDER_STORAGE_KEY = 'sy99-library-column-order';

export interface LibraryColumnDef {
  id: LibraryColumnId;
  label: string;
  mono?: boolean;
  annotatable?: boolean;
  field?: LibraryReviewField;
  getText: (row: LibraryVoiceParam) => string;
}

export const LIBRARY_COLUMNS: Record<LibraryColumnId, LibraryColumnDef> = {
  group: {
    id: 'group',
    label: 'Group',
    getText: (row) => row.group,
  },
  uiLabel: {
    id: 'uiLabel',
    label: 'Название',
    getText: (row) => row.uiLabel,
  },
  paramTag: {
    id: 'paramTag',
    label: 'Tag',
    mono: true,
    getText: (row) => row.paramTag,
  },
  elementIndex: {
    id: 'elementIndex',
    label: 'El',
    mono: true,
    getText: (row) => String(row.elementIndex),
  },
  addressHex: {
    id: 'addressHex',
    label: 'Address',
    mono: true,
    getText: (row) => row.addressHex,
  },
  bindStatus: {
    id: 'bindStatus',
    label: 'Bind',
    mono: true,
    getText: (row) => row.bindStatus ?? 'manual_only',
  },
  raw8101: {
    id: 'raw8101',
    label: '8101 sysex',
    mono: true,
    annotatable: true,
    field: 'raw8101',
    getText: (row) => formatLibraryValue(row.raw8101),
  },
  raw0040: {
    id: 'raw0040',
    label: '0040 sysex',
    mono: true,
    annotatable: true,
    field: 'raw0040',
    getText: (row) => formatLibraryValue(row.raw0040),
  },
  uiValue: {
    id: 'uiValue',
    label: 'UI',
    mono: true,
    annotatable: true,
    field: 'uiValue',
    getText: (row) => formatLibraryValue(row.uiValue),
  },
  valueLabel: {
    id: 'valueLabel',
    label: 'Расшифровка',
    annotatable: true,
    field: 'valueLabel',
    getText: (row) => row.valueLabel || (row.uiValue !== null ? String(row.uiValue) : '—'),
  },
};

export function loadColumnOrder(): LibraryColumnId[] {
  try {
    const raw = localStorage.getItem(LIBRARY_COLUMN_ORDER_STORAGE_KEY);
    if (!raw) {
      return [...DEFAULT_LIBRARY_COLUMN_ORDER];
    }
    const parsed = JSON.parse(raw) as unknown;
    if (!Array.isArray(parsed)) {
      return [...DEFAULT_LIBRARY_COLUMN_ORDER];
    }
    const valid = parsed.filter(
      (id): id is LibraryColumnId =>
        typeof id === 'string' && id in LIBRARY_COLUMNS,
    );
    const missing = DEFAULT_LIBRARY_COLUMN_ORDER.filter((id) => !valid.includes(id));
    return [...valid, ...missing];
  } catch {
    return [...DEFAULT_LIBRARY_COLUMN_ORDER];
  }
}

export function saveColumnOrder(order: LibraryColumnId[]): void {
  localStorage.setItem(LIBRARY_COLUMN_ORDER_STORAGE_KEY, JSON.stringify(order));
}

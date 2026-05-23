export type LibraryReviewField = 'raw8101' | 'raw0040' | 'uiValue' | 'valueLabel';

export type LibraryAutoCheckStatus =
  | 'ok'
  | 'missing'
  | 'decode'
  | 'source'
  | 'fixture'
  | 'catalog';

export type LibraryReviewItemSource = 'manual' | 'confirmed_ok';

export interface LibraryReviewItem {
  source: LibraryReviewItemSource;
  paramId: string;
  elementIndex: number;
  field: LibraryReviewField;
  appValue: string;
  synthValue: string;
}

export interface LibraryAutoCheckSnapshot {
  paramId: string;
  elementIndex: number;
  autoStatus: LibraryAutoCheckStatus;
  appUi: number | null;
  expectedUi?: number | null;
  fixtureId?: string;
}

export interface LibraryReviewSubmitBody {
  pageId: string;
  pageLabel: string;
  mm: number;
  slotCode: string;
  voiceName: string;
  captureFile: string;
  fixtureId?: string;
  manualItems: LibraryReviewItem[];
  confirmedOk: LibraryReviewItem[];
  autoChecksSnapshot: LibraryAutoCheckSnapshot[];
}

export interface LibraryReviewCreated {
  id: string;
  url: string;
  apiUrl: string;
}

export interface LibraryReviewRecord extends LibraryReviewSubmitBody {
  id: string;
  createdAt: string;
}

export type IssueFilter = 'all' | 'auto' | 'manual' | 'issues';

export function reviewItemKey(
  paramId: string,
  elementIndex: number,
  field: LibraryReviewField,
): string {
  return `${paramId}:${elementIndex}:${field}`;
}

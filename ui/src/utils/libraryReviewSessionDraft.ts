import type { LibraryReviewItem } from '../types/libraryReview';
import { reviewItemKey } from '../types/libraryReview';

const STORAGE_PREFIX = 'library-review-draft';

export function libraryReviewDraftKey(pageId: string, mm: number): string {
  return `${STORAGE_PREFIX}:${pageId}:${mm}`;
}

export function loadLibraryReviewDraft(pageId: string, mm: number): LibraryReviewItem[] {
  try {
    const raw = sessionStorage.getItem(libraryReviewDraftKey(pageId, mm));
    if (!raw) {
      return [];
    }
    const parsed = JSON.parse(raw) as unknown;
    if (!Array.isArray(parsed)) {
      return [];
    }
    const items: LibraryReviewItem[] = [];
    for (const entry of parsed) {
      if (
        typeof entry === 'object' &&
        entry !== null &&
        (entry as LibraryReviewItem).source &&
        typeof (entry as LibraryReviewItem).paramId === 'string' &&
        typeof (entry as LibraryReviewItem).elementIndex === 'number' &&
        typeof (entry as LibraryReviewItem).field === 'string' &&
        typeof (entry as LibraryReviewItem).appValue === 'string' &&
        typeof (entry as LibraryReviewItem).synthValue === 'string'
      ) {
        items.push(entry as LibraryReviewItem);
      }
    }
    return items;
  } catch {
    return [];
  }
}

export function saveLibraryReviewDraft(
  pageId: string,
  mm: number,
  items: LibraryReviewItem[],
): void {
  try {
    const key = libraryReviewDraftKey(pageId, mm);
    if (items.length === 0) {
      sessionStorage.removeItem(key);
      return;
    }
    sessionStorage.setItem(key, JSON.stringify(items));
  } catch {
    /* ignore quota / private mode */
  }
}

export function clearLibraryReviewDraft(pageId: string, mm: number): void {
  try {
    sessionStorage.removeItem(libraryReviewDraftKey(pageId, mm));
  } catch {
    /* ignore */
  }
}

export function manualItemsFromDraft(items: LibraryReviewItem[]): Map<string, LibraryReviewItem> {
  const map = new Map<string, LibraryReviewItem>();
  for (const item of items) {
    map.set(reviewItemKey(item.paramId, item.elementIndex, item.field), item);
  }
  return map;
}

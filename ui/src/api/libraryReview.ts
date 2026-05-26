import { apiFetch } from './client';
import type { LibraryReviewCreated, LibraryReviewRecord, LibraryReviewSubmitBody } from '../types/libraryReview';

export async function createLibraryReview(
  body: LibraryReviewSubmitBody,
): Promise<LibraryReviewCreated> {
  return apiFetch<LibraryReviewCreated>('/library/reviews', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  });
}

export async function fetchLibraryReview(reviewId: string): Promise<LibraryReviewRecord> {
  return apiFetch<LibraryReviewRecord>(`/library/reviews/${encodeURIComponent(reviewId)}`);
}

export async function deleteLibraryReview(reviewId: string): Promise<void> {
  await apiFetch<unknown>(`/library/reviews/${encodeURIComponent(reviewId)}`, {
    method: 'DELETE',
  });
}

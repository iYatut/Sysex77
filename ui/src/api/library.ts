import type { LibraryTree, LibraryVoiceDetail, LibraryVoicesResponse } from '../types/library';
import { ApiError, apiFetch } from './client';

async function sleep(ms: number): Promise<void> {
  await new Promise((resolve) => {
    window.setTimeout(resolve, ms);
  });
}

async function fetchLibraryWithRetry<T>(path: string, attempts = 3): Promise<T> {
  let lastError: unknown;

  for (let attempt = 0; attempt < attempts; ++attempt) {
    try {
      return await apiFetch<T>(path);
    } catch (error) {
      lastError = error;

      if (
        error instanceof ApiError &&
        error.status === 503 &&
        attempt + 1 < attempts
      ) {
        const retryAfterMs =
          typeof error.body === 'object' &&
          error.body !== null &&
          'retryAfterMs' in error.body &&
          typeof (error.body as { retryAfterMs: unknown }).retryAfterMs === 'number'
            ? (error.body as { retryAfterMs: number }).retryAfterMs
            : 500;

        await sleep(retryAfterMs);
        continue;
      }

      throw error;
    }
  }

  throw lastError;
}

export function fetchLibraryTree(): Promise<LibraryTree> {
  return fetchLibraryWithRetry<LibraryTree>('/library/tree');
}

export function fetchLibraryVoices(pageId: string): Promise<LibraryVoicesResponse> {
  return fetchLibraryWithRetry<LibraryVoicesResponse>(
    `/library/pages/${encodeURIComponent(pageId)}/voices`,
  );
}

export function fetchLibraryVoiceDetail(pageId: string, mm: number): Promise<LibraryVoiceDetail> {
  return fetchLibraryWithRetry<LibraryVoiceDetail>(
    `/library/pages/${encodeURIComponent(pageId)}/voices/${encodeURIComponent(String(mm))}`,
  );
}

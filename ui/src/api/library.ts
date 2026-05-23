import type { LibraryTree, LibraryVoiceDetail, LibraryVoicesResponse } from '../types/library';
import { apiFetch } from './client';

export function fetchLibraryTree(): Promise<LibraryTree> {
  return apiFetch<LibraryTree>('/library/tree');
}

export function fetchLibraryVoices(pageId: string): Promise<LibraryVoicesResponse> {
  return apiFetch<LibraryVoicesResponse>(`/library/pages/${encodeURIComponent(pageId)}/voices`);
}

export function fetchLibraryVoiceDetail(pageId: string, mm: number): Promise<LibraryVoiceDetail> {
  return apiFetch<LibraryVoiceDetail>(
    `/library/pages/${encodeURIComponent(pageId)}/voices/${encodeURIComponent(String(mm))}`,
  );
}

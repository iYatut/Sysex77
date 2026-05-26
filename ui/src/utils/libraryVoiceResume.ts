/** Возврат с ParamDetailPage на тот же голос в Library (не корень /library). */
export type LibraryVoiceResume = {
  libraryId: string;
  pageId: string;
  mm: number;
  voiceLabel: string;
  reviewId?: string;
};

const STORAGE_KEY = 'sy99-library-voice-resume';

export function saveLibraryVoiceResume(resume: LibraryVoiceResume): void {
  try {
    sessionStorage.setItem(STORAGE_KEY, JSON.stringify(resume));
  } catch {
    /* ignore quota / private mode */
  }
}

export function loadLibraryVoiceResume(): LibraryVoiceResume | null {
  try {
    const raw = sessionStorage.getItem(STORAGE_KEY);
    if (!raw) {
      return null;
    }
    const parsed = JSON.parse(raw) as Partial<LibraryVoiceResume>;
    if (
      typeof parsed.libraryId !== 'string' ||
      !parsed.libraryId ||
      typeof parsed.pageId !== 'string' ||
      !parsed.pageId ||
      typeof parsed.mm !== 'number' ||
      parsed.mm < 0 ||
      parsed.mm > 63 ||
      typeof parsed.voiceLabel !== 'string' ||
      !parsed.voiceLabel
    ) {
      return null;
    }
    return {
      libraryId: parsed.libraryId,
      pageId: parsed.pageId,
      mm: parsed.mm,
      voiceLabel: parsed.voiceLabel,
      reviewId: typeof parsed.reviewId === 'string' && parsed.reviewId ? parsed.reviewId : undefined,
    };
  } catch {
    return null;
  }
}

export function libraryVoiceResumePath(resume: LibraryVoiceResume): string {
  const base = `/library/${encodeURIComponent(resume.libraryId)}/${encodeURIComponent(resume.pageId)}/${encodeURIComponent(String(resume.mm))}`;
  return resume.reviewId
    ? `${base}?review=${encodeURIComponent(resume.reviewId)}`
    : base;
}

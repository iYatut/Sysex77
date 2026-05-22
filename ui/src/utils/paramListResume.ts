/** Позиция в списке параметров — чтобы «К списку» возвращал к той же строке. */
export type ParamListResume = {
  paramId: string;
  elementIndex: number;
  tagQuery?: string;
  groupQuery?: string;
};

const STORAGE_KEY = 'sy99-param-list-resume';

export function saveParamListResume(resume: ParamListResume): void {
  try {
    sessionStorage.setItem(STORAGE_KEY, JSON.stringify(resume));
  } catch {
    /* ignore quota / private mode */
  }
}

export function loadParamListResume(): ParamListResume | null {
  try {
    const raw = sessionStorage.getItem(STORAGE_KEY);
    if (!raw) {
      return null;
    }
    const parsed = JSON.parse(raw) as Partial<ParamListResume>;
    if (typeof parsed.paramId !== 'string' || !parsed.paramId) {
      return null;
    }
    const elementIndex =
      typeof parsed.elementIndex === 'number' && parsed.elementIndex >= 0 && parsed.elementIndex <= 3
        ? parsed.elementIndex
        : 0;
    return {
      paramId: parsed.paramId,
      elementIndex,
      tagQuery: typeof parsed.tagQuery === 'string' ? parsed.tagQuery : '',
      groupQuery: typeof parsed.groupQuery === 'string' ? parsed.groupQuery : '',
    };
  } catch {
    return null;
  }
}

export function resumeDetailPath(resume: ParamListResume): string {
  const base = `/params/${encodeURIComponent(resume.paramId)}`;
  return resume.elementIndex > 0 ? `${base}?element=${resume.elementIndex}` : base;
}

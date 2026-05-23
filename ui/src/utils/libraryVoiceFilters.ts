export const LIBRARY_VOICE_FOCUS_FILTERS_EVENT = 'library-voice-focus-filters';

export function isLibraryVoiceDetailPath(pathname: string): boolean {
  const parts = pathname.split('/').filter(Boolean);
  return parts[0] === 'library' && parts.length === 4;
}

export function focusLibraryVoiceFilters(): void {
  window.dispatchEvent(new CustomEvent(LIBRARY_VOICE_FOCUS_FILTERS_EVENT));
}

export function scrollToLibraryVoiceFilters(toolbar: HTMLElement): void {
  const root = document.documentElement;
  const nav = document.querySelector('.app-shell:has(.page--library-voice) .app-nav');
  const navH =
    parseFloat(root.style.getPropertyValue('--library-voice-nav-h')) ||
    (nav instanceof HTMLElement ? nav.offsetHeight : 0) ||
    58;
  const y = toolbar.getBoundingClientRect().top + window.scrollY - navH - 8;
  window.scrollTo({ top: Math.max(0, y), behavior: 'smooth' });
}

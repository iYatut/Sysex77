import type { LibraryReviewItem } from '../types/libraryReview';
import { isAutoSuspect } from '../utils/libraryReviewDraft';
import type { LibraryVoiceParam } from '../types/library';

type LibraryReviewPanelProps = {
  autoSuspectCount: number;
  manualCount: number;
  resolvedCount: number;
  reviewUrl: string | null;
  reviewId: string | null;
  submitting: boolean;
  submitError: string | null;
  onSubmit: () => void;
  onCopyLink: () => void;
  onCopyPrompt: () => void;
  onDeleteReview: () => void;
};

export function LibraryReviewPanel({
  autoSuspectCount,
  manualCount,
  resolvedCount,
  reviewUrl,
  reviewId,
  submitting,
  submitError,
  onSubmit,
  onCopyLink,
  onCopyPrompt,
  onDeleteReview,
}: LibraryReviewPanelProps) {
  return (
    <div className="library-review-panel">
      <h2 className="library-review-panel__title">Проверка SY-99</h2>
      <p className="page-subtitle">
        auto suspect: <strong>{autoSuspectCount}</strong> · manual:{' '}
        <strong>{manualCount}</strong>
        {resolvedCount > 0 && (
          <>
            {' '}
            · исправлено: <strong>{resolvedCount}</strong>
          </>
        )}
      </p>
      <div className="library-review-panel__actions">
        <button type="button" className="btn btn--primary" disabled={submitting} onClick={onSubmit}>
          {submitting ? 'Отправка…' : 'Отправить отчёт'}
        </button>
        {reviewUrl && (
          <>
            <button type="button" className="btn" onClick={onCopyLink}>
              Скопировать ссылку
            </button>
            <button type="button" className="btn" onClick={onCopyPrompt}>
              Текст для агента
            </button>
          </>
        )}
        {reviewId && (
          <button type="button" className="btn btn--ghost" onClick={onDeleteReview}>
            Закрыть отчёт
          </button>
        )}
      </div>
      {reviewUrl && (
        <p className="library-review-panel__url mono" title={reviewUrl}>
          {reviewUrl}
        </p>
      )}
      {submitError && <div className="alert alert--error">{submitError}</div>}
    </div>
  );
}

export function countManualMismatches(items: LibraryReviewItem[]): number {
  return items.filter((item) => item.source === 'manual').length;
}

export function countAutoSuspects(rows: LibraryVoiceParam[]): number {
  return rows.filter((row) => isAutoSuspect(row.autoCheckStatus)).length;
}

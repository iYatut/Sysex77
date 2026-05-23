import { useEffect, useRef } from 'react';
import type { LibraryReviewField } from '../types/libraryReview';

type LibraryValuePopoverProps = {
  appValue: string;
  initialSynthValue: string;
  anchorRect: DOMRect | null;
  onConfirm: (synthValue: string) => void;
  onConfirmMatch: () => void;
  onClose: () => void;
};

export function LibraryValuePopover({
  appValue,
  initialSynthValue,
  anchorRect,
  onConfirm,
  onConfirmMatch,
  onClose,
}: LibraryValuePopoverProps) {
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    inputRef.current?.focus();
    inputRef.current?.select();
  }, []);

  useEffect(() => {
    function onKey(event: KeyboardEvent) {
      if (event.key === 'Escape') {
        onClose();
      }
    }
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [onClose]);

  if (!anchorRect) {
    return null;
  }

  const top = anchorRect.bottom + window.scrollY + 6;
  const left = Math.min(anchorRect.left + window.scrollX, window.innerWidth - 280);

  return (
    <div
      className="library-value-popover"
      style={{ top, left }}
      onClick={(event) => event.stopPropagation()}
    >
      <p className="library-value-popover__label">На SY-99 вижу:</p>
      <p className="library-value-popover__app">
        В приложении: <span className="mono">{appValue}</span>
      </p>
      <input
        ref={inputRef}
        className="library-value-popover__input mono"
        type="text"
        defaultValue={initialSynthValue}
        onKeyDown={(event) => {
          if (event.key === 'Enter') {
            event.preventDefault();
            onConfirm(event.currentTarget.value.trim());
          }
        }}
      />
      <div className="library-value-popover__actions">
        <button
          type="button"
          className="btn btn--primary"
          onClick={() => onConfirm(inputRef.current?.value.trim() ?? '')}
        >
          OK
        </button>
        <button type="button" className="btn" onClick={onConfirmMatch}>
          ✓ совпадает
        </button>
        <button type="button" className="btn btn--ghost" onClick={onClose}>
          Отмена
        </button>
      </div>
    </div>
  );
}

export type AnnotateTarget = {
  paramId: string;
  elementIndex: number;
  field: LibraryReviewField;
  appValue: string;
  hint: string;
  anchorRect: DOMRect;
};

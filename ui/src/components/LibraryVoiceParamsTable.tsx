import { useCallback, useEffect, useLayoutEffect, useMemo, useRef, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { createLibraryReview, deleteLibraryReview, fetchLibraryReview } from '../api/libraryReview';
import { LibraryReviewPanel } from './LibraryReviewPanel';
import { LibraryValuePopover, type AnnotateTarget } from './LibraryValuePopover';
import type { LibraryVoiceDetail, LibraryVoiceParam } from '../types/library';
import type {
  IssueFilter,
  LibraryReviewField,
  LibraryReviewItem,
} from '../types/libraryReview';
import { reviewItemKey } from '../types/libraryReview';
import {
  DEFAULT_LIBRARY_COLUMN_ORDER,
  LIBRARY_COLUMNS,
  loadColumnOrder,
  saveColumnOrder,
  type LibraryColumnId,
} from '../utils/libraryParamsColumns';
import {
  buildAutoChecksSnapshot,
  computeResolvedManualItems,
  defaultSynthHint,
  formatAgentPrompt,
  getFieldAppValue,
  isAutoSuspect,
} from '../utils/libraryReviewDraft';
import {
  buildSy99LcdOrder,
  loadSavedRowOrder,
  mergeRowOrder,
  moveRowKey,
  moveRowKeyByStep,
  paramRowKey,
  saveRowOrder,
  sortRowsByOrder,
} from '../utils/libraryRowOrder';
import {
  LIBRARY_VOICE_FOCUS_FILTERS_EVENT,
  scrollToLibraryVoiceFilters,
} from '../utils/libraryVoiceFilters';

type LibraryVoiceParamsTableProps = {
  detail: LibraryVoiceDetail;
  libraryId: string;
  reviewIdFromUrl: string | null;
  onReviewIdChange: (reviewId: string | null) => void;
};

function uniqueGroups(rows: LibraryVoiceParam[]): string[] {
  return [...new Set(rows.map((row) => row.group).filter(Boolean))].sort();
}

function uniqueSysexGroups(rows: LibraryVoiceParam[]): number[] {
  return [...new Set(rows.map((row) => row.sysexGroup).filter((g): g is number => g != null && g > 0))].sort(
    (a, b) => a - b,
  );
}

function bindStatusLabel(status: string | undefined): string {
  const value = status ?? 'manual_only';
  if (value.startsWith('confirmed')) {
    return 'confirmed';
  }
  if (value.startsWith('candidate')) {
    return 'candidate';
  }
  return 'manual_only';
}

function matchesBindFilter(row: LibraryVoiceParam, filter: BindStatusFilter): boolean {
  if (filter === 'all') {
    return true;
  }
  const label = bindStatusLabel(row.bindStatus);
  return label === filter;
}

type BindStatusFilter = 'all' | 'manual_only' | 'confirmed' | 'candidate';

function rowHasManualMismatch(
  row: LibraryVoiceParam,
  manualItems: Map<string, LibraryReviewItem>,
): boolean {
  const fields: LibraryReviewField[] = ['raw8101', 'raw0040', 'uiValue', 'valueLabel'];
  return fields.some((field) => {
    const item = manualItems.get(reviewItemKey(row.id, row.elementIndex, field));
    return item?.source === 'manual' && item.appValue !== item.synthValue;
  });
}

function statusDotClass(
  row: LibraryVoiceParam,
  manualItems: Map<string, LibraryReviewItem>,
): string {
  if (rowHasManualMismatch(row, manualItems)) {
    return 'library-status-dot library-status-dot--manual';
  }
  if (isAutoSuspect(row.autoCheckStatus)) {
    return 'library-status-dot library-status-dot--auto';
  }
  const bind = bindStatusLabel(row.bindStatus);
  if (bind === 'manual_only' && !row.inDump) {
    return 'library-status-dot library-status-dot--missing';
  }
  if (bind === 'candidate') {
    return 'library-status-dot library-status-dot--candidate';
  }
  if (bind === 'confirmed' && row.inDump) {
    return 'library-status-dot library-status-dot--ok';
  }
  if (!row.inDump) {
    return 'library-status-dot library-status-dot--missing';
  }
  return 'library-status-dot library-status-dot--ok';
}

function rowClassName(
  row: LibraryVoiceParam,
  manualItems: Map<string, LibraryReviewItem>,
  isDragOver: boolean,
): string {
  const classes = ['params-row'];
  if (isDragOver) {
    classes.push('library-param-row--drag-over');
  }
  if (rowHasManualMismatch(row, manualItems)) {
    classes.push('library-param-row--manual-mismatch');
  } else if (isAutoSuspect(row.autoCheckStatus)) {
    classes.push('library-param-row--auto-suspect');
  } else if (!row.inDump) {
    classes.push('library-param-row--missing');
  }
  return classes.join(' ');
}

export function LibraryVoiceParamsTable({
  detail,
  libraryId,
  reviewIdFromUrl,
  onReviewIdChange,
}: LibraryVoiceParamsTableProps) {
  const navigate = useNavigate();
  const rows = detail.params;
  const allRowKeys = useMemo(() => rows.map(paramRowKey), [rows]);

  const [groupQuery, setGroupQuery] = useState('');
  const [sysexGroupQuery, setSysexGroupQuery] = useState('');
  const [tagQuery, setTagQuery] = useState('');
  const [inDumpOnly, setInDumpOnly] = useState(false);
  const [bindStatusFilter, setBindStatusFilter] = useState<BindStatusFilter>('all');
  const [issueFilter, setIssueFilter] = useState<IssueFilter>('all');
  const [rowOrder, setRowOrder] = useState<string[]>(() => {
    const keys = rows.map(paramRowKey);
    const saved = loadSavedRowOrder();
    if (!saved || saved.length === 0) {
      return buildSy99LcdOrder(keys);
    }
    return mergeRowOrder(keys, saved);
  });
  const [columnOrder, setColumnOrder] = useState<LibraryColumnId[]>(() => loadColumnOrder());
  const [dragRowKey, setDragRowKey] = useState<string | null>(null);
  const [dragOverRowKey, setDragOverRowKey] = useState<string | null>(null);
  const [dragColumnId, setDragColumnId] = useState<LibraryColumnId | null>(null);

  const [manualItems, setManualItems] = useState<Map<string, LibraryReviewItem>>(new Map());
  const [annotateTarget, setAnnotateTarget] = useState<AnnotateTarget | null>(null);

  const [reviewId, setReviewId] = useState<string | null>(reviewIdFromUrl);
  const [reviewUrl, setReviewUrl] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);
  const [submitError, setSubmitError] = useState<string | null>(null);

  useEffect(() => {
    setRowOrder((prev) => {
      const saved = loadSavedRowOrder();
      if ((!prev.length || prev.length === 0) && (!saved || saved.length === 0)) {
        return buildSy99LcdOrder(allRowKeys);
      }
      return mergeRowOrder(allRowKeys, prev.length > 0 ? prev : saved);
    });
  }, [allRowKeys]);

  useEffect(() => {
    setReviewId(reviewIdFromUrl);
  }, [reviewIdFromUrl]);

  useEffect(() => {
    if (!reviewIdFromUrl) {
      return;
    }
    let cancelled = false;
    void fetchLibraryReview(reviewIdFromUrl)
      .then((record) => {
        if (cancelled) {
          return;
        }
        const next = new Map<string, LibraryReviewItem>();
        for (const item of record.manualItems ?? []) {
          next.set(reviewItemKey(item.paramId, item.elementIndex, item.field), item);
        }
        for (const item of record.confirmedOk ?? []) {
          next.set(reviewItemKey(item.paramId, item.elementIndex, item.field), item);
        }
        setManualItems(next);
        setReviewUrl(
          `http://localhost:5173/library/${libraryId}/${detail.pageId}/${detail.mm}?review=${record.id}`,
        );
      })
      .catch(() => {
        if (!cancelled) {
          setSubmitError('Не удалось загрузить отчёт по ссылке');
        }
      });
    return () => {
      cancelled = true;
    };
  }, [reviewIdFromUrl, libraryId, detail.pageId, detail.mm]);

  const orderedRows = useMemo(() => sortRowsByOrder(rows, rowOrder), [rows, rowOrder]);

  const resolvedKeys = useMemo(
    () => computeResolvedManualItems([...manualItems.values()], rows),
    [manualItems, rows],
  );

  const groups = useMemo(() => uniqueGroups(rows), [rows]);
  const sysexGroups = useMemo(() => uniqueSysexGroups(rows), [rows]);
  const inDumpCount = useMemo(() => rows.filter((row) => row.inDump).length, [rows]);
  const confirmedCount = useMemo(
    () => rows.filter((row) => bindStatusLabel(row.bindStatus) === 'confirmed').length,
    [rows],
  );
  const unboundCount = useMemo(
    () => rows.filter((row) => bindStatusLabel(row.bindStatus) === 'manual_only').length,
    [rows],
  );
  const autoSuspectCount = useMemo(
    () => rows.filter((row) => isAutoSuspect(row.autoCheckStatus)).length,
    [rows],
  );
  const manualCount = useMemo(
    () => [...manualItems.values()].filter((item) => item.source === 'manual').length,
    [manualItems],
  );

  const orderedColumns = useMemo(() => {
    const valid = columnOrder.filter((id) => id in LIBRARY_COLUMNS);
    const missing = DEFAULT_LIBRARY_COLUMN_ORDER.filter((id) => !valid.includes(id));
    return [...valid, ...missing].map((id) => LIBRARY_COLUMNS[id]);
  }, [columnOrder]);

  const filteredRows = useMemo(() => {
    const tagNeedle = tagQuery.trim().toLowerCase();

    return orderedRows.filter((row) => {
      if (inDumpOnly && !row.inDump) {
        return false;
      }
      if (groupQuery && row.group !== groupQuery) {
        return false;
      }
      if (sysexGroupQuery && String(row.sysexGroup ?? '') !== sysexGroupQuery) {
        return false;
      }
      if (!matchesBindFilter(row, bindStatusFilter)) {
        return false;
      }
      if (tagNeedle) {
        const matchesTag =
          row.paramTag.toLowerCase().includes(tagNeedle) ||
          row.uiLabel.toLowerCase().includes(tagNeedle) ||
          row.id.toLowerCase().includes(tagNeedle);
        if (!matchesTag) {
          return false;
        }
      }

      const hasManual = rowHasManualMismatch(row, manualItems);
      const hasAuto = isAutoSuspect(row.autoCheckStatus);

      if (issueFilter === 'auto' && !hasAuto) {
        return false;
      }
      if (issueFilter === 'manual' && !hasManual) {
        return false;
      }
      if (issueFilter === 'issues' && !hasManual && !hasAuto) {
        return false;
      }

      return true;
    });
  }, [orderedRows, groupQuery, sysexGroupQuery, tagQuery, inDumpOnly, bindStatusFilter, issueFilter, manualItems]);

  const filtersToolbarRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const handler = () => {
      const toolbar = filtersToolbarRef.current;
      if (!toolbar) {
        return;
      }
      requestAnimationFrame(() => scrollToLibraryVoiceFilters(toolbar));
    };

    window.addEventListener(LIBRARY_VOICE_FOCUS_FILTERS_EVENT, handler);
    return () => window.removeEventListener(LIBRARY_VOICE_FOCUS_FILTERS_EVENT, handler);
  }, []);

  useLayoutEffect(() => {
    const root = document.documentElement;
    const nav = document.querySelector('.app-shell:has(.page--library-voice) .app-nav');

    const syncNavHeight = () => {
      if (nav instanceof HTMLElement) {
        root.style.setProperty('--library-voice-nav-h', `${nav.offsetHeight}px`);
      }
    };

    syncNavHeight();

    const resizeObserver = new ResizeObserver(syncNavHeight);
    if (nav instanceof HTMLElement) {
      resizeObserver.observe(nav);
    }
    window.addEventListener('resize', syncNavHeight);

    return () => {
      resizeObserver.disconnect();
      window.removeEventListener('resize', syncNavHeight);
      root.style.removeProperty('--library-voice-nav-h');
    };
  }, []);

  const persistRowOrder = useCallback((next: string[]) => {
    setRowOrder(next);
    saveRowOrder(next);
  }, []);

  const setReviewItem = useCallback((item: LibraryReviewItem | null, key: string) => {
    setManualItems((prev) => {
      const next = new Map(prev);
      if (item) {
        next.set(key, item);
      } else {
        next.delete(key);
      }
      return next;
    });
  }, []);

  const openAnnotate = useCallback(
    (row: LibraryVoiceParam, field: LibraryReviewField, event: React.MouseEvent | React.KeyboardEvent) => {
      event.stopPropagation();
      const target = event.currentTarget as HTMLElement;
      const column = orderedColumns.find((col) => col.field === field);
      const key = reviewItemKey(row.id, row.elementIndex, field);
      const existing = manualItems.get(key);
      setAnnotateTarget({
        paramId: row.id,
        elementIndex: row.elementIndex,
        field,
        appValue: getFieldAppValue(row, field),
        hint: existing?.synthValue ?? (column ? defaultSynthHint(row, column) : ''),
        anchorRect: target.getBoundingClientRect(),
      });
    },
    [manualItems, orderedColumns],
  );

  const handleColumnDrop = (targetId: LibraryColumnId) => {
    if (!dragColumnId || dragColumnId === targetId) {
      return;
    }
    const next = [...columnOrder];
    const from = next.indexOf(dragColumnId);
    const to = next.indexOf(targetId);
    if (from < 0 || to < 0) {
      return;
    }
    next.splice(from, 1);
    next.splice(to, 0, dragColumnId);
    setColumnOrder(next);
    saveColumnOrder(next);
    setDragColumnId(null);
  };

  const handleRowDrop = (targetKey: string) => {
    if (!dragRowKey || dragRowKey === targetKey) {
      return;
    }
    persistRowOrder(moveRowKey(rowOrder, dragRowKey, targetKey));
    setDragRowKey(null);
    setDragOverRowKey(null);
  };

  const handleMoveRow = (rowKey: string, delta: -1 | 1) => {
    persistRowOrder(moveRowKeyByStep(rowOrder, rowKey, delta));
  };

  const handleSubmitReview = async () => {
    setSubmitting(true);
    setSubmitError(null);
    try {
      const manualList = [...manualItems.values()].filter((item) => item.source === 'manual');
      const confirmedList = [...manualItems.values()].filter((item) => item.source === 'confirmed_ok');
      const fixtureId = rows.find((row) => row.fixtureId)?.fixtureId;
      const created = await createLibraryReview({
        pageId: detail.pageId,
        pageLabel: detail.pageLabel,
        mm: detail.mm,
        slotCode: detail.slotCode,
        voiceName: detail.voiceName,
        captureFile: detail.captureFile,
        fixtureId,
        manualItems: manualList,
        confirmedOk: confirmedList,
        autoChecksSnapshot: buildAutoChecksSnapshot(rows),
      });
      setReviewId(created.id);
      setReviewUrl(created.url);
      onReviewIdChange(created.id);
    } catch (err) {
      setSubmitError(err instanceof Error ? err.message : 'Ошибка отправки');
    } finally {
      setSubmitting(false);
    }
  };

  const handleCopyLink = async () => {
    if (!reviewUrl) {
      return;
    }
    await navigator.clipboard.writeText(reviewUrl);
  };

  const handleCopyPrompt = async () => {
    const manualList = [...manualItems.values()].filter((item) => item.source === 'manual');
    const text = formatAgentPrompt(
      detail.voiceName,
      detail.slotCode,
      reviewUrl ?? window.location.href,
      manualList,
      buildAutoChecksSnapshot(rows),
    );
    await navigator.clipboard.writeText(text);
  };

  const handleDeleteReview = async () => {
    if (!reviewId) {
      return;
    }
    try {
      await deleteLibraryReview(reviewId);
      setReviewId(null);
      setReviewUrl(null);
      setManualItems(new Map());
      onReviewIdChange(null);
    } catch (err) {
      setSubmitError(err instanceof Error ? err.message : 'Ошибка удаления');
    }
  };

  if (rows.length === 0) {
    return <p className="empty-state">Каталог параметров пуст.</p>;
  }

  return (
    <>
      <LibraryReviewPanel
        autoSuspectCount={autoSuspectCount}
        manualCount={manualCount}
        resolvedCount={resolvedKeys.size}
        reviewUrl={reviewUrl}
        reviewId={reviewId}
        submitting={submitting}
        submitError={submitError}
        onSubmit={() => void handleSubmitReview()}
        onCopyLink={() => void handleCopyLink()}
        onCopyPrompt={() => void handleCopyPrompt()}
        onDeleteReview={() => void handleDeleteReview()}
      />

      <div
        className="library-voice-table-toolbar"
        id="library-voice-filters"
        ref={filtersToolbarRef}
      >
      <div className="params-filters">
        <label className="filter-field">
          <span>Group</span>
          <select value={groupQuery} onChange={(event) => setGroupQuery(event.target.value)}>
            <option value="">Все группы</option>
            {groups.map((group) => (
              <option key={group} value={group}>
                {group}
              </option>
            ))}
          </select>
        </label>

        <label className="filter-field">
          <span>SysEx GG</span>
          <select
            value={sysexGroupQuery}
            onChange={(event) => setSysexGroupQuery(event.target.value)}
          >
            <option value="">Все группы GG</option>
            {sysexGroups.map((group) => (
              <option key={group} value={String(group)}>
                0x{group.toString(16).toUpperCase().padStart(2, '0')}
              </option>
            ))}
          </select>
        </label>

        <label className="filter-field">
          <span>Bind</span>
          <select
            value={bindStatusFilter}
            onChange={(event) => setBindStatusFilter(event.target.value as BindStatusFilter)}
          >
            <option value="all">Все</option>
            <option value="manual_only">Только unbound</option>
            <option value="confirmed">Confirmed</option>
            <option value="candidate">Candidate</option>
          </select>
        </label>

        <label className="filter-field">
          <span>Parameter</span>
          <input
            type="search"
            value={tagQuery}
            placeholder="tag или название"
            onChange={(event) => setTagQuery(event.target.value)}
          />
        </label>

        <label className="filter-field">
          <span>Проблемы</span>
          <select
            value={issueFilter}
            onChange={(event) => setIssueFilter(event.target.value as IssueFilter)}
          >
            <option value="all">Все строки</option>
            <option value="issues">Любые проблемы</option>
            <option value="auto">Только авто</option>
            <option value="manual">Только ручные</option>
          </select>
        </label>

        <label className="filter-field filter-field--checkbox">
          <span>Только с значением в дампе</span>
          <input
            type="checkbox"
            checked={inDumpOnly}
            onChange={(event) => setInDumpOnly(event.target.checked)}
          />
        </label>

        <div className="filter-field library-row-order-actions">
          <span>Порядок</span>
          <div className="library-row-order-buttons">
            <button
              type="button"
              className="btn btn--ghost btn--tiny"
              onClick={() => persistRowOrder(buildSy99LcdOrder(allRowKeys))}
            >
              Строки: SY-99 LCD
            </button>
            <button
              type="button"
              className="btn btn--ghost btn--tiny"
              onClick={() => persistRowOrder([...allRowKeys])}
            >
              Строки: сброс
            </button>
            <button
              type="button"
              className="btn btn--ghost btn--tiny"
              onClick={() => {
                const next = [...DEFAULT_LIBRARY_COLUMN_ORDER];
                setColumnOrder(next);
                saveColumnOrder(next);
              }}
            >
              Колонки: сброс
            </button>
          </div>
        </div>
      </div>

      <p className="page-subtitle">
        {filteredRows.length} из {rows.length} строк · в дампе: {inDumpCount} · confirmed:{' '}
        {confirmedCount} · unbound: {unboundCount}
        {detail.catalogStats ? (
          <>
            {' '}
            · каталог: {detail.catalogStats.catalogParams} params / {detail.catalogStats.totalRows}{' '}
            rows
          </>
        ) : null}{' '}
        · строки: ↑↓ / ⋮⋮ · колонки: ⋮⋮ в шапке
      </p>
      </div>

      <div className="table-wrap library-params-table-wrap">
        <table className="params-table library-voice-params-table">
          <thead>
            <tr>
              <th className="library-row-order-col" title="Перетащите строку вверх/вниз">
                ↕
              </th>
              <th className="library-status-col" title="● зелёный confirmed · ● жёлтый candidate/unbound · ● серый manual_only">
                ●
              </th>
              {orderedColumns.map((column) => (
                <th
                  key={column.id}
                  className={column.mono ? 'mono library-col-draggable' : 'library-col-draggable'}
                  draggable
                  onDragStart={(event) => {
                    event.stopPropagation();
                    setDragColumnId(column.id);
                    event.dataTransfer.effectAllowed = 'move';
                  }}
                  onDragEnd={() => setDragColumnId(null)}
                  onDragOver={(event) => event.preventDefault()}
                  onDrop={(event) => {
                    event.preventDefault();
                    event.stopPropagation();
                    handleColumnDrop(column.id);
                  }}
                >
                  <span className="library-col-drag-handle" title="Перетащите колонку">
                    ⋮⋮
                  </span>{' '}
                  {column.label}
                </th>
              ))}
              <th className="library-flag-col" title="Отметить расхождение">
                ⚑
              </th>
            </tr>
          </thead>
          <tbody>
            {filteredRows.map((row) => {
              const rowKey = paramRowKey(row);
              const globalIndex = rowOrder.indexOf(rowKey);
              const canMoveUp = globalIndex > 0;
              const canMoveDown = globalIndex >= 0 && globalIndex < rowOrder.length - 1;

              return (
                <tr
                  key={rowKey}
                  className={rowClassName(row, manualItems, dragOverRowKey === rowKey)}
                  tabIndex={0}
                  onClick={() => navigate(`/params/${encodeURIComponent(row.id)}`)}
                  onKeyDown={(event) => {
                    if (event.key === 'Enter' || event.key === ' ') {
                      event.preventDefault();
                      navigate(`/params/${encodeURIComponent(row.id)}`);
                    }
                  }}
                  onDragOver={(event) => {
                    if (!dragRowKey) {
                      return;
                    }
                    event.preventDefault();
                    setDragOverRowKey(rowKey);
                  }}
                  onDragLeave={() => {
                    if (dragOverRowKey === rowKey) {
                      setDragOverRowKey(null);
                    }
                  }}
                  onDrop={(event) => {
                    event.preventDefault();
                    event.stopPropagation();
                    handleRowDrop(rowKey);
                  }}
                >
                  <td
                    className="library-row-order-col"
                    onClick={(event) => event.stopPropagation()}
                  >
                    <div className="library-row-order-controls">
                      <button
                        type="button"
                        className="library-row-move-btn"
                        title="Вверх"
                        disabled={!canMoveUp}
                        onClick={() => handleMoveRow(rowKey, -1)}
                      >
                        ↑
                      </button>
                      <span
                        className="library-row-drag-handle"
                        draggable
                        title="Перетащить строку"
                        onDragStart={(event) => {
                          event.stopPropagation();
                          setDragRowKey(rowKey);
                          event.dataTransfer.effectAllowed = 'move';
                        }}
                        onDragEnd={() => {
                          setDragRowKey(null);
                          setDragOverRowKey(null);
                        }}
                      >
                        ⋮⋮
                      </span>
                      <button
                        type="button"
                        className="library-row-move-btn"
                        title="Вниз"
                        disabled={!canMoveDown}
                        onClick={() => handleMoveRow(rowKey, 1)}
                      >
                        ↓
                      </button>
                    </div>
                  </td>
                  <td className="library-status-col">
                    <span
                      className={statusDotClass(row, manualItems)}
                      title={row.autoCheckMessage ?? row.autoCheckStatus ?? ''}
                    />
                  </td>
                  {orderedColumns.map((column) => {
                    const field = column.field;
                    const item =
                      field !== undefined
                        ? manualItems.get(reviewItemKey(row.id, row.elementIndex, field))
                        : undefined;
                    const isManualMismatch =
                      item?.source === 'manual' && item.appValue !== item.synthValue;
                    const isResolved =
                      field !== undefined &&
                      resolvedKeys.has(reviewItemKey(row.id, row.elementIndex, field));

                    return (
                      <td
                        key={column.id}
                        className={column.mono ? 'mono' : undefined}
                        onClick={
                          column.annotatable && field
                            ? (event) => openAnnotate(row, field, event)
                            : undefined
                        }
                        onKeyDown={
                          column.annotatable && field
                            ? (event) => {
                                if (event.key === 'Enter' || event.key === ' ') {
                                  openAnnotate(row, field, event);
                                }
                              }
                            : undefined
                        }
                        tabIndex={column.annotatable && field ? 0 : undefined}
                        role={column.annotatable && field ? 'button' : undefined}
                      >
                        <span className="library-value-cell">
                          <span>{column.getText(row)}</span>
                          {isManualMismatch && (
                            <span className="library-value--synth-report">{item.synthValue}</span>
                          )}
                          {isResolved && (
                            <span className="library-value--resolved" title="Исправлено">
                              ✓
                            </span>
                          )}
                        </span>
                      </td>
                    );
                  })}
                  <td className="library-flag-col">
                    <button
                      type="button"
                      className="library-flag-btn"
                      title="Отметить значение с SY-99"
                      onClick={(event) => {
                        event.stopPropagation();
                        openAnnotate(row, 'uiValue', event);
                      }}
                    >
                      ⚑
                    </button>
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>

      {annotateTarget && (
        <LibraryValuePopover
          appValue={annotateTarget.appValue}
          initialSynthValue={annotateTarget.hint}
          anchorRect={annotateTarget.anchorRect}
          onConfirm={(synthValue) => {
            const key = reviewItemKey(
              annotateTarget.paramId,
              annotateTarget.elementIndex,
              annotateTarget.field,
            );
            if (!synthValue) {
              setReviewItem(null, key);
            } else {
              setReviewItem(
                {
                  source: 'manual',
                  paramId: annotateTarget.paramId,
                  elementIndex: annotateTarget.elementIndex,
                  field: annotateTarget.field,
                  appValue: annotateTarget.appValue,
                  synthValue,
                },
                key,
              );
            }
            setAnnotateTarget(null);
          }}
          onConfirmMatch={() => {
            const key = reviewItemKey(
              annotateTarget.paramId,
              annotateTarget.elementIndex,
              annotateTarget.field,
            );
            setReviewItem(
              {
                source: 'confirmed_ok',
                paramId: annotateTarget.paramId,
                elementIndex: annotateTarget.elementIndex,
                field: annotateTarget.field,
                appValue: annotateTarget.appValue,
                synthValue: annotateTarget.appValue,
              },
              key,
            );
            setAnnotateTarget(null);
          }}
          onClose={() => setAnnotateTarget(null)}
        />
      )}
    </>
  );
}

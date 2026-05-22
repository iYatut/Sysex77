import { useEffect, useMemo, useRef, useState } from 'react';
import type {
  DisplayValueMapRow,
  LogRowOverlay,
  ParamValueMap,
  PresentationKind,
} from '../types/paramValueMap';
import { MidiNoteSelect } from './MidiNoteSelect';
import { PRESENTATION_KIND_OPTIONS } from '../types/paramValueMap';
import {
  createBlankStoredRow,
  displayRowsToStored,
  effectiveCustomRows,
  importRowsFromObservationLog,
  isCustomValueMap,
  META_FULL_TABLE_THRESHOLD,
  rowsForValueMapTable,
  updateStoredRow,
  verificationForPersist,
  type ValueMapTableContext,
  type ValueMapTableMode,
} from '../utils/customValueMap';
import { formatElementLabel, sy99LcdItemNumber } from '../utils/elementLabel';
import { midiNoteToName } from '../utils/midiNotes';
import type { ParamMeta } from '../types/paramMeta';
import type { StoredValueMapRow } from '../types/paramMeta';
import { collectProgramLabelSuggestions } from '../utils/programLabelSuggestions';
import { prefillProgramLabelsFromTemplate } from '../utils/prefillProgramLabels';
import type { LogDuplicateEntry } from '../utils/parseSysexLog';
import {
  emptyParamVerificationState,
  applyCanonicalRaw,
  type ParamVerificationState,
  type RowOverride,
  wipedParamVerificationState,
} from '../utils/paramVerification';
import {
  compareLogWithValueMap,
  dedupeLogText,
  duplicateReasonLabel,
  findLogDuplicates,
  logLineMatchesParamAddress,
  logMatchStatusLabel,
  parseSysexLogText,
  type ParsedLogLine,
} from '../utils/parseSysexLog';

type ParamValueMapTableProps = {
  param: ParamMeta | null;
  paramId: string;
  elementIndex: number;
  /** raw/ui 0…127 как MIDI-ноты (ENLL, ENLH) */
  useMidiNotes?: boolean;
  valueMap: ParamValueMap | null;
  activeRaw: number | null;
  verification: ParamVerificationState;
  onVerificationChange: (state: ParamVerificationState) => void;
  onPersistVerification: (state: ParamVerificationState) => Promise<void>;
  onRefetchValueMap?: () => Promise<ParamValueMap | null>;
  verifySaving?: boolean;
  loading?: boolean;
  error?: string | null;
  /** Внутри панели «Правда SY99» — таблица только справочник, без второй «истины». */
  embeddedInTruthPanel?: boolean;
};

function parsePresentationKind(value: string): PresentationKind {
  if (
    value === 'decoded' ||
    value === 'namedEnum' ||
    value === 'text'
  ) {
    return value;
  }
  return 'numeric';
}

function mergeDisplayRows(
  valueMap: ParamValueMap,
  verification: ParamVerificationState,
  useMidiNotes: boolean,
  tableCtx: ValueMapTableContext,
): DisplayValueMapRow[] {
  const custom = isCustomValueMap(verification);
  const baseRows = rowsForValueMapTable(valueMap, verification, tableCtx).rows;

  return baseRows.map((row) => {
    const ov = custom ? undefined : verification.rowOverrides[String(row.raw)];
    const sysexHex = ov?.sysexHex ?? row.sysexHex;
    const programLabel =
      ov?.programLabel ??
      (useMidiNotes && !custom ? midiNoteToName(row.raw) : row.programLabel);
    const ui = ov?.ui ?? row.ui;
    const presentationKind = ov?.presentationKind ?? row.presentationKind;
    const isEdited =
      !custom &&
      ov !== undefined &&
      (ov.sysexHex !== undefined ||
        ov.programLabel !== undefined ||
        ov.ui !== undefined ||
        ov.presentationKind !== undefined);

    return {
      ...row,
      sysexHex,
      programLabel,
      ui,
      presentationKind,
      isEdited,
      isVerified: verification.verifiedRaws.includes(row.raw),
    };
  });
}

function valueMapFromDisplayRows(
  valueMap: ParamValueMap,
  displayRows: DisplayValueMapRow[],
): ParamValueMap {
  return {
    ...valueMap,
    rows: displayRows.map(({ isEdited, isVerified, ...row }) => row),
  };
}

export function ParamValueMapTable({
  param,
  paramId,
  elementIndex,
  useMidiNotes = false,
  valueMap,
  activeRaw,
  verification,
  onVerificationChange,
  onPersistVerification,
  onRefetchValueMap,
  verifySaving = false,
  loading = false,
  error = null,
  embeddedInTruthPanel = false,
}: ParamValueMapTableProps) {
  const [filter, setFilter] = useState('');
  const [confirmMessage, setConfirmMessage] = useState<string | null>(null);
  const [saveMessage, setSaveMessage] = useState<string | null>(null);
  const [logDuplicates, setLogDuplicates] = useState<LogDuplicateEntry[]>([]);
  const [logComparison, setLogComparison] = useState<{
    overlaysByRaw: Map<number, LogRowOverlay>;
    extras: ParsedLogLine[];
  } | null>(null);
  const [compareEngine, setCompareEngine] = useState<'api' | 'client' | null>(null);
  const [selectedRaws, setSelectedRaws] = useState<Set<number>>(() => new Set());
  const selectAllRef = useRef<HTMLInputElement>(null);

  const logText = verification.observationLog;

  useEffect(() => {
    setSelectedRaws(new Set());
  }, [paramId, elementIndex]);

  const tableCtx = useMemo(() => {
    if (!valueMap || !param) {
      return null;
    }
    return { param, elementIndex, valueMap };
  }, [param, elementIndex, valueMap]);

  const tableMode: ValueMapTableMode = useMemo(() => {
    if (!tableCtx) {
      return 'meta-full';
    }
    return rowsForValueMapTable(valueMap!, verification, tableCtx).mode;
  }, [tableCtx, valueMap, verification]);

  const displayRows = useMemo(() => {
    if (!tableCtx) {
      return [];
    }
    let rows = mergeDisplayRows(valueMap!, verification, useMidiNotes, tableCtx);
    if (
      tableMode === 'meta-full' &&
      selectedRaws.size > 0 &&
      selectedRaws.size < rows.length
    ) {
      rows = rows.filter((row) => selectedRaws.has(row.raw));
    }
    return rows;
  }, [tableCtx, valueMap, verification, useMidiNotes, tableMode, selectedRaws]);

  useEffect(() => {
    if (!valueMap || !logText.trim() || !param) {
      setLogDuplicates([]);
      setLogComparison(null);
      setCompareEngine(null);
      return;
    }

    let cancelled = false;
    const timer = window.setTimeout(() => {
      const parsed = parseSysexLogText(logText).filter((line) =>
        logLineMatchesParamAddress(line, param.addr, elementIndex, param.perElement),
      );
      const mapForCompare = valueMapFromDisplayRows(valueMap, displayRows);

      if (cancelled) {
        return;
      }

      setLogDuplicates(findLogDuplicates(parsed));
      setLogComparison(compareLogWithValueMap(parsed, mapForCompare));
      setCompareEngine('client');
    }, 400);

    return () => {
      cancelled = true;
      window.clearTimeout(timer);
    };
  }, [logText, valueMap, displayRows, param, elementIndex]);

  const filteredRows = useMemo(() => {
    const q = filter.trim();
    if (!q) {
      return displayRows;
    }

    const asNumber = Number.parseInt(q, 10);
    const isNumeric = !Number.isNaN(asNumber);

    return displayRows.filter((row) => {
      if (isNumeric && (row.raw === asNumber || row.ui === asNumber || row.index === asNumber)) {
        return true;
      }

      const haystack = [
        String(row.raw),
        String(row.ui),
        row.programLabel,
        row.registryLabel ?? '',
        row.sysexHex,
        row.presentationKind,
      ]
        .join(' ')
        .toLowerCase();

      return haystack.includes(q.toLowerCase());
    });
  }, [filter, displayRows]);

  const programLabelListId = `program-labels-${paramId}-el${elementIndex}`;

  const programLabelSuggestions = useMemo(
    () => collectProgramLabelSuggestions(param, verification, displayRows),
    [param, verification, displayRows],
  );

  const filteredRaws = useMemo(() => filteredRows.map((row) => row.raw), [filteredRows]);

  const selectedInFilterCount = useMemo(
    () => filteredRaws.filter((raw) => selectedRaws.has(raw)).length,
    [filteredRaws, selectedRaws],
  );

  const allFilteredSelected =
    filteredRaws.length > 0 && selectedInFilterCount === filteredRaws.length;
  const someFilteredSelected =
    selectedInFilterCount > 0 && selectedInFilterCount < filteredRaws.length;

  useEffect(() => {
    if (selectAllRef.current) {
      selectAllRef.current.indeterminate = someFilteredSelected;
    }
  }, [someFilteredSelected, allFilteredSelected]);

  const activeRow = useMemo(() => {
    if (activeRaw === null) {
      return null;
    }
    return displayRows.find((row) => row.raw === activeRaw) ?? null;
  }, [activeRaw, displayRows]);

  function patchVerification(patch: Partial<ParamVerificationState>) {
    onVerificationChange({ ...verification, ...patch });
    setConfirmMessage(null);
    setSaveMessage(null);
  }

  async function handleSaveEdits() {
    setSaveMessage(null);
    setConfirmMessage(null);

    if (!tableCtx) {
      return;
    }

    const toSave = verificationForPersist(verification, displayRows, {
      ...tableCtx,
      selectedRaws,
    });

    if (toSave.customRows === null && displayRows.length > META_FULL_TABLE_THRESHOLD) {
      setSaveMessage(
        'Не сохранено: слишком много строк Meta. Отметьте ✓, выберите строки слева, заполните журнал или «Импорт из лога».',
      );
      return;
    }

    try {
      await onPersistVerification(toSave);
      onVerificationChange(toSave);
      const rowCount = toSave.customRows?.length ?? displayRows.length;
      setSaveMessage(
        `Список сохранён: ${paramId} эл.${formatElementLabel(elementIndex)} — ${rowCount} строк, ${toSave.verifiedRaws.length} сверено.`,
      );
    } catch {
      setSaveMessage('Не удалось сохранить — проверьте, что Sysex77.exe запущен (API :8765).');
    }
  }

  /** Короткий список (журнал / ✓ / выбранные), не весь rawMin…rawMax. */
  function stateWithSavedRows(): ParamVerificationState {
    if (!tableCtx) {
      return verification;
    }
    const base = verificationForPersist(verification, displayRows, {
      ...tableCtx,
      selectedRaws,
    });
    if (effectiveCustomRows(base) !== null) {
      return base;
    }
    if (displayRows.length > 0) {
      return { ...verification, customRows: displayRowsToStored(displayRows), rowOverrides: {} };
    }
    return verification;
  }

  function patchSavedRow(raw: number, patch: Partial<StoredValueMapRow>) {
    const base = stateWithSavedRows();
    const rows = effectiveCustomRows(base);
    if (rows === null) {
      return;
    }
    patchVerification({
      ...base,
      customRows: updateStoredRow(rows, raw, patch),
      confirmed: false,
      confirmedAt: null,
    });
  }

  function setRowOverride(
    raw: number,
    field: keyof RowOverride,
    value: string | number | PresentationKind,
  ) {
    patchSavedRow(raw, { [field]: value } as Partial<StoredValueMapRow>);
  }

  function patchRowOverride(raw: number, patch: RowOverride) {
    patchSavedRow(raw, patch);
  }

  function clearRowOverride(raw: number) {
    const base = stateWithSavedRows();
    if (base.customRows === null) {
      return;
    }
    patchVerification({
      ...base,
      customRows: base.customRows.filter((r) => r.raw !== raw),
      verifiedRaws: base.verifiedRaws.filter((r) => r !== raw),
      confirmed: false,
      confirmedAt: null,
    });
  }

  function toggleSelected(raw: number) {
    setSelectedRaws((prev) => {
      const next = new Set(prev);
      if (next.has(raw)) {
        next.delete(raw);
      } else {
        next.add(raw);
      }
      return next;
    });
  }

  function toggleSelectAllFiltered() {
    setSelectedRaws((prev) => {
      const next = new Set(prev);
      if (allFilteredSelected) {
        for (const raw of filteredRaws) {
          next.delete(raw);
        }
      } else {
        for (const raw of filteredRaws) {
          next.add(raw);
        }
      }
      return next;
    });
  }

  async function handleRebuildFromMeta() {
    const ok = window.confirm(
      'Пересобрать список из Meta (rawMin/rawMax)?\n\nТекущие строки в таблице будут заменены.',
    );
    if (!ok) {
      return;
    }

    const next: ParamVerificationState = {
      ...verification,
      customRows: null,
      rowOverrides: {},
      confirmed: false,
      confirmedAt: null,
    };
    onVerificationChange(next);
    setSelectedRaws(new Set());

    try {
      await onPersistVerification(next);
      if (onRefetchValueMap) {
        await onRefetchValueMap();
      }
      setSaveMessage('Список пересобран из rawMin/rawMax (Sysex77).');
    } catch {
      setSaveMessage('Не сохранено на сервере.');
    }
  }

  async function handleAddRow() {
    if (!valueMap) {
      return;
    }

    const base = stateWithSavedRows();
    const customRows = [...(base.customRows ?? [])];
    customRows.push(createBlankStoredRow(valueMap, customRows));
    const next: ParamVerificationState = {
      ...base,
      customRows,
      confirmed: false,
      confirmedAt: null,
    };
    onVerificationChange(next);

    try {
      await onPersistVerification(next);
      setSaveMessage('Строка добавлена.');
    } catch {
      setSaveMessage('Не сохранено на сервере.');
    }
  }

  async function handleClearSelectedRows() {
    if (selectedRaws.size === 0) {
      setSaveMessage('Отметьте строки галочкой слева (или «выбрать все» в заголовке).');
      return;
    }

    const count = selectedRaws.size;
    const ok = window.confirm(`Удалить ${count} строк из списка?`);
    if (!ok) {
      return;
    }

    const base = stateWithSavedRows();
    const toDelete = selectedRaws;
    const next: ParamVerificationState = {
      ...base,
      customRows: (base.customRows ?? []).filter((r) => !toDelete.has(r.raw)),
      verifiedRaws: base.verifiedRaws.filter((r) => !toDelete.has(r)),
      confirmed: false,
      confirmedAt: null,
    };
    onVerificationChange(next);
    setSelectedRaws(new Set());

    try {
      await onPersistVerification(next);
      setSaveMessage(`Удалено строк: ${count}. Сохранено в params_meta.json.`);
    } catch {
      setSaveMessage('Не сохранено на сервере — перезапустите Sysex77.exe.');
    }
  }

  function toggleRawVerified(raw: number) {
    const set = new Set(verification.verifiedRaws);
    if (set.has(raw)) {
      set.delete(raw);
    } else {
      set.add(raw);
    }
    patchVerification({
      verifiedRaws: [...set].sort((a, b) => a - b),
      confirmed: false,
      confirmedAt: null,
    });
  }

  function applyLogMatchesToVerified() {
    if (!logComparison) {
      return;
    }
    const set = new Set(verification.verifiedRaws);
    for (const [raw, overlay] of logComparison.overlaysByRaw) {
      if (overlay.status === 'match') {
        set.add(raw);
      }
    }
    patchVerification({ verifiedRaws: [...set].sort((a, b) => a - b) });
  }

  async function handleConfirmParam() {
    if (logDuplicates.length > 0) {
      setConfirmMessage('В журнале есть дубликаты — устраните их перед подтверждением.');
      return;
    }

    const checkedRaws = displayRows.filter((row) => row.isVerified).map((row) => row.raw);

    let truthRaw: number | null = null;

    if (checkedRaws.length === 1) {
      truthRaw = checkedRaws[0];
    } else if (checkedRaws.length > 1) {
      window.alert(
        'Отметьте ✓ ровно одну строку (режим на SY99). Сейчас отмечено: ' + checkedRaws.join(', '),
      );
      return;
    } else if (verification.canonicalRaw !== null) {
      // Уже сохранено в панели «Сверка» — не подменять dump (activeRaw).
      truthRaw = verification.canonicalRaw;
    } else if (activeRaw !== null) {
      truthRaw = activeRaw;
    } else {
      window.alert(
        'Отметьте ✓ ровно одну строку, сохраните raw в панели «Сверка», или дождитесь dump после Sync.',
      );
      return;
    }

    const next = applyCanonicalRaw(verification, truthRaw);
    onVerificationChange(next);
    setLogComparison(null);

    try {
      await onPersistVerification(next);
      setConfirmMessage(`Запомнен raw ${truthRaw} (журнал и таблица сохранены).`);
    } catch {
      setConfirmMessage('Подтверждение не сохранено на сервере — запустите Sysex77.exe.');
    }
  }

  async function handleResetVerification() {
    const ok = window.confirm('Сбросить правки и галочки ✓ (журнал останется)?');
    if (!ok) {
      return;
    }

    const empty = emptyParamVerificationState();
    onVerificationChange(empty);

    try {
      await onPersistVerification(empty);
      setConfirmMessage(null);
      setSaveMessage(null);
    } catch {
      setSaveMessage('Сброс не сохранён на сервере.');
    }
  }

  function handleRemoveDuplicatesFromLog() {
    const { text, removedCount } = dedupeLogText(logText);
    patchVerification({ observationLog: text, confirmed: false, confirmedAt: null });
    setConfirmMessage(null);
    if (removedCount > 0) {
      setSaveMessage(`Из журнала удалено повторов: ${removedCount}`);
    } else {
      setSaveMessage('Повторов SysEx в журнале не найдено');
    }
  }

  async function handleWipeAndStartFresh() {
    const ok = window.confirm(
      'Стереть таблицу и журнал?\n\n' +
        '• все строки карты (в т.ч. ошибочные 128)\n' +
        '• журнал наблюдений\n' +
        '• сверка и правки\n\n' +
        'Таблица станет пустой. Дальше: «Импорт из лога» или «+ Строка».',
    );
    if (!ok) {
      return;
    }

    const wiped = wipedParamVerificationState();
    onVerificationChange(wiped);
    setSelectedRaws(new Set());
    setLogComparison(null);

    try {
      await onPersistVerification(wiped);
      setConfirmMessage(null);
      setSaveMessage('Таблица и журнал очищены. Список пуст — соберите заново.');
    } catch {
      setSaveMessage('Сброс не сохранён на сервере — перезапустите Sysex77.exe.');
    }
  }

  function handlePrefillProgramLabels() {
    if (!param || !valueMap) {
      return;
    }

    const result = prefillProgramLabelsFromTemplate(param, elementIndex, verification, valueMap);
    if (!result.changed || result.templateElementIndex === null) {
      setSaveMessage('Нет подписей Program на других element для копирования.');
      return;
    }

    patchVerification(result.state);
    setSaveMessage(
      `Подписи Program/Вид с эл. ${formatElementLabel(result.templateElementIndex)} ` +
        '(кадры SysEx не менялись) — нажмите «Сохранить».',
    );
  }

  async function handleImportFromLog() {
    if (!param || !valueMap) {
      return;
    }

    const result = importRowsFromObservationLog(logText, {
      param,
      elementIndex,
      valueMap,
    });

    if (result.rows.length === 0) {
      const hint =
        result.skippedWrongAddress > 0
          ? ` (отфильтровано ${result.skippedWrongAddress} строк других параметров)`
          : '';
      setSaveMessage(`В журнале нет строк SysEx для этого параметра${hint}.`);
      return;
    }

    const next: ParamVerificationState = {
      ...verification,
      customRows: result.rows,
      rowOverrides: {},
      observationLog: result.dedupedLog,
      confirmed: false,
      confirmedAt: null,
    };
    onVerificationChange(next);
    setLogComparison(null);

    const parts: string[] = [
      `Импортировано ${result.rows.length} значений (raw: ${result.rows.map((r) => r.raw).join(', ')}).`,
    ];
    if (result.removedDuplicateLines > 0) {
      parts.push(`Убрано повторов SysEx: ${result.removedDuplicateLines}.`);
    }
    if (result.skippedWrongAddress > 0) {
      parts.push(`Пропущено чужих адресов: ${result.skippedWrongAddress}.`);
    }
    if (result.rawConflicts.length > 0) {
      parts.push(
        `Конфликт raw (оставлен первый SysEx): ${result.rawConflicts.map((c) => c.raw).join(', ')}.`,
      );
    }

    try {
      await onPersistVerification(next);
      setSaveMessage(parts.join(' '));
    } catch {
      setSaveMessage('Импорт не сохранён на сервере.');
    }
  }

  function handleClearLogOnly() {
    patchVerification({ observationLog: '', confirmed: false, confirmedAt: null });
    setLogComparison(null);
    setSaveMessage('Журнал очищен (нажмите «Сохранить» для записи в файл).');
  }

  function overlayForRow(row: DisplayValueMapRow): LogRowOverlay | undefined {
    return logComparison?.overlaysByRaw.get(row.raw);
  }

  function rowClassName(row: DisplayValueMapRow, overlay?: LogRowOverlay): string {
    const classes = ['value-map-row'];
    const isTruth = verification.canonicalRaw !== null && row.raw === verification.canonicalRaw;

    if (isTruth) {
      classes.push('row--param-confirmed');
    }
    if (row.isVerified) {
      classes.push('row--verified');
    }
    if (row.isEdited) {
      classes.push('row--edited');
    }
    if (activeRaw !== null && row.raw === activeRaw) {
      if (verification.canonicalRaw === null || verification.canonicalRaw === activeRaw) {
        classes.push('row--active');
      } else if (!isTruth) {
        classes.push('row--dump-mismatch');
      }
    }
    if (row.labelMismatch) {
      classes.push('row--mismatch');
    }
    if (overlay?.status && overlay.status !== 'match') {
      classes.push(`row--log-${overlay.status}`);
    }
    return classes.join(' ');
  }

  const sectionClass = embeddedInTruthPanel
    ? 'value-map-panel value-map-panel--embedded'
    : 'panel value-map-panel';

  return (
    <div className={sectionClass}>
      <div className="value-map-panel__header">
        <div>
          <h3>{embeddedInTruthPanel ? 'Справочник SysEx' : 'Карта значений (окна Sysex77)'}</h3>
          <p className="panel-hint">
            {embeddedInTruthPanel
              ? `${paramId} · эл. ${formatElementLabel(elementIndex)} — сверка здесь: raw + Program совпадают со списком в «Сверка». ✓ одна строка → «Сохранить правду».`
              : `${paramId} · эл. ${formatElementLabel(elementIndex)} — один список в params_meta.json. «Импорт из лога» — только кадры из журнала. «Пересобрать из Meta» — полный rawMin…rawMax (128), только если нужен`}
            {valueMap && (
              <>
                {' '}
                · сейчас raw {valueMap.rawMin}…{valueMap.rawMax}
              </>
            )}
            {valueMap?.uiMin !== undefined && valueMap?.uiMax !== undefined && (
              <> · UI {valueMap.uiMin}…{valueMap.uiMax}</>
            )}
            {compareEngine === 'api' && ' (движок: Sysex77)'}
            {compareEngine === 'client' && ' (движок: офлайн, API недоступен)'}
          </p>
        </div>
        {!embeddedInTruthPanel && verification.canonicalRaw !== null && (
          <span className="verify-badge verify-badge--confirmed">Правда ✓</span>
        )}
      </div>

      {tableMode === 'meta-full' &&
        valueMap &&
        valueMap.rows.length > META_FULL_TABLE_THRESHOLD && (
          <div className="alert">
            Показан полный диапазон Meta ({valueMap.rows.length} строк). Короткий список: журнал
            SY99 → «Импорт из лога», галочки ✓ или выбор строк слева → «Сохранить».
          </div>
        )}

      {verification.canonicalRaw !== null &&
        activeRaw !== null &&
        verification.canonicalRaw !== activeRaw && (
          <div className="alert">
            Запомнено raw {verification.canonicalRaw}
            {useMidiNotes ? ` (${midiNoteToName(verification.canonicalRaw)})` : ''}, dump после
            Sync — raw {activeRaw}
            {useMidiNotes ? ` (${midiNoteToName(activeRaw)})` : ''}. Золотая строка — ваша правда;
            серая — только dump (не совпадает с SY99 на синте — обновите Sync или проверьте
            элемент).
          </div>
        )}

      {loading && <div className="alert">Загрузка карты…</div>}
      {error && <div className="alert alert--error">{error}</div>}
      {saveMessage && <div className="alert alert--success">{saveMessage}</div>}
      {confirmMessage && (
        <div
          className={
            confirmMessage.startsWith('Запомнен') || confirmMessage.includes('отмечен')
              ? 'alert alert--success'
              : 'alert alert--error'
          }
        >
          {confirmMessage}
        </div>
      )}

      {activeRow && !embeddedInTruthPanel && (
        <div className="value-map-current">
          <h3>Текущее значение (dump)</h3>
          <p className="mono value-map-current__sysex">{activeRow.sysexHex}</p>
          <p>
            raw <span className="mono">{activeRow.raw}</span> → окно:{' '}
            <strong className="mono">{activeRow.programLabel}</strong>
          </p>
        </div>
      )}

      {valueMap && displayRows.length === 0 && (
        <div className="alert">
          Таблица пуста — ввод здесь: вставьте кадры в журнал ниже →{' '}
          <strong>Импорт из лога</strong>, или <strong>+ Строка</strong>, или{' '}
          <strong>Пересобрать из Meta</strong> (все raw {valueMap.rawMin}…{valueMap.rawMax}).
        </div>
      )}

      {valueMap && (
        <div className="value-map-workspace">
          <div className="value-map-workspace__main">
            <label className="form-field value-map-log">
              <span className="value-map-log__label-row">
                <span>Наблюдённый лог SY99</span>
                <button
                  type="button"
                  className="btn btn--ghost btn--tiny"
                  onClick={handleClearLogOnly}
                >
                  Очистить журнал
                </button>
              </span>
              <textarea
                rows={10}
                placeholder={'[#1] SysEx: F0 43 10 34 02 00 00 2B 00 7E F7'}
                value={logText}
                onChange={(event) =>
                  patchVerification({
                    observationLog: event.target.value,
                    confirmed: false,
                    confirmedAt: null,
                  })
                }
              />
            </label>

          {logDuplicates.length > 0 && (
            <div className="log-duplicates-panel">
              <div className="log-duplicates-panel__header">
                <h3>Дубликаты в журнале ({logDuplicates.length})</h3>
                <button
                  type="button"
                  className="btn btn--ghost"
                  onClick={handleRemoveDuplicatesFromLog}
                >
                  Убрать дубликаты из журнала
                </button>
              </div>
              <p className="panel-hint">
                Повтор [#N] или тот же SysEx (часто двойной щелчок колеса). Кнопка оставляет только
                первое вхождение каждого кадра.
              </p>
              <div className="table-wrap">
                <table className="params-table log-duplicates-table">
                  <thead>
                    <tr>
                      <th>строка</th>
                      <th>[#N]</th>
                      <th>причина</th>
                      <th>SysEx</th>
                      <th>raw</th>
                    </tr>
                  </thead>
                  <tbody>
                    {logDuplicates.map((dup, i) => (
                      <tr key={`${dup.line.lineIndex}-${i}`} className="log-duplicate-row">
                        <td className="mono">{dup.line.lineIndex + 1}</td>
                        <td className="mono">{dup.line.logIndex ?? '—'}</td>
                        <td>{duplicateReasonLabel(dup.reason)}</td>
                        <td className="mono sysex-cell">{dup.line.sysexHex}</td>
                        <td className="mono">{dup.line.raw}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>
          )}

          {logComparison && logComparison.extras.length > 0 && (
            <div className="alert alert--error">
              В журнале есть raw, которых нет в таблице:{' '}
              {logComparison.extras.map((e) => e.raw).join(', ')}.
              {displayRows.length === 0 ? (
                <> Нажмите «Импорт из лога», чтобы построить список из журнала.</>
              ) : (
                <> Удалите лишнее из журнала или добавьте строки в таблицу.</>
              )}
            </div>
          )}

          <datalist id={programLabelListId}>
            {programLabelSuggestions.map((label) => (
              <option key={label} value={label} />
            ))}
          </datalist>

          <div className="table-wrap value-map-table-wrap">
            <table className="params-table value-map-table">
              <thead>
                <tr>
                  <th className="value-map-col-select" title="Выбрать строки для очистки">
                    <input
                      ref={selectAllRef}
                      type="checkbox"
                      checked={allFilteredSelected}
                      onChange={toggleSelectAllFiltered}
                      aria-label="Выбрать все строки в фильтре"
                    />
                  </th>
                  <th className="value-map-col-verify" title="Сверено вручную">
                    ✓
                  </th>
                  <th
                    className="value-map-col-index"
                    title={
                      paramId === 'ELMODE'
                        ? 'Номер пункта SY99 (01…11)'
                        : useMidiNotes
                          ? 'raw = MIDI-нота (0=C-2 … 127=G8)'
                          : 'Индекс строки в списке'
                    }
                  >
                    {paramId === 'ELMODE' ? 'SY99' : useMidiNotes ? 'raw' : '#'}
                  </th>
                  <th>SysEx (редакт.)</th>
                  <th>raw</th>
                  <th>{useMidiNotes ? 'нота (ui/raw)' : 'ui (редакт.)'}</th>
                  <th>Registry</th>
                  <th title="Подпись как на экране SY99 (напр. off)">Program (редакт.)</th>
                  <th title="Вид подписи: text — своя строка с SY99">Вид</th>
                  {logText.trim() ? <th>Лог</th> : null}
                  <th />
                </tr>
              </thead>
              <tbody>
                {filteredRows.map((row) => {
                  const overlay = overlayForRow(row);
                  return (
                    <tr key={row.raw} className={rowClassName(row, overlay)}>
                      <td className="value-map-col-select">
                        <input
                          type="checkbox"
                          checked={selectedRaws.has(row.raw)}
                          onChange={() => toggleSelected(row.raw)}
                          title="Выбрать для очистки"
                          aria-label={`Выбрать raw ${row.raw}`}
                        />
                      </td>
                      <td className="value-map-col-verify">
                        <button
                          type="button"
                          className={
                            row.isVerified
                              ? 'verify-mark verify-mark--on'
                              : 'verify-mark'
                          }
                          onClick={() => toggleRawVerified(row.raw)}
                          title={row.isVerified ? 'Сверено — снять' : 'Отметить сверенным'}
                        >
                          ✓
                        </button>
                      </td>
                      <td
                        className="mono value-map-col-index"
                        title={
                          paramId === 'ELMODE'
                            ? `пункт ${sy99LcdItemNumber(row.raw)} · raw SysEx = ${row.raw}`
                            : useMidiNotes
                              ? `raw ${row.raw} · ${midiNoteToName(row.raw)}`
                              : `строка ${row.index} · raw ${row.raw}`
                        }
                      >
                        {paramId === 'ELMODE'
                          ? sy99LcdItemNumber(row.raw)
                          : useMidiNotes
                            ? row.raw
                            : row.index}
                      </td>
                      <td className="sysex-cell-td">
                        <input
                          type="text"
                          className="cell-input cell-input--sysex mono"
                          value={row.sysexHex}
                          title={row.sysexHex}
                          spellCheck={false}
                          onChange={(e) => setRowOverride(row.raw, 'sysexHex', e.target.value)}
                        />
                      </td>
                      <td className="mono">{row.raw}</td>
                      <td>
                        {useMidiNotes ? (
                          <MidiNoteSelect
                            value={row.ui}
                            onChange={(note) => {
                              const rowPatch: RowOverride = { ui: note };
                              if (!verification.rowOverrides[String(row.raw)]?.programLabel) {
                                rowPatch.programLabel = midiNoteToName(note);
                                rowPatch.presentationKind = 'text';
                              }
                              patchRowOverride(row.raw, rowPatch);
                            }}
                          />
                        ) : (
                          <input
                            type="number"
                            className="cell-input mono cell-input--narrow"
                            value={row.ui}
                            onChange={(e) =>
                              setRowOverride(row.raw, 'ui', Number(e.target.value))
                            }
                          />
                        )}
                      </td>
                      <td className="mono">{row.registryLabel ?? '—'}</td>
                      <td>
                        <input
                          type="text"
                          className="cell-input mono"
                          list={programLabelListId}
                          value={row.programLabel}
                          placeholder={
                            row.presentationKind === 'text' ? 'напр. off' : undefined
                          }
                          title={
                            programLabelSuggestions.length > 0
                              ? `Подпись SY99 (${programLabelSuggestions.length} вариантов из других element)`
                              : 'Подпись значения — сохраните на el.1, подсказки появятся на el.2'
                          }
                          autoComplete="on"
                          onChange={(e) =>
                            setRowOverride(row.raw, 'programLabel', e.target.value)
                          }
                        />
                      </td>
                      <td>
                        <select
                          className={`cell-input cell-input--kind kind-select kind-select--${row.presentationKind}`}
                          value={row.presentationKind}
                          title="Как отображать подпись на SY99"
                          onChange={(e) =>
                            setRowOverride(
                              row.raw,
                              'presentationKind',
                              parsePresentationKind(e.target.value),
                            )
                          }
                        >
                          {PRESENTATION_KIND_OPTIONS.map((opt) => (
                            <option key={opt.value} value={opt.value}>
                              {opt.value}
                            </option>
                          ))}
                        </select>
                      </td>
                      {logText.trim() ? (
                        <td className="mono log-status-cell">
                          {overlay ? (
                            <span className={`log-status log-status--${overlay.status}`}>
                              {logMatchStatusLabel(overlay.status ?? 'match')}
                            </span>
                          ) : (
                            <span className="log-status log-status--missing">—</span>
                          )}
                        </td>
                      ) : null}
                      <td>
                        {row.isEdited && (
                          <button
                            type="button"
                            className="btn btn--ghost btn--tiny"
                            onClick={() => clearRowOverride(row.raw)}
                          >
                            сброс
                          </button>
                        )}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
          </div>

          <aside className="value-map-workspace__rail" aria-label="Действия и фильтр">
            <div className="value-map-actions value-map-actions--rail">
              {embeddedInTruthPanel && (
                <button
                  type="button"
                  className="btn btn--primary"
                  onClick={() => void handleConfirmParam()}
                  disabled={verifySaving || displayRows.length === 0}
                  title="Одна ✓ в таблице → единственная правда"
                >
                  {verifySaving ? 'Сохранение…' : 'Сохранить правду'}
                </button>
              )}
              <button
                type="button"
                className="btn btn--ghost btn--danger"
                onClick={() => void handleWipeAndStartFresh()}
                disabled={verifySaving}
                title="Пустая таблица + пустой журнал (без 128 строк)"
              >
                Стереть и начать заново
              </button>
              <button
                type="button"
                className="btn btn--ghost"
                onClick={() => void handleImportFromLog()}
                disabled={verifySaving || !logText.trim()}
                title="Построить таблицу только из строк в журнале ниже"
              >
                Импорт из лога
              </button>
              {param?.perElement && (
                <button
                  type="button"
                  className="btn btn--ghost"
                  onClick={handlePrefillProgramLabels}
                  disabled={verifySaving || !valueMap}
                  title="Скопировать только Program и Вид с другого element; SysEx не трогаем"
                >
                  Подписи с эл.…
                </button>
              )}
              <button
                type="button"
                className="btn btn--ghost"
                onClick={() => void handleRebuildFromMeta()}
                disabled={verifySaving}
                title="Все raw из Meta rawMin…rawMax (часто 128 — только если нужно)"
              >
                Пересобрать из Meta
              </button>
              <button
                type="button"
                className="btn btn--ghost"
                onClick={() => void handleAddRow()}
                disabled={verifySaving}
              >
                + Строка
              </button>
              <button
                type="button"
                className="btn btn--primary"
                onClick={() => void handleSaveEdits()}
                disabled={verifySaving}
              >
                {verifySaving ? 'Сохранение…' : 'Сохранить'}
              </button>
              {!embeddedInTruthPanel && (
                <button
                  type="button"
                  className="btn btn--ghost"
                  onClick={() => void handleConfirmParam()}
                  disabled={verifySaving || displayRows.length === 0}
                  title="Сохранить одну выбранную ✓ строку как правду"
                >
                  Подтвердить (одна ✓)
                </button>
              )}
              <button
                type="button"
                className="btn btn--ghost"
                onClick={() => void handleResetVerification()}
                disabled={verifySaving}
              >
                Сбросить правки
              </button>
              {logComparison && (
                <button type="button" className="btn btn--ghost" onClick={applyLogMatchesToVerified}>
                  Отметить OK из лога
                </button>
              )}
              <button
                type="button"
                className="btn btn--ghost btn--danger"
                onClick={() => void handleClearSelectedRows()}
                disabled={verifySaving || selectedRaws.size === 0}
                title="Удалить выбранные строки из списка"
              >
                Удалить выбранные{selectedRaws.size > 0 ? ` (${selectedRaws.size})` : ''}
              </button>
            </div>

            <div className="value-map-toolbar value-map-toolbar--rail">
              <label className="filter-field">
                <span>Фильтр</span>
                <input
                  type="search"
                  value={filter}
                  onChange={(event) => setFilter(event.target.value)}
                  placeholder="raw / ui / метка"
                />
              </label>
              <span className="value-map-count mono">
                в списке {displayRows.length}
                {tableMode !== 'meta-full' && ` (${tableMode})`}
                {' · '}сверено {verification.verifiedRaws.length}
                {selectedRaws.size > 0 && ` · выбрано ${selectedRaws.size}`}
              </span>
            </div>
          </aside>
        </div>
      )}
    </div>
  );
}

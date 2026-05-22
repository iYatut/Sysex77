import { useEffect, useMemo, useState } from 'react';
import { Link, useParams, useSearchParams } from 'react-router-dom';
import {
  fetchAllParams,
  fetchCurrentDump,
  fetchParamById,
  fetchParamValueMap,
  focusParamInSy99,
  updateParamCatalogFields,
  updateParamVerification,
} from '../api/params';
import { MidiNoteRangeField } from '../components/MidiNoteRangeField';
import { ParamValueMapTable } from '../components/ParamValueMapTable';
import { formatElementLabel } from '../utils/elementLabel';
import {
  canUseMidiNoteRangeEditor,
  midiNoteToName,
  paramUsesMidiNoteRange,
} from '../utils/midiNotes';
import type { ParamValueMap } from '../types/paramValueMap';
import type { ParamCodecKind } from '../types/paramCatalog';
import { ApiError } from '../api/client';
import type { ParamMeta } from '../types/paramMeta';
import type { ParamDumpRow } from '../types/paramDump';
import { formatDumpValue, paramSourceTag } from '../types/paramDump';
import { uniqueSortedGroups } from '../utils/paramGroups';
import {
  ELMODE_BUILTIN_RANGE,
  enumNamesEditorRange,
  MIDI_CC_ENUM_COPY_TARGETS,
  catalogFormToPatch,
  clampRawBound,
  META_RAW_INPUT_MAX,
  META_RAW_INPUT_MIN,
  parseRawBoundInput,
  paramToCatalogForm,
  rawSpanCount,
  isEnumParam,
  type ParamCatalogForm,
} from '../utils/paramForm';
import {
  CODEC_KIND_HINTS,
  describeCodecPair,
  formRawBoundsToStored,
  paramUsesUiCodec,
} from '../utils/paramCodec';
import { loadParamListResume, saveParamListResume } from '../utils/paramListResume';
import {
  clearLegacyParamVerification,
  emptyParamVerificationState,
  hasVerificationData,
  loadLegacyParamVerification,
  verificationFromParam,
  type ParamVerificationState,
} from '../utils/paramVerification';

const CODEC_KIND_OPTIONS: ParamCodecKind[] = ['identity', 'signed', 'enum'];

function parseApiError(err: unknown, fallback: string): string {
  if (err instanceof ApiError) {
    return `${err.message} (${err.status})`;
  }
  if (err instanceof Error) {
    return err.message;
  }
  return fallback;
}

function humanizeFocusSy99Error(err: unknown): string {
  const base = parseApiError(err, 'Не удалось открыть параметр на SY99');
  if (base.includes('no value')) {
    return (
      'Нет raw для отправки — dump пустой. Для части параметров (напр. AFTMD) bulk после Sync ' +
      'ещё не подключён к resolve: выберите raw в списке ниже и нажмите снова.'
    );
  }
  if (base.includes('MIDI output not available')) {
    return 'MIDI out недоступен — запустите Sysex77 и выберите выход MIDI на SY99.';
  }
  if (base.includes('unknown parameter id')) {
    return 'Параметр не найден в реестре Sysex77 — пересоберите exe с актуальным API.';
  }
  return base;
}

function formatRawWithNote(raw: number | null, asNote: boolean): string {
  if (raw === null || raw === undefined) {
    return '—';
  }
  if (!asNote) {
    return String(raw);
  }
  return `${raw} (${midiNoteToName(raw)})`;
}

function dumpRowsForParam(rows: ParamDumpRow[], paramId: string, perElement: boolean): ParamDumpRow[] {
  const indices = perElement ? [0, 1, 2, 3] : [0];
  const byIndex = new Map(rows.filter((row) => row.id === paramId).map((row) => [row.elementIndex, row]));

  return indices.map(
    (elementIndex) =>
      byIndex.get(elementIndex) ?? {
        id: paramId,
        elementIndex,
        raw: null,
        ui: null,
        source: 'None',
      },
  );
}

function resolveVerificationState(param: ParamMeta, elementIndex: number): ParamVerificationState {
  const fromServer = verificationFromParam(param, elementIndex);
  if (hasVerificationData(fromServer)) {
    return fromServer;
  }
  return loadLegacyParamVerification(param.id, elementIndex);
}

function parseElementFromSearch(searchParams: URLSearchParams): number {
  const raw = searchParams.get('element');
  if (raw === null) {
    return 0;
  }
  const n = Number.parseInt(raw, 10);
  if (Number.isNaN(n) || n < 0 || n > 3) {
    return 0;
  }
  return n;
}

type EnumNamesEditorProps = {
  names: string[];
  rawMin: number;
  rawMax: number;
  onChange: (names: string[]) => void;
};

function formatEnumRowLabels(raw: number): { sy99: string } {
  const sy99 = raw + 1;
  const sy99Label = sy99 < 10 ? `0${sy99}` : String(sy99);
  return { sy99: sy99Label };
}

type UiBoundFieldsProps = {
  uiMin: number;
  uiMax: number;
  minLabel: string;
  maxLabel: string;
  onChange: (patch: { uiMin?: number; uiMax?: number }) => void;
};

function UiBoundFields({ uiMin, uiMax, minLabel, maxLabel, onChange }: UiBoundFieldsProps) {
  const [draftMin, setDraftMin] = useState<string | null>(null);
  const [draftMax, setDraftMax] = useState<string | null>(null);

  function commitMin(text: string) {
    onChange({
      uiMin: clampRawBound(
        parseRawBoundInput(text, uiMin, META_RAW_INPUT_MIN, META_RAW_INPUT_MAX),
        META_RAW_INPUT_MIN,
        Math.min(META_RAW_INPUT_MAX, uiMax),
      ),
    });
    setDraftMin(null);
  }

  function commitMax(text: string) {
    onChange({
      uiMax: clampRawBound(
        parseRawBoundInput(text, uiMax, META_RAW_INPUT_MIN, META_RAW_INPUT_MAX),
        Math.max(META_RAW_INPUT_MIN, uiMin),
        META_RAW_INPUT_MAX,
      ),
    });
    setDraftMax(null);
  }

  return (
    <>
      <label className="form-field">
        <span>{minLabel}</span>
        <input
          type="number"
          className="mono"
          value={draftMin ?? String(uiMin)}
          onFocus={() => setDraftMin(String(uiMin))}
          onChange={(event) => setDraftMin(event.target.value)}
          onBlur={(event) => commitMin(event.target.value)}
          onKeyDown={(event) => {
            if (event.key === 'Enter') {
              commitMin((event.target as HTMLInputElement).value);
            }
          }}
        />
      </label>
      <label className="form-field">
        <span>{maxLabel}</span>
        <input
          type="number"
          className="mono"
          value={draftMax ?? String(uiMax)}
          onFocus={() => setDraftMax(String(uiMax))}
          onChange={(event) => setDraftMax(event.target.value)}
          onBlur={(event) => commitMax(event.target.value)}
          onKeyDown={(event) => {
            if (event.key === 'Enter') {
              commitMax((event.target as HTMLInputElement).value);
            }
          }}
        />
      </label>
    </>
  );
}

function EnumNamesEditor({ names, rawMin, rawMax, onChange }: EnumNamesEditorProps) {
  const rawValues: number[] = [];
  for (let raw = rawMin; raw <= rawMax; raw++) {
    rawValues.push(raw);
  }

  function updateName(raw: number, value: string) {
    const next = [...names];
    next[raw] = value;
    onChange(next);
  }

  function clearName(raw: number) {
    const next = [...names];
    next[raw] = '';
    onChange(next);
  }

  return (
    <div className="enum-list">
      {rawValues.map((raw) => {
        const { sy99 } = formatEnumRowLabels(raw);
        const name = names[raw] ?? '';
        return (
          <div key={raw} className="enum-row">
            <span
              className="enum-index mono"
              title={`raw SysEx (VV) = ${raw}; на SY99 LCD пункт ${sy99}`}
            >
              {sy99}
            </span>
            <input
              type="text"
              value={name}
              onChange={(event) => updateName(raw, event.target.value)}
            />
            <button type="button" className="btn btn--ghost" onClick={() => clearName(raw)}>
              Очистить
            </button>
          </div>
        );
      })}
    </div>
  );
}

export function ParamDetailPage() {
  const { id = '' } = useParams();
  const [searchParams] = useSearchParams();
  const elementIndex = parseElementFromSearch(searchParams);

  const [param, setParam] = useState<ParamMeta | null>(null);
  const [form, setForm] = useState<ParamCatalogForm | null>(null);
  const [groups, setGroups] = useState<string[]>([]);
  const [dump, setDump] = useState<ParamDumpRow[]>([]);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [saveMessage, setSaveMessage] = useState<string | null>(null);
  const [valueMap, setValueMap] = useState<ParamValueMap | null>(null);
  const [valueMapLoading, setValueMapLoading] = useState(false);
  const [valueMapError, setValueMapError] = useState<string | null>(null);
  const [rangeAsNotes, setRangeAsNotes] = useState(false);
  const [verification, setVerification] = useState<ParamVerificationState>(() =>
    emptyParamVerificationState(),
  );
  const [verifySaving, setVerifySaving] = useState(false);
  const [showAllRuntimeMapRows, setShowAllRuntimeMapRows] = useState(false);
  const [focusSy99Loading, setFocusSy99Loading] = useState(false);
  const [focusSy99Message, setFocusSy99Message] = useState<string | null>(null);
  const [focusSendRaw, setFocusSendRaw] = useState(0);

  useEffect(() => {
    let cancelled = false;

    async function loadPage() {
      if (!id) {
        setError('Не указан id параметра');
        setLoading(false);
        return;
      }

      setLoading(true);
      setValueMapLoading(true);
      setError(null);
      setSaveMessage(null);
      setValueMapError(null);

      try {
        const [paramData, allParams, dumpData, mapData] = await Promise.all([
          fetchParamById(id),
          fetchAllParams(),
          fetchCurrentDump(),
          fetchParamValueMap(id, elementIndex),
        ]);

        if (cancelled) {
          return;
        }

        setParam(paramData);
        setForm(paramToCatalogForm(paramData));
        setGroups(uniqueSortedGroups(allParams));
        setDump(dumpData);
        setValueMap(mapData);
        setVerification(resolveVerificationState(paramData, elementIndex));
        setShowAllRuntimeMapRows(false);
      } catch (err) {
        if (!cancelled) {
          setError(parseApiError(err, 'Не удалось загрузить параметр'));
          setValueMap(null);
        }
      } finally {
        if (!cancelled) {
          setLoading(false);
          setValueMapLoading(false);
        }
      }
    }

    void loadPage();

    return () => {
      cancelled = true;
    };
  }, [id, elementIndex]);

  useEffect(() => {
    if (param) {
      setRangeAsNotes(paramUsesMidiNoteRange(param));
    }
  }, [param?.id, param?.tag, param?.decode, param?.encode]);

  useEffect(() => {
    if (!id) {
      return;
    }
    const prev = loadParamListResume();
    saveParamListResume({
      paramId: id,
      elementIndex,
      tagQuery: prev?.tagQuery ?? '',
      groupQuery: param?.group ?? prev?.groupQuery ?? '',
    });
  }, [id, elementIndex, param?.group]);

  const listBackState = useMemo(() => {
    const prev = loadParamListResume();
    return {
      resume: {
        paramId: id,
        elementIndex,
        tagQuery: prev?.tagQuery ?? '',
        groupQuery: param?.group ?? prev?.groupQuery ?? '',
      },
    };
  }, [id, elementIndex, param?.group]);

  const showNoteRangeEditor = useMemo(() => {
    if (!form || !param) {
      return false;
    }
    return rangeAsNotes && canUseMidiNoteRangeEditor(form.uiMin, form.uiMax);
  }, [form, param, rangeAsNotes]);

  const dumpRows = useMemo(() => {
    if (!param) {
      return [];
    }
    return dumpRowsForParam(dump, param.id, param.perElement);
  }, [dump, param]);

  const activeRawForValueMap = useMemo(() => {
    const row = dumpRows.find((r) => r.elementIndex === elementIndex);
    return row?.raw ?? null;
  }, [dumpRows, elementIndex]);

  const focusRawFromDump = useMemo(
    () => verification.canonicalRaw ?? activeRawForValueMap,
    [verification.canonicalRaw, activeRawForValueMap],
  );

  const runtimeMapPreviewRows = useMemo(() => {
    if (!valueMap) {
      return [];
    }
    if (showAllRuntimeMapRows) {
      return valueMap.rows;
    }
    return valueMap.rows.slice(0, 32);
  }, [valueMap, showAllRuntimeMapRows]);

  const showEnumEditor = useMemo(() => {
    if (!param || !form) {
      return false;
    }
    return isEnumParam(param) || form.enumNames.length > 0 || form.decode === 'enum';
  }, [param, form]);

  const rawSpan = form ? rawSpanCount(form.uiMin, form.uiMax) : 0;
  const rawRangeValid = rawSpan > 0 && rawSpan <= 140;
  const usesUiCodec = param ? paramUsesUiCodec(param) : false;

  const storedBoundsPreview = useMemo(() => {
    if (!param || !form) {
      return { rawMin: 0, rawMax: 0 };
    }
    return formRawBoundsToStored(param, form.uiMin, form.uiMax);
  }, [param, form]);

  const focusRawOptions = useMemo(() => {
    if (!form || !param) {
      return [] as { raw: number; label: string }[];
    }

    if (showEnumEditor && form.enumNames.length > 0) {
      const { rawMin, rawMax } = enumNamesEditorRange(form.uiMin, form.uiMax);
      const options: { raw: number; label: string }[] = [];
      for (let raw = rawMin; raw <= rawMax; raw++) {
        const name = form.enumNames[raw]?.trim();
        options.push({ raw, label: name ? `raw ${raw} · ${name}` : `raw ${raw}` });
      }
      return options;
    }

    const min = storedBoundsPreview.rawMin;
    const max = storedBoundsPreview.rawMax;
    const span = max - min + 1;
    if (span <= 0 || span > 32) {
      return [{ raw: min, label: `raw ${min}` }];
    }

    return Array.from({ length: span }, (_, index) => {
      const raw = min + index;
      return { raw, label: `raw ${raw}` };
    });
  }, [form, param, showEnumEditor, storedBoundsPreview.rawMin, storedBoundsPreview.rawMax]);

  useEffect(() => {
    if (focusRawFromDump !== null) {
      setFocusSendRaw(focusRawFromDump);
      return;
    }
    if (focusRawOptions.length > 0) {
      setFocusSendRaw(focusRawOptions[0].raw);
    }
  }, [focusRawFromDump, focusRawOptions, id, elementIndex]);

  const codecUiHint = useMemo(() => {
    if (!valueMap || valueMap.uiMin === undefined || valueMap.uiMax === undefined) {
      return null;
    }
    return `На SY99 (колонка ui): ${valueMap.uiMin}…${valueMap.uiMax}`;
  }, [valueMap]);

  function updateForm(patch: Partial<ParamCatalogForm>) {
    setForm((current) => (current ? { ...current, ...patch } : current));
    setSaveMessage(null);
  }

  async function handleCopyEnumToAssignParams() {
    if (!id || !form || !param) {
      return;
    }

    const patch = catalogFormToPatch(form, param);
    const names = patch.enumNames ?? [];
    const filled = names.filter((n) => n.trim().length > 0).length;

    if (filled === 0) {
      setSaveMessage('Сначала заполните enumMap и «Сохранить каталог» на этом параметре.');
      return;
    }

    const targets = MIDI_CC_ENUM_COPY_TARGETS.filter((tag) => tag !== id);
    const ok = window.confirm(
      `Скопировать подписи raw ${form.uiMin}…${form.uiMax} (${filled} непустых) ` +
        `в ${targets.length} параметров?\n\n${targets.join(', ')}`,
    );
    if (!ok) {
      return;
    }

    setSaving(true);
    setSaveMessage(null);

    let copied = 0;
    const failed: string[] = [];

    for (const targetId of targets) {
      try {
        await updateParamCatalogFields(targetId, { enumNames: [...names] });
        copied += 1;
      } catch {
        failed.push(targetId);
      }
    }

    setSaving(false);
    setSaveMessage(
      failed.length === 0
        ? `enumMap скопирован в ${copied} параметров (params_meta.json).`
        : `Скопировано: ${copied}. Ошибки: ${failed.join(', ')} — запущен ли Sysex77?`,
    );
  }

  async function handleSave() {
    if (!id || !form || !param) {
      return;
    }

    setSaving(true);
    setError(null);
    setSaveMessage(null);

    try {
      const patch = catalogFormToPatch(form, param);
      const updated = await updateParamCatalogFields(id, patch);
      setParam(updated);
      setForm(paramToCatalogForm(updated));

      const map = await fetchParamValueMap(id, elementIndex);
      setValueMap(map);
      setShowAllRuntimeMapRows(false);

      const stored = formRawBoundsToStored(updated, form.uiMin, form.uiMax);
      setSaveMessage(
        stored.convertedFromUi
          ? `Каталог: UI ${form.uiMin}…${form.uiMax} → SysEx raw ${updated.rawMin}…${updated.rawMax}.`
          : `Каталог сохранён (raw ${updated.rawMin}…${updated.rawMax}).`,
      );

      const dumpData = await fetchCurrentDump();
      setDump(dumpData);
    } catch (err) {
      setError(parseApiError(err, 'Не удалось сохранить каталог'));
    } finally {
      setSaving(false);
    }
  }

  async function handlePersistVerification(state: ParamVerificationState) {
    if (!id || !param) {
      return;
    }

    setVerifySaving(true);
    setValueMapError(null);

    try {
      const updated = await updateParamVerification(id, elementIndex, state, param);
      setParam(updated);
      setVerification(verificationFromParam(updated, elementIndex));
      clearLegacyParamVerification(id, elementIndex);
    } catch (err) {
      setValueMapError(parseApiError(err, 'Не удалось сохранить карту значений'));
      throw err;
    } finally {
      setVerifySaving(false);
    }
  }

  async function refetchValueMap(): Promise<ParamValueMap | null> {
    if (!id) {
      return null;
    }

    setValueMapLoading(true);
    setValueMapError(null);

    try {
      const map = await fetchParamValueMap(id, elementIndex);
      setValueMap(map);
      setShowAllRuntimeMapRows(false);
      return map;
    } catch (err) {
      setValueMap(null);
      setValueMapError(parseApiError(err, 'Не удалось загрузить карту значений'));
      return null;
    } finally {
      setValueMapLoading(false);
    }
  }

  function elementLink(el: number) {
    if (el === 0) {
      return `/params/${encodeURIComponent(id)}`;
    }
    return `/params/${encodeURIComponent(id)}?element=${el}`;
  }

  async function refreshDumpFromApi(): Promise<ParamDumpRow[]> {
    const dumpData = await fetchCurrentDump();
    setDump(dumpData);
    return dumpData;
  }

  async function handleFocusSy99() {
    if (!id || !param) {
      return;
    }

    setFocusSy99Loading(true);
    setFocusSy99Message(null);

    try {
      await refreshDumpFromApi();
    } catch (err) {
      setFocusSy99Message(parseApiError(err, 'Не удалось обновить dump'));
      setFocusSy99Loading(false);
      return;
    }

    const raw = focusRawFromDump ?? focusSendRaw;
    const rawSource =
      focusRawFromDump !== null
        ? verification.canonicalRaw !== null
          ? 'правда'
          : 'dump'
        : 'выбран вручную';

    try {
      await focusParamInSy99(id, elementIndex, { raw });
      const elSuffix = param.perElement
        ? ` · эл. ${formatElementLabel(elementIndex)}`
        : '';
      setFocusSy99Message(
        `Отправлено на SY99 (${id}${elSuffix}, raw ${raw}, ${rawSource}) — синт должен перейти на экран параметра.`,
      );
    } catch (err) {
      setFocusSy99Message(humanizeFocusSy99Error(err));
    } finally {
      setFocusSy99Loading(false);
    }
  }

  return (
    <section className="page">
      <header className="page-header page-header--detail">
        <div>
          <Link to="/params" state={listBackState} className="back-link">
            ← К списку
          </Link>
          <h1>
            {id || 'Параметр'}
            {param?.perElement && (
              <span className="page-title-suffix mono"> · эл. {formatElementLabel(elementIndex)}</span>
            )}
          </h1>
          <p className="page-subtitle page-subtitle--catalog">
            Редактирование дескriptor-полей каталога (params_meta.json). Значения пресета и runtime
            dump здесь не сохраняются.
          </p>
        </div>
        <button
          type="button"
          className="btn btn--primary"
          disabled={loading || saving || !form}
          onClick={() => void handleSave()}
        >
          {saving ? 'Сохранение…' : 'Сохранить каталог'}
        </button>
      </header>

      {loading && <div className="alert">Загрузка…</div>}
      {error && <div className="alert alert--error">{error}</div>}
      {saveMessage && <div className="alert alert--success">{saveMessage}</div>}

      {param && form && (
        <div className="detail-layout">
          <section className="panel panel--meta">
            <h2>Каталог (ParamCatalogEntry)</h2>

            <div className="form-grid">
              <label className="form-field">
                <span>id</span>
                <input type="text" className="mono" value={param.id} readOnly />
              </label>

              <label className="form-field">
                <span>groupId</span>
                <input
                  type="text"
                  list="param-groups"
                  className="mono"
                  value={form.groupId}
                  onChange={(event) => updateForm({ groupId: event.target.value })}
                />
                <datalist id="param-groups">
                  {groups.map((group) => (
                    <option key={group} value={group} />
                  ))}
                </datalist>
              </label>

              {param.perElement && (
                <label className="form-field">
                  <span>elementIndex</span>
                  <input type="text" className="mono" value={String(elementIndex)} readOnly />
                </label>
              )}

              <label className="form-field">
                <span title="Как байт VV из SysEx → число/пункт в dump и сверке">
                  decode
                </span>
                <select
                  className="mono"
                  value={form.decode}
                  onChange={(event) =>
                    updateForm({ decode: event.target.value as ParamCodecKind })
                  }
                >
                  {CODEC_KIND_OPTIONS.map((kind) => (
                    <option key={kind} value={kind}>
                      {kind}
                    </option>
                  ))}
                </select>
              </label>

              <label className="form-field">
                <span title="Как значение из UI → байт VV при отправке на SY99">
                  encode
                </span>
                <select
                  className="mono"
                  value={form.encode}
                  onChange={(event) =>
                    updateForm({ encode: event.target.value as ParamCodecKind })
                  }
                >
                  {CODEC_KIND_OPTIONS.map((kind) => (
                    <option key={kind} value={kind}>
                      {kind}
                    </option>
                  ))}
                </select>
              </label>

              <details className="codec-cheat-sheet form-field--full">
                <summary className="codec-cheat-sheet__summary">
                  Codec — справка
                  <span className="codec-cheat-sheet__summary-current mono">
                    {form.decode} / {form.encode}
                  </span>
                </summary>
                <div className="codec-cheat-sheet__body">
                  <ul className="codec-cheat-sheet__list">
                    {(Object.keys(CODEC_KIND_HINTS) as ParamCodecKind[]).map((kind) => (
                      <li
                        key={kind}
                        className={
                          form.decode === kind || form.encode === kind
                            ? 'codec-cheat-sheet__item codec-cheat-sheet__item--active'
                            : 'codec-cheat-sheet__item'
                        }
                      >
                        <span className="mono">{kind}</span>
                        <span>{CODEC_KIND_HINTS[kind].lcd}</span>
                        <span className="codec-cheat-sheet__example">
                          {CODEC_KIND_HINTS[kind].example}
                        </span>
                      </li>
                    ))}
                  </ul>
                  <p className="codec-cheat-sheet__active">
                    {describeCodecPair(form.decode, form.encode)}
                  </p>
                  <p className="codec-cheat-sheet__foot">
                    decode — читать (dump, журнал, сверка). encode — писать («Открыть в SY99»,
                    слайдеры). Если LCD ≠ raw в dump — смените codec.
                  </p>
                </div>
              </details>

              <label className="form-field form-field--full">
                <span>sysexTemplate</span>
                <input type="text" className="mono" value={form.sysexTemplate} readOnly />
              </label>

              <label className="form-field">
                <span>rawMin (SysEx, preview)</span>
                <input type="text" className="mono" value={String(storedBoundsPreview.rawMin)} readOnly />
              </label>

              <label className="form-field">
                <span>rawMax (SysEx, preview)</span>
                <input type="text" className="mono" value={String(storedBoundsPreview.rawMax)} readOnly />
              </label>

              {form && canUseMidiNoteRangeEditor(form.uiMin, form.uiMax) && (
                <label className="form-field form-field--checkbox form-field--full">
                  <input
                    type="checkbox"
                    checked={rangeAsNotes}
                    onChange={(event) => setRangeAsNotes(event.target.checked)}
                  />
                  <span>
                    uiMin/uiMax в MIDI-нотах (C-2…G8)
                    {paramUsesMidiNoteRange(param) && ' — рекомендуется для Note Limit'}
                  </span>
                </label>
              )}

              {showNoteRangeEditor ? (
                <>
                  <MidiNoteRangeField
                    label="uiMin (нота)"
                    value={form.uiMin}
                    max={form.uiMax}
                    onChange={(uiMin) => updateForm({ uiMin })}
                  />
                  <MidiNoteRangeField
                    label="uiMax (нота)"
                    value={form.uiMax}
                    min={form.uiMin}
                    onChange={(uiMax) => updateForm({ uiMax })}
                  />
                </>
              ) : (
                <>
                  <UiBoundFields
                    uiMin={form.uiMin}
                    uiMax={form.uiMax}
                    minLabel={usesUiCodec ? 'uiMin (UI на SY99)' : 'uiMin'}
                    maxLabel={usesUiCodec ? 'uiMax (UI на SY99)' : 'uiMax'}
                    onChange={updateForm}
                  />
                  {usesUiCodec && (
                    <p className="panel-hint form-field--full">
                      Для signed codec вводите диапазон как на LCD SY99. В файл сохраняются байты
                      SysEx (rawMin/rawMax). {codecUiHint ?? ''}
                      {param.id === 'ELDT' && param.rawMin === 0 && param.rawMax === 127 && (
                        <>
                          {' '}
                          <button
                            type="button"
                            className="link-button"
                            onClick={() => updateForm({ uiMin: -7, uiMax: 7 })}
                          >
                            Подставить −7…+7
                          </button>
                        </>
                      )}
                    </p>
                  )}
                </>
              )}
            </div>

            {showEnumEditor && (
              <details className="enum-names-details form-section" open>
                <summary className="enum-names-details__summary">
                  enumMap — ui {form.uiMin}…{form.uiMax}
                  {rawRangeValid
                    ? ` (${rawSpan} подписей)`
                    : ' — укажите uiMin ≤ uiMax'}
                </summary>
                <p className="panel-hint">
                  Подписи enum → raw (индекс = raw). После правки — «Сохранить каталог».
                </p>
                {rawRangeValid && rawSpan >= 63 && (
                  <p className="panel-hint">
                    <button
                      type="button"
                      className="btn btn--ghost btn--inline"
                      disabled={saving}
                      onClick={() => void handleCopyEnumToAssignParams()}
                    >
                      Скопировать enumMap на все Assign (PMASN, AMRNG…)
                    </button>
                  </p>
                )}
                {id === 'ELMODE' && (form.uiMin !== 0 || form.uiMax !== 10) && (
                  <p className="panel-hint">
                    У SY99 ELMODE — raw 0…10.{' '}
                    <button
                      type="button"
                      className="btn btn--ghost btn--inline"
                      onClick={() =>
                        updateForm({
                          uiMin: ELMODE_BUILTIN_RANGE.rawMin,
                          uiMax: ELMODE_BUILTIN_RANGE.rawMax,
                          enumNames: [...ELMODE_BUILTIN_RANGE.enumNames],
                          decode: 'enum',
                          encode: 'enum',
                        })
                      }
                    >
                      Подставить 0…10 из реестра
                    </button>
                  </p>
                )}
                <div className="enum-names-details__body">
                  {rawRangeValid ? (
                    <EnumNamesEditor
                      names={form.enumNames}
                      rawMin={enumNamesEditorRange(form.uiMin, form.uiMax).rawMin}
                      rawMax={enumNamesEditorRange(form.uiMin, form.uiMax).rawMax}
                      onChange={(enumNames) => updateForm({ enumNames })}
                    />
                  ) : (
                    <p className="panel-hint">
                      Список подписей скрыт: uiMin не может быть больше uiMax.
                    </p>
                  )}
                </div>
              </details>
            )}

            <details className="form-section meta-readonly">
              <summary>Реестр (только чтение, не в каталоге)</summary>
              <div className="form-grid">
                <label className="form-field">
                  <span>tag</span>
                  <input type="text" className="mono" value={param.tag} readOnly />
                </label>
                <label className="form-field">
                  <span>uiLabel</span>
                  <input type="text" value={param.uiLabel} readOnly />
                </label>
                <label className="form-field form-field--checkbox">
                  <input type="checkbox" checked={param.perElement} readOnly disabled />
                  <span>perElement</span>
                </label>
              </div>
              <p className="mono">
                addr: b3={param.addr.b3} b4={param.addr.b4} b5={param.addr.b5} b6={param.addr.b6}
              </p>
              <p className="mono">
                API decode / encode: {param.decode} / {param.encode}
              </p>
              <pre>{JSON.stringify(param.confidence, null, 2)}</pre>
            </details>

            {focusSy99Message && (
              <div
                className={
                  focusSy99Message.startsWith('Отправлен')
                    ? 'alert alert--success panel-inline-alert'
                    : 'alert alert--error panel-inline-alert'
                }
              >
                <p>{focusSy99Message}</p>
                {!focusSy99Message.startsWith('Отправлен') && focusRawFromDump === null && (
                  <p className="panel-hint focus-sy99-hint">
                    Dump пуст — выберите raw ниже (для AFTMD bulk после Sync ещё не в resolve).
                    Sync в <strong>Sysex77</strong> всё равно нужен для других параметров; смотрите{' '}
                    <Link to="/dump">Dump</Link>.
                  </p>
                )}
                {!focusSy99Message.startsWith('Отправлен') && focusRawFromDump !== null && (
                  <p className="panel-hint focus-sy99-hint">
                    Sync в <strong>Sysex77</strong> («Sync from SY99» → Stop sync), затем повторите.
                    Смотрите <Link to="/dump">Dump</Link>.
                  </p>
                )}
              </div>
            )}

            <div className="focus-sy99-row form-field--full">
              {focusRawFromDump !== null ? (
                <p className="panel-hint focus-sy99-hint">
                  Raw для SY99:{' '}
                  <span className="mono">{focusRawFromDump}</span>
                  {verification.canonicalRaw !== null ? ' · из правды' : ' · из dump'}
                </p>
              ) : (
                <label className="form-field">
                  <span>Raw для отправки (dump пуст)</span>
                  <select
                    className="mono"
                    value={focusSendRaw}
                    onChange={(event) =>
                      setFocusSendRaw(Number.parseInt(event.target.value, 10))
                    }
                  >
                    {focusRawOptions.map((option) => (
                      <option key={option.raw} value={option.raw}>
                        {option.label}
                      </option>
                    ))}
                  </select>
                </label>
              )}
            </div>

            <div className="panel-footer panel-footer--meta">
              <button
                type="button"
                className="btn btn--ghost"
                disabled={loading || focusSy99Loading}
                title="Повторно отправить SysEx с текущим raw — SY99 переключится на экран параметра (как при движении слайдера)"
                onClick={() => void handleFocusSy99()}
              >
                {focusSy99Loading ? 'Отправка…' : 'Открыть в SY99'}
              </button>
            </div>
          </section>

          <div className="detail-workspace">
            <ParamValueMapTable
              param={param}
              paramId={id}
              elementIndex={elementIndex}
              useMidiNotes={showNoteRangeEditor}
              valueMap={valueMap}
              activeRaw={activeRawForValueMap}
              verification={verification}
              onVerificationChange={setVerification}
              onPersistVerification={handlePersistVerification}
              onRefetchValueMap={refetchValueMap}
              verifySaving={verifySaving}
              loading={valueMapLoading}
              error={valueMapError}
            />
          </div>

          <details className="panel catalog-debug-panel">
            <summary>Runtime / отладка (не сохраняется в params_meta.json)</summary>
            <p className="panel-hint">
              Dump после Sync в Sysex77 и справочная карта значений. Эти данные не записываются при
              «Сохранить каталог».
            </p>

            {valueMapLoading && <div className="alert">Загрузка карты значений…</div>}
            {valueMapError && <div className="alert alert--error">{valueMapError}</div>}

            <div className="table-wrap">
              <table className="params-table dump-by-element-table">
                <thead>
                  <tr>
                    <th>Элемент</th>
                    <th>raw (dump)</th>
                    <th>ui</th>
                    <th>source</th>
                    {param.perElement && <th />}
                  </tr>
                </thead>
                <tbody>
                  {dumpRows.map((row) => {
                    const isCurrent = row.elementIndex === elementIndex;
                    return (
                      <tr
                        key={row.elementIndex}
                        className={isCurrent ? 'dump-row--current' : undefined}
                      >
                        <td className="mono">{formatElementLabel(row.elementIndex)}</td>
                        <td className="mono">
                          {formatRawWithNote(row.raw, showNoteRangeEditor)}
                        </td>
                        <td className="mono">{formatDumpValue(row.ui)}</td>
                        <td className="mono">{paramSourceTag(row.source)}</td>
                        {param.perElement && (
                          <td className="dump-row-actions">
                            <Link
                              to={elementLink(row.elementIndex)}
                              className={isCurrent ? 'element-link is-current' : 'element-link'}
                            >
                              {isCurrent ? 'здесь' : 'открыть'}
                            </Link>
                          </td>
                        )}
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>

            {valueMap && valueMap.rows.length > 0 && (
              <div className="table-wrap value-map-table-wrap">
                <p className="panel-hint">
                  Справочник API ({valueMap.rows.length} строк, raw {valueMap.rawMin}…
                  {valueMap.rawMax})
                </p>
                <table className="params-table">
                  <thead>
                    <tr>
                      <th>raw</th>
                      <th>ui</th>
                      <th>programLabel</th>
                    </tr>
                  </thead>
                  <tbody>
                    {runtimeMapPreviewRows.map((row) => (
                      <tr key={row.raw}>
                        <td className="mono">{row.raw}</td>
                        <td className="mono">{row.ui}</td>
                        <td>{row.programLabel}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
                {valueMap.rows.length > 32 && (
                  <p className="panel-hint">
                    {showAllRuntimeMapRows ? (
                      <>
                        Показаны все {valueMap.rows.length} строк.{' '}
                        <button
                          type="button"
                          className="link-button"
                          onClick={() => setShowAllRuntimeMapRows(false)}
                        >
                          Свернуть до 32
                        </button>
                      </>
                    ) : (
                      <>
                        Показаны первые 32 из {valueMap.rows.length} строк.{' '}
                        <button
                          type="button"
                          className="link-button"
                          onClick={() => setShowAllRuntimeMapRows(true)}
                        >
                          Показать все {valueMap.rows.length} строк
                        </button>
                      </>
                    )}
                  </p>
                )}
              </div>
            )}
          </details>
        </div>
      )}
    </section>
  );
}

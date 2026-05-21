import { useEffect, useMemo, useState } from 'react';
import { Link, useParams } from 'react-router-dom';
import {
  fetchAllParams,
  fetchCurrentDump,
  fetchParamById,
  fetchParamValueMap,
  updateParamById,
} from '../api/params';
import { ParamValueMapTable } from '../components/ParamValueMapTable';
import type { ParamValueMap } from '../types/paramValueMap';
import { ApiError } from '../api/client';
import type { ParamMeta } from '../types/paramMeta';
import type { ParamDumpRow } from '../types/paramDump';
import { formatDumpValue, paramSourceTag } from '../types/paramDump';
import { uniqueSortedGroups } from '../utils/paramGroups';
import {
  formToPatch,
  isEnumParam,
  paramToForm,
  type ParamMetaForm,
} from '../utils/paramForm';

function parseApiError(err: unknown, fallback: string): string {
  if (err instanceof ApiError) {
    return `${err.message} (${err.status})`;
  }
  if (err instanceof Error) {
    return err.message;
  }
  return fallback;
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

type EnumNamesEditorProps = {
  names: string[];
  onChange: (names: string[]) => void;
};

function EnumNamesEditor({ names, onChange }: EnumNamesEditorProps) {
  function updateName(index: number, value: string) {
    onChange(names.map((name, i) => (i === index ? value : name)));
  }

  function removeName(index: number) {
    onChange(names.filter((_, i) => i !== index));
  }

  return (
    <div className="enum-list">
      {names.map((name, index) => (
        <div key={index} className="enum-row">
          <span className="enum-index mono">{index}</span>
          <input
            type="text"
            value={name}
            onChange={(event) => updateName(index, event.target.value)}
          />
          <button type="button" className="btn btn--ghost" onClick={() => removeName(index)}>
            Удалить
          </button>
        </div>
      ))}
      <button type="button" className="btn btn--ghost" onClick={() => onChange([...names, ''])}>
        + Добавить значение
      </button>
    </div>
  );
}

export function ParamDetailPage() {
  const { id = '' } = useParams();
  const [param, setParam] = useState<ParamMeta | null>(null);
  const [form, setForm] = useState<ParamMetaForm | null>(null);
  const [groups, setGroups] = useState<string[]>([]);
  const [dump, setDump] = useState<ParamDumpRow[]>([]);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [saveMessage, setSaveMessage] = useState<string | null>(null);
  const [valueMap, setValueMap] = useState<ParamValueMap | null>(null);
  const [valueMapLoading, setValueMapLoading] = useState(false);
  const [valueMapError, setValueMapError] = useState<string | null>(null);
  const [valueMapElementIndex, setValueMapElementIndex] = useState(0);

  useEffect(() => {
    let cancelled = false;

    async function load() {
      if (!id) {
        setError('Не указан id параметра');
        setLoading(false);
        return;
      }

      setLoading(true);
      setError(null);
      setSaveMessage(null);

      try {
        const [paramData, allParams, dumpData] = await Promise.all([
          fetchParamById(id),
          fetchAllParams(),
          fetchCurrentDump(),
        ]);

        if (cancelled) {
          return;
        }

        setParam(paramData);
        setForm(paramToForm(paramData));
        setGroups(uniqueSortedGroups(allParams));
        setDump(dumpData);
      } catch (err) {
        if (!cancelled) {
          setError(parseApiError(err, 'Не удалось загрузить параметр'));
        }
      } finally {
        if (!cancelled) {
          setLoading(false);
        }
      }
    }

    void load();

    return () => {
      cancelled = true;
    };
  }, [id]);

  useEffect(() => {
    let cancelled = false;

    async function loadValueMap() {
      if (!id) {
        return;
      }

      setValueMapLoading(true);
      setValueMapError(null);

      try {
        const map = await fetchParamValueMap(id, valueMapElementIndex);
        if (!cancelled) {
          setValueMap(map);
        }
      } catch (err) {
        if (!cancelled) {
          setValueMap(null);
          setValueMapError(parseApiError(err, 'Не удалось загрузить карту значений'));
        }
      } finally {
        if (!cancelled) {
          setValueMapLoading(false);
        }
      }
    }

    void loadValueMap();

    return () => {
      cancelled = true;
    };
  }, [id, valueMapElementIndex]);

  const dumpRows = useMemo(() => {
    if (!param || !form) {
      return [];
    }
    return dumpRowsForParam(dump, param.id, form.perElement);
  }, [dump, form, param]);

  const activeRawForValueMap = useMemo(() => {
    const row = dumpRows.find((r) => r.elementIndex === valueMapElementIndex);
    return row?.raw ?? null;
  }, [dumpRows, valueMapElementIndex]);

  const showEnumEditor = useMemo(() => {
    if (!param || !form) {
      return false;
    }
    return isEnumParam(param) || form.enumNames.length > 0;
  }, [param, form]);

  function updateForm(patch: Partial<ParamMetaForm>) {
    setForm((current) => (current ? { ...current, ...patch } : current));
    setSaveMessage(null);
  }

  async function handleSave() {
    if (!id || !form) {
      return;
    }

    setSaving(true);
    setError(null);
    setSaveMessage(null);

    try {
      const updated = await updateParamById(id, formToPatch(form));
      setParam(updated);
      setForm(paramToForm(updated));

      const dumpData = await fetchCurrentDump();
      setDump(dumpData);
      setSaveMessage('Сохранено');
    } catch (err) {
      setError(parseApiError(err, 'Не удалось сохранить'));
    } finally {
      setSaving(false);
    }
  }

  return (
    <section className="page">
      <header className="page-header page-header--detail">
        <div>
          <Link to="/params" className="back-link">
            ← К списку
          </Link>
          <h1>{id || 'Параметр'}</h1>
        </div>
        <button
          type="button"
          className="btn btn--primary"
          disabled={loading || saving || !form}
          onClick={() => void handleSave()}
        >
          {saving ? 'Сохранение…' : 'Сохранить'}
        </button>
      </header>

      {loading && <div className="alert">Загрузка…</div>}
      {error && <div className="alert alert--error">{error}</div>}
      {saveMessage && <div className="alert alert--success">{saveMessage}</div>}

      {param && form && (
        <div className="detail-layout">
          <section className="panel">
            <h2>Meta</h2>

            <div className="form-grid">
              <label className="form-field">
                <span>Id</span>
                <input type="text" className="mono" value={param.id} readOnly />
              </label>

              <label className="form-field">
                <span>tag</span>
                <input type="text" className="mono" value={param.tag} readOnly />
              </label>

              <label className="form-field">
                <span>uiLabel</span>
                <input
                  type="text"
                  value={form.uiLabel}
                  onChange={(event) => updateForm({ uiLabel: event.target.value })}
                />
              </label>

              <label className="form-field">
                <span>group</span>
                <input
                  type="text"
                  list="param-groups"
                  value={form.group}
                  onChange={(event) => updateForm({ group: event.target.value })}
                />
                <datalist id="param-groups">
                  {groups.map((group) => (
                    <option key={group} value={group} />
                  ))}
                </datalist>
              </label>

              <label className="form-field form-field--checkbox">
                <input
                  type="checkbox"
                  checked={form.perElement}
                  onChange={(event) => updateForm({ perElement: event.target.checked })}
                />
                <span>perElement</span>
              </label>

              <label className="form-field">
                <span>rawMin</span>
                <input
                  type="number"
                  className="mono"
                  value={form.rawMin}
                  onChange={(event) => updateForm({ rawMin: Number(event.target.value) })}
                />
              </label>

              <label className="form-field">
                <span>rawMax</span>
                <input
                  type="number"
                  className="mono"
                  value={form.rawMax}
                  onChange={(event) => updateForm({ rawMax: Number(event.target.value) })}
                />
              </label>
            </div>

            {showEnumEditor && (
              <div className="form-section">
                <h3>enumNames</h3>
                <EnumNamesEditor
                  names={form.enumNames}
                  onChange={(enumNames) => updateForm({ enumNames })}
                />
              </div>
            )}

            <div className="form-section meta-readonly">
              <h3>Адрес и кодеки</h3>
              <p className="mono">
                addr: b3={param.addr.b3} b4={param.addr.b4} b5={param.addr.b5} b6={param.addr.b6}
              </p>
              <p className="mono">
                decode / encode: {param.decode} / {param.encode}
              </p>
              <pre>{JSON.stringify(param.confidence, null, 2)}</pre>
            </div>
          </section>

          <section className="panel">
            <h2>Текущие значения</h2>
            <p className="panel-hint">Из GET /api/dump/current</p>

            <div className="table-wrap">
              <table className="params-table">
                <thead>
                  <tr>
                    <th>elementIndex</th>
                    <th>raw</th>
                    <th>ui</th>
                    <th>source</th>
                  </tr>
                </thead>
                <tbody>
                  {dumpRows.map((row) => (
                    <tr key={row.elementIndex}>
                      <td className="mono">{row.elementIndex}</td>
                      <td className="mono">{formatDumpValue(row.raw)}</td>
                      <td className="mono">{formatDumpValue(row.ui)}</td>
                      <td className="mono">{paramSourceTag(row.source)}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </section>

          {form.perElement && (
            <section className="panel panel--inline">
              <label className="form-field">
                <span>elementIndex для карты значений</span>
                <select
                  value={valueMapElementIndex}
                  onChange={(event) => setValueMapElementIndex(Number(event.target.value))}
                >
                  {[0, 1, 2, 3].map((idx) => (
                    <option key={idx} value={idx}>
                      {idx}
                    </option>
                  ))}
                </select>
              </label>
            </section>
          )}

          <ParamValueMapTable
            valueMap={valueMap}
            activeRaw={activeRawForValueMap}
            loading={valueMapLoading}
            error={valueMapError}
          />
        </div>
      )}
    </section>
  );
}

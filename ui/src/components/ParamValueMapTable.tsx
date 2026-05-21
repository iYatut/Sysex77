import { useMemo, useState } from 'react';
import type { LogRowOverlay, ParamValueMap, ParamValueMapRow, PresentationKind } from '../types/paramValueMap';
import {
  compareLogWithValueMap,
  logMatchStatusLabel,
  parseSysexLogText,
} from '../utils/parseSysexLog';

type ParamValueMapTableProps = {
  valueMap: ParamValueMap | null;
  activeRaw: number | null;
  loading?: boolean;
  error?: string | null;
};

function kindBadge(kind: PresentationKind): string {
  switch (kind) {
    case 'namedEnum':
      return 'namedEnum';
    case 'decoded':
      return 'decoded';
    default:
      return 'numeric';
  }
}

export function ParamValueMapTable({
  valueMap,
  activeRaw,
  loading = false,
  error = null,
}: ParamValueMapTableProps) {
  const [filter, setFilter] = useState('');
  const [logText, setLogText] = useState('');

  const logComparison = useMemo(() => {
    if (!valueMap || !logText.trim()) {
      return null;
    }
    const parsed = parseSysexLogText(logText);
    return compareLogWithValueMap(parsed, valueMap);
  }, [logText, valueMap]);

  const filteredRows = useMemo(() => {
    if (!valueMap) {
      return [];
    }

    const q = filter.trim();
    if (!q) {
      return valueMap.rows;
    }

    const asNumber = Number.parseInt(q, 10);
    const isNumeric = !Number.isNaN(asNumber);

    return valueMap.rows.filter((row) => {
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
  }, [filter, valueMap]);

  const activeRow = useMemo(() => {
    if (!valueMap || activeRaw === null) {
      return null;
    }
    return valueMap.rows.find((row) => row.raw === activeRaw) ?? null;
  }, [activeRaw, valueMap]);

  function overlayForRow(row: ParamValueMapRow): LogRowOverlay | undefined {
    return logComparison?.overlaysByRaw.get(row.raw);
  }

  function rowClassName(row: ParamValueMapRow, overlay?: LogRowOverlay): string {
    const classes = ['value-map-row'];
    if (activeRaw !== null && row.raw === activeRaw) {
      classes.push('row--active');
    }
    if (row.labelMismatch) {
      classes.push('row--mismatch');
    }
    if (overlay?.status && overlay.status !== 'match') {
      classes.push(`row--log-${overlay.status}`);
    }
    return classes.join(' ');
  }

  return (
    <section className="panel value-map-panel">
      <h2>Карта значений (окна Sysex77)</h2>
      <p className="panel-hint">
        Полный список raw → SysEx и подпись, как в combo/slider программы. GET /api/params/
        {'{id}'}/value-map
      </p>

      {loading && <div className="alert">Загрузка карты…</div>}
      {error && <div className="alert alert--error">{error}</div>}

      {activeRow && (
        <div className="value-map-current">
          <h3>Текущее значение</h3>
          <p className="mono value-map-current__sysex">{activeRow.sysexHex}</p>
          <p>
            raw <span className="mono">{activeRow.raw}</span> → окно:{' '}
            <strong className="mono">{activeRow.programLabel}</strong>{' '}
            <span className={`kind-badge kind-badge--${activeRow.presentationKind}`}>
              {kindBadge(activeRow.presentationKind)}
            </span>
          </p>
        </div>
      )}

      {valueMap && (
        <>
          <div className="value-map-toolbar">
            <label className="filter-field">
              <span>Фильтр (raw / ui / метка)</span>
              <input
                type="search"
                value={filter}
                onChange={(event) => setFilter(event.target.value)}
                placeholder="0–127 или all"
              />
            </label>
            <span className="value-map-count mono">
              {filteredRows.length} / {valueMap.rows.length} строк
            </span>
          </div>

          <label className="form-field value-map-log">
            <span>Вставить наблюдённый лог</span>
            <textarea
              rows={5}
              placeholder={'[#42] SysEx: F0 43 10 34 02 00 00 39 00 27 F7   39'}
              value={logText}
              onChange={(event) => setLogText(event.target.value)}
            />
          </label>

          {logComparison && logComparison.extras.length > 0 && (
            <div className="alert alert--error">
              Лишние строки в логе (raw вне диапазона):{' '}
              {logComparison.extras.map((e) => e.raw).join(', ')}
            </div>
          )}

          <div className="table-wrap value-map-table-wrap">
            <table className="params-table value-map-table">
              <thead>
                <tr>
                  <th>#</th>
                  <th>SysEx</th>
                  <th>raw</th>
                  <th>ui</th>
                  <th>Registry</th>
                  <th>Program (окно)</th>
                  <th>kind</th>
                  {logText.trim() ? <th>Лог</th> : null}
                </tr>
              </thead>
              <tbody>
                {filteredRows.map((row) => {
                  const overlay = overlayForRow(row);
                  return (
                    <tr key={row.raw} className={rowClassName(row, overlay)}>
                      <td className="mono">{row.index}</td>
                      <td className="mono sysex-cell">{row.sysexHex}</td>
                      <td className="mono">{row.raw}</td>
                      <td className="mono">{row.ui}</td>
                      <td className="mono">{row.registryLabel ?? '—'}</td>
                      <td className="mono">{row.programLabel}</td>
                      <td>
                        <span className={`kind-badge kind-badge--${row.presentationKind}`}>
                          {kindBadge(row.presentationKind)}
                        </span>
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
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </>
      )}
    </section>
  );
}

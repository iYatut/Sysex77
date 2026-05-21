import { useMemo, useState } from 'react';
import type { ParamMeta } from '../types/paramMeta';

type ParamPickerProps = {
  params: ParamMeta[];
  paramId: string;
  elementIndex: number;
  onChange: (paramId: string, elementIndex: number) => void;
};

function matchesParamSearch(param: ParamMeta, query: string): boolean {
  const needle = query.trim().toLowerCase();
  if (!needle) {
    return true;
  }

  return (
    param.id.toLowerCase().includes(needle) ||
    param.tag.toLowerCase().includes(needle) ||
    param.uiLabel.toLowerCase().includes(needle)
  );
}

export function ParamPicker({ params, paramId, elementIndex, onChange }: ParamPickerProps) {
  const [query, setQuery] = useState('');

  const selected = useMemo(
    () => params.find((param) => param.id === paramId) ?? null,
    [params, paramId],
  );

  const filtered = useMemo(
    () => params.filter((param) => matchesParamSearch(param, query)).slice(0, 20),
    [params, query],
  );

  return (
    <div className="param-picker">
      <input
        type="search"
        placeholder="Поиск paramId / tag / uiLabel"
        value={query}
        onChange={(event) => setQuery(event.target.value)}
      />

      {selected && (
        <p className="param-picker__selected mono">
          {selected.id} — {selected.uiLabel}
          {selected.perElement ? ' (per element)' : ''}
        </p>
      )}

      {query.trim() && (
        <ul className="param-picker__results">
          {filtered.map((param) => (
            <li key={param.id}>
              <button
                type="button"
                className="param-picker__option"
                onClick={() => {
                  onChange(param.id, param.perElement ? elementIndex : 0);
                  setQuery('');
                }}
              >
                <span className="mono">{param.id}</span>
                <span>{param.uiLabel}</span>
                <span className="param-picker__group">{param.group}</span>
              </button>
            </li>
          ))}
          {filtered.length === 0 && <li className="param-picker__empty">Ничего не найдено</li>}
        </ul>
      )}

      <label className="form-field">
        <span>paramId</span>
        <input
          type="text"
          className="mono"
          value={paramId}
          onChange={(event) => onChange(event.target.value, elementIndex)}
        />
      </label>

      <label className="form-field">
        <span>elementIndex</span>
        <select
          value={elementIndex}
          disabled={selected !== null && !selected.perElement}
          onChange={(event) => onChange(paramId, Number(event.target.value))}
        >
          {[0, 1, 2, 3].map((index) => (
            <option key={index} value={index}>
              {index}
            </option>
          ))}
        </select>
      </label>
    </div>
  );
}

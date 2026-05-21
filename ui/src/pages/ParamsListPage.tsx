import { useEffect, useMemo, useState } from 'react';
import { fetchAllParams } from '../api/params';
import { ApiError } from '../api/client';
import { ParamsFilters } from '../components/ParamsFilters';
import { ParamsTable } from '../components/ParamsTable';
import type { ParamMeta } from '../types/paramMeta';
import { uniqueSortedGroups } from '../utils/paramGroups';

function matchesFilters(param: ParamMeta, tagQuery: string, groupQuery: string): boolean {
  const tagNeedle = tagQuery.trim().toLowerCase();
  const groupNeedle = groupQuery.trim();

  if (groupNeedle && param.group !== groupNeedle) {
    return false;
  }

  if (!tagNeedle) {
    return true;
  }

  return (
    param.tag.toLowerCase().includes(tagNeedle) ||
    param.uiLabel.toLowerCase().includes(tagNeedle) ||
    param.id.toLowerCase().includes(tagNeedle)
  );
}

export function ParamsListPage() {
  const [params, setParams] = useState<ParamMeta[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [tagQuery, setTagQuery] = useState('');
  const [groupQuery, setGroupQuery] = useState('');

  useEffect(() => {
    let cancelled = false;

    async function load() {
      setLoading(true);
      setError(null);

      try {
        const data = await fetchAllParams();
        if (!cancelled) {
          setParams(data);
        }
      } catch (err) {
        if (!cancelled) {
          const message =
            err instanceof ApiError
              ? `${err.message} (${err.status})`
              : err instanceof Error
                ? err.message
                : 'Не удалось загрузить параметры';
          setError(message);
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
  }, []);

  const groups = useMemo(() => uniqueSortedGroups(params), [params]);

  const filteredParams = useMemo(
    () => params.filter((param) => matchesFilters(param, tagQuery, groupQuery)),
    [params, tagQuery, groupQuery],
  );

  return (
    <section className="page">
      <header className="page-header">
        <div>
          <h1>Параметры SY99</h1>
          <p className="page-subtitle">
            {loading
              ? 'Загрузка…'
              : `${filteredParams.length} из ${params.length} параметров`}
          </p>
        </div>
      </header>

      <ParamsFilters
        tagQuery={tagQuery}
        groupQuery={groupQuery}
        groups={groups}
        onTagQueryChange={setTagQuery}
        onGroupQueryChange={setGroupQuery}
      />

      {error && <div className="alert alert--error">{error}</div>}
      {loading && <div className="alert">Загрузка списка параметров…</div>}
      {!loading && !error && <ParamsTable params={filteredParams} />}
    </section>
  );
}

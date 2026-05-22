import { useEffect, useMemo, useState } from 'react';
import { Link, useLocation } from 'react-router-dom';
import { fetchAllParams } from '../api/params';
import { ApiError } from '../api/client';
import { ParamsFilters } from '../components/ParamsFilters';
import { ParamsTable } from '../components/ParamsTable';
import type { ParamMeta } from '../types/paramMeta';
import { uniqueSortedGroups } from '../utils/paramGroups';
import {
  formatElementLabel,
} from '../utils/elementLabel';
import {
  loadParamListResume,
  resumeDetailPath,
  type ParamListResume,
} from '../utils/paramListResume';
import { paramHealthSummary, resolveParamHealthStatus } from '../utils/paramWorkQueue';

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

type ListLocationState = {
  resume?: ParamListResume;
};

export function ParamsListPage() {
  const location = useLocation();
  const [params, setParams] = useState<ParamMeta[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [tagQuery, setTagQuery] = useState('');
  const [groupQuery, setGroupQuery] = useState('');
  const [resume, setResume] = useState<ParamListResume | null>(null);

  useEffect(() => {
    const fromNav = (location.state as ListLocationState | null)?.resume;
    const stored = loadParamListResume();
    const picked = fromNav ?? stored;
    setResume(picked);
    if (picked) {
      setTagQuery(picked.tagQuery ?? '');
      setGroupQuery(picked.groupQuery ?? '');
    }
  }, [location.key, location.state]);

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

  const filteredParams = useMemo(() => {
    const filtered = params.filter((param) => matchesFilters(param, tagQuery, groupQuery));
    const rank = (param: ParamMeta) => {
      const health = resolveParamHealthStatus(param);
      if (health === 'error') {
        return 0;
      }
      if (health === 'unknown') {
        return 1;
      }
      return 2;
    };

    return [...filtered].sort((a, b) => {
      const diff = rank(a) - rank(b);
      if (diff !== 0) {
        return diff;
      }
      return a.id.localeCompare(b.id);
    });
  }, [params, tagQuery, groupQuery]);

  const healthSummary = useMemo(() => paramHealthSummary(params), [params]);

  return (
    <section className="page">
      <header className="page-header">
        <div>
          <h1>Каталог параметров SY99</h1>
          <p className="page-subtitle page-subtitle--catalog">
            Эта страница редактирует глобальный каталог параметров (адреса, диапазоны, enum,
            шаблоны SysEx). Она <strong>не</strong> хранит значения per-preset.
          </p>
          <p className="page-subtitle">
            {loading
              ? 'Загрузка…'
              : `${filteredParams.length} из ${params.length} параметров`}
          </p>
          {!loading && !error && params.length > 0 && (
            <p className="param-health-summary">
              <span className="param-status-dot param-status-dot--ok" aria-hidden />
              <strong>{healthSummary.ok}</strong> без ошибок
              <span className="param-health-sep">·</span>
              <span className="param-status-dot param-status-dot--error" aria-hidden />
              <strong>{healthSummary.error}</strong> с ошибками
              <span className="param-health-sep">·</span>
              <span className="param-status-dot param-status-dot--unknown" aria-hidden />
              <strong>{healthSummary.unknown}</strong> не проверено
              <span className="param-health-sep">·</span>
              <strong>{healthSummary.total}</strong> всего
            </p>
          )}
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
      {resume && !loading && !error && (
        <p className="params-resume-banner">
          Вернулись после{' '}
          <Link to={resumeDetailPath(resume)} state={{ resume }} className="mono">
            {resume.paramId}
          </Link>
          {resume.elementIndex > 0 && (
            <span className="mono"> · эл. {formatElementLabel(resume.elementIndex)}</span>
          )}
          . Строка подсвечена в таблице ниже.
        </p>
      )}

      {!loading && !error && (
        <ParamsTable
          params={filteredParams}
          resume={resume}
          listFilters={{ tagQuery, groupQuery }}
        />
      )}
    </section>
  );
}

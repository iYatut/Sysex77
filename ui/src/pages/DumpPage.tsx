import { useEffect, useMemo, useState } from 'react';
import { Link } from 'react-router-dom';
import { fetchAllParams, fetchCurrentDump } from '../api/params';
import { ApiError } from '../api/client';
import { DumpFilters } from '../components/DumpFilters';
import { DumpTable } from '../components/DumpTable';
import {
  joinDumpWithMeta,
  matchesDumpGroup,
  matchesDumpSource,
  uniqueDumpGroups,
} from '../utils/dumpRows';

function parseApiError(err: unknown, fallback: string): string {
  if (err instanceof ApiError) {
    return `${err.message} (${err.status})`;
  }
  if (err instanceof Error) {
    return err.message;
  }
  return fallback;
}

export function DumpPage() {
  const [rows, setRows] = useState<ReturnType<typeof joinDumpWithMeta>>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [groupQuery, setGroupQuery] = useState('');
  const [sourceQuery, setSourceQuery] = useState('');

  useEffect(() => {
    let cancelled = false;

    async function load() {
      setLoading(true);
      setError(null);

      try {
        const [dump, params] = await Promise.all([fetchCurrentDump(), fetchAllParams()]);

        if (!cancelled) {
          setRows(joinDumpWithMeta(dump, params));
        }
      } catch (err) {
        if (!cancelled) {
          setError(parseApiError(err, 'Не удалось загрузить dump'));
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

  const groups = useMemo(() => uniqueDumpGroups(rows), [rows]);

  const filteredRows = useMemo(
    () => rows.filter((row) => matchesDumpGroup(row, groupQuery) && matchesDumpSource(row, sourceQuery)),
    [rows, groupQuery, sourceQuery],
  );

  return (
    <section className="page">
      <header className="page-header">
        <div>
          <Link to="/params" className="back-link">
            ← К параметрам
          </Link>
          <h1>LiveSynthState Dump</h1>
          <p className="page-subtitle">
            {loading ? 'Загрузка…' : `${filteredRows.length} из ${rows.length} строк`}
          </p>
        </div>
      </header>

      <DumpFilters
        groupQuery={groupQuery}
        sourceQuery={sourceQuery}
        groups={groups}
        onGroupQueryChange={setGroupQuery}
        onSourceQueryChange={setSourceQuery}
      />

      {error && <div className="alert alert--error">{error}</div>}
      {loading && <div className="alert">Загрузка dump/current…</div>}
      {!loading && !error && <DumpTable rows={filteredRows} />}
    </section>
  );
}

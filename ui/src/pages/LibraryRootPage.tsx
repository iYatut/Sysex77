import { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { fetchLibraryTree } from '../api/library';
import { ApiError } from '../api/client';
import { LibraryBreadcrumbs } from '../components/LibraryBreadcrumbs';
import type { LibraryTree } from '../types/library';

function parseApiError(err: unknown, fallback: string): string {
  if (err instanceof ApiError) {
    return `${err.message} (${err.status})`;
  }
  if (err instanceof Error) {
    return err.message;
  }
  return fallback;
}

export function LibraryRootPage() {
  const [tree, setTree] = useState<LibraryTree | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let cancelled = false;

    async function load() {
      setLoading(true);
      setError(null);

      try {
        const data = await fetchLibraryTree();

        if (!cancelled) {
          setTree(data);
        }
      } catch (err) {
        if (!cancelled) {
          setError(parseApiError(err, 'Не удалось загрузить библиотеку'));
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

  return (
    <section className="page">
      <LibraryBreadcrumbs items={[{ label: 'Library' }]} />

      <header className="page-header">
        <h1>Library</h1>
        <p className="page-subtitle">
          Файлы синхронизации из captures/ — выберите SY-99, затем папку памяти.
        </p>
      </header>

      {error && <div className="alert alert--error">{error}</div>}
      {loading && <div className="alert">Загрузка library/tree…</div>}

      {!loading && tree && (
        <div className="library-tree">
          <Link to={`/library/${tree.id}`} className="library-folder-card library-folder-card--root">
            <span className="library-folder-card__title">{tree.label}</span>
            <span className="library-folder-card__meta">
              {tree.active ? 'captures загружены' : 'нет AUTOSYNC captures — запустите sync в Sysex77'}
            </span>
          </Link>
        </div>
      )}
    </section>
  );
}

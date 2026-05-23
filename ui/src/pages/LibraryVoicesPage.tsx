import { useEffect, useMemo, useState } from 'react';
import { Link, useParams } from 'react-router-dom';
import { fetchLibraryTree, fetchLibraryVoices } from '../api/library';
import { ApiError } from '../api/client';
import { LibraryBreadcrumbs } from '../components/LibraryBreadcrumbs';
import { formatElmodeCardLabel } from '../utils/elmodeLabel';
import type { LibraryPageInfo, LibraryVoiceSummary } from '../types/library';

function parseApiError(err: unknown, fallback: string): string {
  if (err instanceof ApiError) {
    return `${err.message} (${err.status})`;
  }
  if (err instanceof Error) {
    return err.message;
  }
  return fallback;
}

export function LibrarySy99Page() {
  const { libraryId = 'sy99' } = useParams();
  const [pages, setPages] = useState<LibraryPageInfo[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let cancelled = false;

    async function load() {
      setLoading(true);
      setError(null);

      try {
        const tree = await fetchLibraryTree();

        if (!cancelled) {
          setPages(tree.pages);
        }
      } catch (err) {
        if (!cancelled) {
          setError(parseApiError(err, 'Не удалось загрузить SY-99'));
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
      <LibraryBreadcrumbs
        items={[
          { label: 'Library', to: '/library' },
          { label: libraryId.toUpperCase() },
        ]}
      />

      <header className="page-header">
        <h1>SY-99</h1>
        <p className="page-subtitle">4 папки памяти — Internal, PRE1, PRE2, Multi.</p>
      </header>

      {error && <div className="alert alert--error">{error}</div>}
      {loading && <div className="alert">Загрузка…</div>}

      {!loading && (
        <div className="library-folder-grid">
          {pages.map((page) => (
            <Link
              key={page.id}
              to={`/library/${libraryId}/${page.id}`}
              className={`library-folder-card${page.available ? '' : ' library-folder-card--disabled'}`}
            >
              <span className="library-folder-card__title">{page.label}</span>
              <span className="library-folder-card__meta">
                {page.available
                  ? `${page.slotCount} слотов · ${page.captureFile ?? ''}`
                  : 'файл capture отсутствует'}
              </span>
            </Link>
          ))}
        </div>
      )}
    </section>
  );
}

export function LibraryVoicesPage() {
  const { libraryId = 'sy99', pageId = 'internal' } = useParams();
  const [voices, setVoices] = useState<LibraryVoiceSummary[]>([]);
  const [pageLabel, setPageLabel] = useState(pageId);
  const [captureFile, setCaptureFile] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [query, setQuery] = useState('');

  useEffect(() => {
    let cancelled = false;

    async function load() {
      setLoading(true);
      setError(null);

      try {
        const data = await fetchLibraryVoices(pageId);

        if (!cancelled) {
          setVoices(data.voices);
          setPageLabel(data.pageLabel);
          setCaptureFile(data.captureFile);
        }
      } catch (err) {
        if (!cancelled) {
          setError(parseApiError(err, 'Не удалось загрузить голоса'));
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
  }, [pageId]);

  const filteredVoices = useMemo(() => {
    const needle = query.trim().toLowerCase();

    if (!needle) {
      return voices;
    }

    return voices.filter(
      (voice) =>
        voice.name.toLowerCase().includes(needle) ||
        voice.slotCode.toLowerCase().includes(needle) ||
        formatElmodeCardLabel(voice.elmodeRaw).toLowerCase().includes(needle),
    );
  }, [voices, query]);

  return (
    <section className="page">
      <LibraryBreadcrumbs
        items={[
          { label: 'Library', to: '/library' },
          { label: 'SY-99', to: `/library/${libraryId}` },
          { label: pageLabel },
        ]}
      />

      <header className="page-header">
        <h1>{pageLabel}</h1>
        <p className="page-subtitle">
          {loading ? 'Загрузка…' : `${filteredVoices.length} голосов`}
          {captureFile ? ` · ${captureFile}` : ''}
        </p>
      </header>

      <div className="params-filters">
        <label className="filter-field">
          <span>Поиск</span>
          <input
            type="search"
            value={query}
            placeholder="A1 или имя голоса"
            onChange={(event) => setQuery(event.target.value)}
          />
        </label>
      </div>

      {error && <div className="alert alert--error">{error}</div>}
      {loading && <div className="alert">Загрузка voices…</div>}

      {!loading && !error && (
        <div className="library-voice-grid">
          {filteredVoices.map((voice) => {
            const elmodeLabel = formatElmodeCardLabel(voice.elmodeRaw);

            return (
            <Link
              key={voice.mm}
              to={`/library/${libraryId}/${pageId}/${voice.mm}`}
              className="library-voice-card"
            >
              <div className="library-voice-card__header">
                <span className="library-voice-card__slot">{voice.slotCode}</span>
                {elmodeLabel ? (
                  <span className="library-voice-card__mode">{elmodeLabel}</span>
                ) : null}
              </div>
              <span className="library-voice-card__name">{voice.name}</span>
            </Link>
            );
          })}
        </div>
      )}
    </section>
  );
}

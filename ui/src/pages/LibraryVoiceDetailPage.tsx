import { useEffect, useState } from 'react';

import { useParams, useSearchParams } from 'react-router-dom';

import { fetchLibraryVoiceDetail } from '../api/library';

import { ApiError } from '../api/client';

import { LibraryBreadcrumbs } from '../components/LibraryBreadcrumbs';

import { LibraryVoiceParamsTable } from '../components/LibraryVoiceParamsTable';

import type { LibraryVoiceDetail } from '../types/library';



function parseApiError(err: unknown, fallback: string): string {

  if (err instanceof ApiError) {

    return `${err.message} (${err.status})`;

  }

  if (err instanceof Error) {

    return err.message;

  }

  return fallback;

}



export function LibraryVoiceDetailPage() {

  const { libraryId = 'sy99', pageId = 'internal', mm = '0' } = useParams();

  const [searchParams, setSearchParams] = useSearchParams();

  const reviewIdFromUrl = searchParams.get('review');

  const slotMm = Number.parseInt(mm, 10);

  const [detail, setDetail] = useState<LibraryVoiceDetail | null>(null);

  const [loading, setLoading] = useState(true);

  const [error, setError] = useState<string | null>(null);



  useEffect(() => {

    let cancelled = false;



    async function load() {

      setLoading(true);

      setError(null);



      try {

        const data = await fetchLibraryVoiceDetail(pageId, slotMm);



        if (!cancelled) {

          setDetail(data);

        }

      } catch (err) {

        if (!cancelled) {

          setError(parseApiError(err, 'Не удалось загрузить голос'));

        }

      } finally {

        if (!cancelled) {

          setLoading(false);

        }

      }

    }



    if (Number.isNaN(slotMm)) {

      setError('Неверный номер слота');

      setLoading(false);

      return;

    }



    void load();



    return () => {

      cancelled = true;

    };

  }, [pageId, slotMm]);



  const slotCode = detail?.slotCode ?? `#${mm}`;

  const voiceName = detail?.voiceName ?? slotCode;



  function handleReviewIdChange(reviewId: string | null) {

    const next = new URLSearchParams(searchParams);

    if (reviewId) {

      next.set('review', reviewId);

    } else {

      next.delete('review');

    }

    setSearchParams(next, { replace: true });

  }



  return (

    <section className="page page--library-voice">

      <LibraryBreadcrumbs

        items={[

          { label: 'Library', to: '/library' },

          { label: 'SY-99', to: `/library/${libraryId}` },

          { label: detail?.pageLabel ?? pageId, to: `/library/${libraryId}/${pageId}` },

          { label: `${slotCode} ${voiceName}` },

        ]}

      />



      <header className="page-header">

        <h1>

          {slotCode} — {voiceName}

        </h1>

        <p className="page-subtitle">

          {loading

            ? 'Загрузка…'

            : detail

              ? `${detail.params.filter((p) => p.inDump).length} в дампе · ${detail.params.length} всего в каталоге · ${detail.captureFile}`

              : 'Параметры голоса'}

        </p>

        {!loading && detail && (

          <p className="page-subtitle">

            Таблица на всю ширину страницы. Порядок строк — как LCD Common SY-99 (#200…).
            Клик по ячейке — значение с синта; ↑↓ / ⋮⋮ — перестановка строк.

          </p>

        )}

      </header>



      {error && <div className="alert alert--error">{error}</div>}

      {loading && <div className="alert">Парсинг 8101VC из capture…</div>}

      {!loading && !error && detail && (

        <LibraryVoiceParamsTable

          detail={detail}

          libraryId={libraryId}

          reviewIdFromUrl={reviewIdFromUrl}

          onReviewIdChange={handleReviewIdChange}

        />

      )}

    </section>

  );

}



import { useEffect, useRef } from 'react';
import { useNavigate } from 'react-router-dom';
import type { ParamMeta } from '../types/paramMeta';
import { formatRawRange } from '../types/paramMeta';
import { paramMetaToCatalogEntry } from '../utils/paramCatalog';
import { formatElementLabel } from '../utils/elementLabel';
import type { ParamListResume } from '../utils/paramListResume';
import { saveParamListResume } from '../utils/paramListResume';
import { resolveParamHealthStatus } from '../utils/paramWorkQueue';

type ParamsTableProps = {
  params: ParamMeta[];
  resume: ParamListResume | null;
  listFilters?: { tagQuery: string; groupQuery: string };
};

export function ParamsTable({ params, resume, listFilters }: ParamsTableProps) {
  const navigate = useNavigate();
  const resumeRowRef = useRef<HTMLTableRowElement | null>(null);
  const resumeIndex =
    resume !== null ? params.findIndex((param) => param.id === resume.paramId) : -1;
  const nextParam = resumeIndex >= 0 ? params[resumeIndex + 1] : undefined;

  useEffect(() => {
    if (resume === null || resumeIndex < 0) {
      return;
    }
    const row = resumeRowRef.current;
    if (row === null) {
      return;
    }
    const timer = window.setTimeout(() => {
      row.scrollIntoView({ block: 'center', behavior: 'smooth' });
    }, 80);
    return () => window.clearTimeout(timer);
  }, [resume?.paramId, resumeIndex]);

  function openParam(param: ParamMeta) {
    if (listFilters) {
      saveParamListResume({
        paramId: param.id,
        elementIndex: 0,
        tagQuery: listFilters.tagQuery,
        groupQuery: listFilters.groupQuery,
      });
    }
    navigate(`/params/${encodeURIComponent(param.id)}`);
  }

  if (params.length === 0) {
    return <p className="empty-state">Параметры не найдены.</p>;
  }

  return (
    <div className="table-wrap params-table-wrap">
      {resume !== null && resumeIndex < 0 && (
        <p className="panel-hint params-resume-hint">
          Последний параметр <span className="mono">{resume.paramId}</span> скрыт фильтром — сбросьте
          поиск или группу.
        </p>
      )}
      {resume !== null && resumeIndex >= 0 && nextParam && (
        <p className="panel-hint params-resume-hint">
          Следующий в списке:{' '}
          <button type="button" className="link-button mono" onClick={() => openParam(nextParam)}>
            {nextParam.id}
          </button>
          {nextParam.uiLabel ? ` (${nextParam.uiLabel})` : ''}
        </p>
      )}
      <table className="params-table">
        <thead>
          <tr>
            <th className="params-table-status-col" title="● зелёный — без ошибок · ● красный — ошибка · ○ — не проверен">
              ●
            </th>
            <th>Id</th>
            <th>groupId</th>
            <th>uiLabel</th>
            <th>perElement</th>
            <th>rawMin…rawMax</th>
            <th>decode / encode</th>
          </tr>
        </thead>
        <tbody>
          {params.map((param, index) => {
            const catalog = paramMetaToCatalogEntry(param);
            const isResume = resume !== null && param.id === resume.paramId;
            const isNext = resumeIndex >= 0 && index === resumeIndex + 1;
            const health = resolveParamHealthStatus(param);
            const rowClass = [
              'params-row',
              health === 'ok' ? 'params-row--health-ok' : '',
              health === 'error' ? 'params-row--health-error' : '',
              isResume ? 'params-row--last-visited' : '',
              isNext ? 'params-row--next-suggestion' : '',
            ]
              .filter(Boolean)
              .join(' ');

            return (
              <tr
                key={param.id}
                ref={isResume ? resumeRowRef : undefined}
                className={rowClass}
                tabIndex={0}
                onClick={() => openParam(param)}
                onKeyDown={(event) => {
                  if (event.key === 'Enter' || event.key === ' ') {
                    event.preventDefault();
                    openParam(param);
                  }
                }}
              >
                <td className="params-table-status-col">
                  {health === 'ok' && (
                    <span
                      className="param-status-dot param-status-dot--ok"
                      title="Без ошибок — связь SY99 подтверждена"
                      aria-label="Без ошибок"
                    />
                  )}
                  {health === 'error' && (
                    <span
                      className="param-status-dot param-status-dot--error"
                      title="Есть ошибка — требует исправления"
                      aria-label="Ошибка"
                    />
                  )}
                  {health === 'unknown' && (
                    <span
                      className="param-status-dot param-status-dot--unknown"
                      title="Ещё не проверен"
                      aria-label="Не проверен"
                    />
                  )}
                </td>
                <td className="mono">
                  {param.id}
                  {isResume && (
                    <span
                      className="params-row-marker"
                      title={
                        resume
                          ? `Возврат с карточки · эл. ${formatElementLabel(resume.elementIndex)}`
                          : undefined
                      }
                    >
                      ← здесь
                    </span>
                  )}
                  {isNext && (
                    <span className="params-row-marker params-row-marker--next" title="Следующий для проверки">
                      далее →
                    </span>
                  )}
                </td>
                <td className="mono">{catalog.groupId}</td>
                <td>{param.uiLabel}</td>
                <td>{param.perElement ? 'да' : 'нет'}</td>
                <td className="mono">{formatRawRange(param.rawMin, param.rawMax)}</td>
                <td className="mono">
                  {catalog.decode} / {catalog.encode}
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}

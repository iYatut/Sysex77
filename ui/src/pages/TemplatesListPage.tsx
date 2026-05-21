import { useEffect, useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { fetchAllTemplates } from '../api/templates';
import { ApiError } from '../api/client';
import type { ControllerTemplate } from '../types/controllerTemplate';
import { pageSlotCount } from '../types/controllerTemplate';

function parseApiError(err: unknown, fallback: string): string {
  if (err instanceof ApiError) {
    return `${err.message} (${err.status})`;
  }
  if (err instanceof Error) {
    return err.message;
  }
  return fallback;
}

export function TemplatesListPage() {
  const navigate = useNavigate();
  const [templates, setTemplates] = useState<ControllerTemplate[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [newId, setNewId] = useState('');

  useEffect(() => {
    let cancelled = false;

    async function load() {
      setLoading(true);
      setError(null);

      try {
        const data = await fetchAllTemplates();
        if (!cancelled) {
          setTemplates(data);
        }
      } catch (err) {
        if (!cancelled) {
          setError(parseApiError(err, 'Не удалось загрузить шаблоны'));
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

  function handleCreate() {
    const id = newId.trim();
    if (!id) {
      return;
    }
    navigate(`/templates/${encodeURIComponent(id)}?new=1`);
  }

  return (
    <section className="page">
      <header className="page-header">
        <div>
          <h1>Controller Templates</h1>
          <p className="page-subtitle">
            {loading ? 'Загрузка…' : `${templates.length} шаблон(ов)`}
          </p>
        </div>
      </header>

      <div className="create-template">
        <label className="filter-field">
          <span>Новый id</span>
          <input
            type="text"
            className="mono"
            placeholder="push2-default"
            value={newId}
            onChange={(event) => setNewId(event.target.value)}
          />
        </label>
        <button type="button" className="btn btn--primary" disabled={!newId.trim()} onClick={handleCreate}>
          Создать шаблон
        </button>
      </div>

      {error && <div className="alert alert--error">{error}</div>}
      {loading && <div className="alert">Загрузка шаблонов…</div>}

      {!loading && !error && (
        <div className="table-wrap">
          <table className="params-table">
            <thead>
              <tr>
                <th>id</th>
                <th>name</th>
                <th>pages</th>
                <th>slots</th>
              </tr>
            </thead>
            <tbody>
              {templates.map((template) => (
                <tr
                  key={template.id}
                  className="params-row"
                  tabIndex={0}
                  onClick={() => navigate(`/templates/${encodeURIComponent(template.id)}`)}
                  onKeyDown={(event) => {
                    if (event.key === 'Enter' || event.key === ' ') {
                      event.preventDefault();
                      navigate(`/templates/${encodeURIComponent(template.id)}`);
                    }
                  }}
                >
                  <td className="mono">{template.id}</td>
                  <td>{template.name}</td>
                  <td>{template.pages.length}</td>
                  <td>{pageSlotCount(template)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}

      {templates.length === 0 && !loading && !error && (
        <p className="empty-state">
          Шаблонов нет. <Link to="/templates/push2-default?new=1">Создать первый</Link>
        </p>
      )}
    </section>
  );
}

import { useEffect, useState } from 'react';
import { Link, useNavigate, useParams, useSearchParams } from 'react-router-dom';
import { fetchAllParams } from '../api/params';
import { createTemplate, fetchTemplateById, updateTemplate } from '../api/templates';
import { ApiError } from '../api/client';
import { ParamPicker } from '../components/ParamPicker';
import type { ParamMeta } from '../types/paramMeta';
import {
  emptyPage,
  emptySlot,
  emptyTemplate,
  type ControllerTemplate,
  type ControllerTemplatePage,
  type ControllerTemplateSlot,
} from '../types/controllerTemplate';

function parseApiError(err: unknown, fallback: string): string {
  if (err instanceof ApiError) {
    return `${err.message} (${err.status})`;
  }
  if (err instanceof Error) {
    return err.message;
  }
  return fallback;
}

export function TemplateEditorPage() {
  const { id = '' } = useParams();
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const isNew = searchParams.get('new') === '1';

  const [params, setParams] = useState<ParamMeta[]>([]);
  const [template, setTemplate] = useState<ControllerTemplate | null>(null);
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [saveMessage, setSaveMessage] = useState<string | null>(null);

  useEffect(() => {
    let cancelled = false;

    async function load() {
      if (!id) {
        setError('Не указан id шаблона');
        setLoading(false);
        return;
      }

      setLoading(true);
      setError(null);
      setSaveMessage(null);

      try {
        const allParams = await fetchAllParams();
        if (cancelled) {
          return;
        }
        setParams(allParams);

        if (isNew) {
          setTemplate(emptyTemplate(id, 'New template'));
          return;
        }

        const data = await fetchTemplateById(id);
        if (!cancelled) {
          setTemplate(data);
        }
      } catch (err) {
        if (!cancelled) {
          if (isNew && err instanceof ApiError && err.status === 404) {
            setTemplate(emptyTemplate(id, 'New template'));
          } else {
            setError(parseApiError(err, 'Не удалось загрузить шаблон'));
          }
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
  }, [id, isNew]);

  function updateTemplateState(updater: (current: ControllerTemplate) => ControllerTemplate) {
    setTemplate((current) => (current ? updater(current) : current));
    setSaveMessage(null);
  }

  function updatePage(pageIndex: number, patch: Partial<ControllerTemplatePage>) {
    updateTemplateState((current) => ({
      ...current,
      pages: current.pages.map((page, index) => (index === pageIndex ? { ...page, ...patch } : page)),
    }));
  }

  function updateSlot(pageIndex: number, slotIndex: number, patch: Partial<ControllerTemplateSlot>) {
    updateTemplateState((current) => ({
      ...current,
      pages: current.pages.map((page, pi) =>
        pi === pageIndex
          ? {
              ...page,
              slots: page.slots.map((slot, si) => (si === slotIndex ? { ...slot, ...patch } : slot)),
            }
          : page,
      ),
    }));
  }

  function addPage() {
    updateTemplateState((current) => ({
      ...current,
      pages: [...current.pages, emptyPage(`Page ${current.pages.length + 1}`)],
    }));
  }

  function removePage(pageIndex: number) {
    updateTemplateState((current) => ({
      ...current,
      pages: current.pages.filter((_, index) => index !== pageIndex),
    }));
  }

  function addSlot(pageIndex: number) {
    updateTemplateState((current) => ({
      ...current,
      pages: current.pages.map((page, index) =>
        index === pageIndex ? { ...page, slots: [...page.slots, emptySlot()] } : page,
      ),
    }));
  }

  function removeSlot(pageIndex: number, slotIndex: number) {
    updateTemplateState((current) => ({
      ...current,
      pages: current.pages.map((page, pi) =>
        pi === pageIndex
          ? { ...page, slots: page.slots.filter((_, si) => si !== slotIndex) }
          : page,
      ),
    }));
  }

  async function handleSave() {
    if (!id || !template) {
      return;
    }

    setSaving(true);
    setError(null);
    setSaveMessage(null);

    const payload: ControllerTemplate = { ...template, id };

    try {
      const saved = isNew ? await createTemplate(id, payload) : await updateTemplate(id, payload);
      setTemplate(saved);
      setSaveMessage(isNew ? 'Создано' : 'Сохранено');
      if (isNew) {
        navigate(`/templates/${encodeURIComponent(id)}`, { replace: true });
      }
    } catch (err) {
      setError(parseApiError(err, 'Не удалось сохранить'));
    } finally {
      setSaving(false);
    }
  }

  return (
    <section className="page">
      <header className="page-header page-header--detail">
        <div>
          <Link to="/templates" className="back-link">
            ← К шаблонам
          </Link>
          <h1>{id || 'Шаблон'}</h1>
        </div>
        <button
          type="button"
          className="btn btn--primary"
          disabled={loading || saving || !template}
          onClick={() => void handleSave()}
        >
          {saving ? 'Сохранение…' : isNew ? 'Создать' : 'Сохранить'}
        </button>
      </header>

      {loading && <div className="alert">Загрузка…</div>}
      {error && <div className="alert alert--error">{error}</div>}
      {saveMessage && <div className="alert alert--success">{saveMessage}</div>}

      {template && (
        <div className="template-editor">
          <section className="panel">
            <h2>Template</h2>
            <div className="form-grid">
              <label className="form-field">
                <span>id</span>
                <input type="text" className="mono" value={template.id} readOnly />
              </label>
              <label className="form-field">
                <span>name</span>
                <input
                  type="text"
                  value={template.name}
                  onChange={(event) =>
                    updateTemplateState((current) => ({ ...current, name: event.target.value }))
                  }
                />
              </label>
            </div>
          </section>

          {template.pages.map((page, pageIndex) => (
            <section key={pageIndex} className="panel">
              <div className="panel-toolbar">
                <h2>Page {pageIndex + 1}</h2>
                <button
                  type="button"
                  className="btn btn--ghost"
                  disabled={template.pages.length <= 1}
                  onClick={() => removePage(pageIndex)}
                >
                  Удалить страницу
                </button>
              </div>

              <label className="form-field">
                <span>name</span>
                <input
                  type="text"
                  value={page.name}
                  onChange={(event) => updatePage(pageIndex, { name: event.target.value })}
                />
              </label>

              <div className="slots-list">
                {page.slots.map((slot, slotIndex) => (
                  <div key={slotIndex} className="slot-card">
                    <div className="slot-card__header">
                      <strong>Slot {slotIndex + 1}</strong>
                      <button
                        type="button"
                        className="btn btn--ghost"
                        onClick={() => removeSlot(pageIndex, slotIndex)}
                      >
                        Удалить
                      </button>
                    </div>

                    <label className="form-field">
                      <span>label</span>
                      <input
                        type="text"
                        value={slot.label}
                        onChange={(event) =>
                          updateSlot(pageIndex, slotIndex, { label: event.target.value })
                        }
                      />
                    </label>

                    <ParamPicker
                      params={params}
                      paramId={slot.paramId}
                      elementIndex={slot.elementIndex}
                      onChange={(paramId, elementIndex) =>
                        updateSlot(pageIndex, slotIndex, { paramId, elementIndex })
                      }
                    />
                  </div>
                ))}
              </div>

              <button type="button" className="btn btn--ghost" onClick={() => addSlot(pageIndex)}>
                + Добавить слот
              </button>
            </section>
          ))}

          <button type="button" className="btn btn--primary" onClick={addPage}>
            + Добавить страницу
          </button>
        </div>
      )}
    </section>
  );
}

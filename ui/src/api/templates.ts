import type { ControllerTemplate } from '../types/controllerTemplate';
import { apiFetch } from './client';

export function fetchAllTemplates(): Promise<ControllerTemplate[]> {
  return apiFetch<ControllerTemplate[]>('/controller-templates');
}

export function fetchTemplateById(id: string): Promise<ControllerTemplate> {
  return apiFetch<ControllerTemplate>(`/controller-templates/${encodeURIComponent(id)}`);
}

export function createTemplate(id: string, template: ControllerTemplate): Promise<ControllerTemplate> {
  return apiFetch<ControllerTemplate>(`/controller-templates/${encodeURIComponent(id)}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(template),
  });
}

export function updateTemplate(id: string, template: ControllerTemplate): Promise<ControllerTemplate> {
  return apiFetch<ControllerTemplate>(`/controller-templates/${encodeURIComponent(id)}`, {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(template),
  });
}

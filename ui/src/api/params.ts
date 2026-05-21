import type { ParamMeta } from '../types/paramMeta';
import type { ParamDumpRow } from '../types/paramDump';
import type { ParamValueMap } from '../types/paramValueMap';
import type { ParamMetaPatch } from '../utils/paramForm';
import { apiFetch } from './client';

export function fetchAllParams(): Promise<ParamMeta[]> {
  return apiFetch<ParamMeta[]>('/params');
}

export function fetchParamById(id: string): Promise<ParamMeta> {
  return apiFetch<ParamMeta>(`/params/${encodeURIComponent(id)}`);
}

export function fetchParamValueMap(
  id: string,
  elementIndex = 0,
  sysexDevice = 16,
): Promise<ParamValueMap> {
  const query = new URLSearchParams({
    elementIndex: String(elementIndex),
    sysexDevice: String(sysexDevice),
  });
  return apiFetch<ParamValueMap>(
    `/params/${encodeURIComponent(id)}/value-map?${query.toString()}`,
  );
}

export function fetchCurrentDump(): Promise<ParamDumpRow[]> {
  return apiFetch<ParamDumpRow[]>('/dump/current');
}

export function updateParamById(id: string, patch: ParamMetaPatch): Promise<ParamMeta> {
  return apiFetch<ParamMeta>(`/params/${encodeURIComponent(id)}`, {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(patch),
  });
}

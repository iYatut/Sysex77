import type { ParamCatalogEntry } from '../types/paramCatalog';
import type { ParamMeta } from '../types/paramMeta';
import type { ParamDumpRow } from '../types/paramDump';
import type { ParamValueMap } from '../types/paramValueMap';
import type { ParamMetaPatch } from '../utils/paramForm';
import type { LogCompareResult } from '../types/logCompare';
import type { RowOverride } from '../types/paramMeta';
import type { ParamVerificationState } from '../utils/paramVerification';
import { paramMetaListToCatalog, serializeParamCatalog } from '../utils/paramCatalog';
import { verificationPatchForPersist } from '../utils/paramVerification';
import { apiFetch } from './client';

export function fetchAllParams(): Promise<ParamMeta[]> {
  return apiFetch<ParamMeta[]>('/params');
}

/** Дескriptor-only представление GET /params (без sy99Verification и truth-полей). */
export async function fetchParamCatalog(): Promise<ParamCatalogEntry[]> {
  const params = await fetchAllParams();
  return paramMetaListToCatalog(params);
}

/** PATCH Meta-полей: только дескriptor, без truth (sy99Verification не включается). */
export function catalogPatchFromFormPatch(patch: ParamMetaPatch): ParamMetaPatch {
  const { sy99Verification: _ignored, ...catalog } = patch as ParamMetaPatch & {
    sy99Verification?: unknown;
  };
  return catalog;
}

export function updateParamCatalogFields(
  id: string,
  patch: ParamMetaPatch,
): Promise<ParamMeta> {
  return updateParamById(id, catalogPatchFromFormPatch(patch));
}

/** JSON для params_meta.json: только ParamCatalogEntry[]. */
export function buildParamCatalogJson(params: ParamMeta[]): ParamCatalogEntry[] {
  return serializeParamCatalog(paramMetaListToCatalog(params));
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

export function updateParamVerification(
  id: string,
  elementIndex: number,
  state: ParamVerificationState,
  param?: Pick<ParamMeta, 'perElement'> | null,
): Promise<ParamMeta> {
  return updateParamById(id, verificationPatchForPersist(param, elementIndex, state));
}

export type FocusSy99Result = {
  ok: boolean;
  paramId: string;
  elementIndex: number;
  message: string;
};

export function focusParamInSy99(
  id: string,
  elementIndex = 0,
  options?: { raw?: number | null; sysexDevice?: number },
): Promise<FocusSy99Result> {
  const body: Record<string, unknown> = {
    paramId: id,
    elementIndex,
    sysexDevice: options?.sysexDevice ?? 16,
  };
  if (options?.raw !== undefined && options.raw !== null) {
    body.raw = options.raw;
  }
  return apiFetch<FocusSy99Result>('/focus-sy99', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  });
}

export function compareParamLog(
  id: string,
  elementIndex: number,
  logText: string,
  rowOverrides: Record<string, RowOverride> = {},
  sysexDevice = 16,
): Promise<LogCompareResult> {
  const query = new URLSearchParams({
    elementIndex: String(elementIndex),
    sysexDevice: String(sysexDevice),
  });
  return apiFetch<LogCompareResult>(
    `/params/${encodeURIComponent(id)}/compare-log?${query.toString()}`,
    {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ logText, rowOverrides }),
    },
  );
}

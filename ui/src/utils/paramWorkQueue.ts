import type { ParamMeta } from '../types/paramMeta';

/** Параметры без известных ошибок (двусторонняя связь SY99 подтверждена). */
export const PARAM_OK_IDS: readonly string[] = [
  'ELMODE',
  'WOL',
  'ELVL',
  'ELDT',
  'ELNS',
  'OUTSEL',
  'EFLN1EL',
  'EFSDLV',
  'EFSDVSNS',
  'EFSDSCL',
  'EFMODE',
] as const;

/** Явные ошибки — дополнять по мере обнаружения. */
export const PARAM_ERROR_IDS: readonly string[] = [] as const;

const OK_SET = new Set<string>(PARAM_OK_IDS);
const ERROR_SET = new Set<string>(PARAM_ERROR_IDS);

export type ParamHealthStatus = 'ok' | 'error' | 'unknown';

export const PARAM_CATALOG_TOTAL = 37;

export function resolveParamHealthStatus(param: ParamMeta): ParamHealthStatus {
  if (ERROR_SET.has(param.id)) {
    return 'error';
  }
  if (OK_SET.has(param.id)) {
    return 'ok';
  }
  return 'unknown';
}

/** @deprecated use resolveParamHealthStatus === 'ok' */
export function isParamIntegrationDone(param: Pick<ParamMeta, 'id'>): boolean {
  return OK_SET.has(param.id);
}

export function paramHealthSummary(params: ParamMeta[]): {
  total: number;
  ok: number;
  error: number;
  unknown: number;
  okParams: ParamMeta[];
  errorParams: ParamMeta[];
  unknownParams: ParamMeta[];
} {
  const okParams: ParamMeta[] = [];
  const errorParams: ParamMeta[] = [];
  const unknownParams: ParamMeta[] = [];

  for (const param of params) {
    const status = resolveParamHealthStatus(param);
    if (status === 'ok') {
      okParams.push(param);
    } else if (status === 'error') {
      errorParams.push(param);
    } else {
      unknownParams.push(param);
    }
  }

  return {
    total: params.length,
    ok: okParams.length,
    error: errorParams.length,
    unknown: unknownParams.length,
    okParams,
    errorParams,
    unknownParams,
  };
}

/** @deprecated use paramHealthSummary */
export function paramWorkQueueSummary(params: ParamMeta[]) {
  const s = paramHealthSummary(params);
  return {
    total: s.total,
    done: s.ok,
    todo: s.unknown + s.error,
    todoParams: [...s.errorParams, ...s.unknownParams],
    doneParams: s.okParams,
  };
}

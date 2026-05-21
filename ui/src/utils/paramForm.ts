import type { ParamMeta } from '../types/paramMeta';

export type ParamMetaPatch = {
  uiLabel?: string;
  group?: string;
  perElement?: boolean;
  rawMin?: number;
  rawMax?: number;
  enumNames?: string[];
};

export type ParamMetaForm = {
  uiLabel: string;
  group: string;
  perElement: boolean;
  rawMin: number;
  rawMax: number;
  enumNames: string[];
};

export function paramToForm(param: ParamMeta): ParamMetaForm {
  return {
    uiLabel: param.uiLabel,
    group: param.group,
    perElement: param.perElement,
    rawMin: param.rawMin,
    rawMax: param.rawMax,
    enumNames: [...param.enumNames],
  };
}

export function formToPatch(form: ParamMetaForm): ParamMetaPatch {
  return {
    uiLabel: form.uiLabel,
    group: form.group,
    perElement: form.perElement,
    rawMin: form.rawMin,
    rawMax: form.rawMax,
    enumNames: form.enumNames,
  };
}

export function isEnumParam(param: ParamMeta | ParamMetaForm): boolean {
  return param.enumNames.length > 0;
}

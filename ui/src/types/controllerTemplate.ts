export interface ControllerTemplateSlot {
  label: string;
  paramId: string;
  elementIndex: number;
}

export interface ControllerTemplatePage {
  name: string;
  slots: ControllerTemplateSlot[];
}

export interface ControllerTemplate {
  id: string;
  name: string;
  pages: ControllerTemplatePage[];
}

export function emptySlot(): ControllerTemplateSlot {
  return { label: '', paramId: '', elementIndex: 0 };
}

export function emptyPage(name = 'Page'): ControllerTemplatePage {
  return { name, slots: [] };
}

export function emptyTemplate(id = '', name = ''): ControllerTemplate {
  return { id, name, pages: [emptyPage('Page 1')] };
}

export function pageSlotCount(template: ControllerTemplate): number {
  return template.pages.reduce((sum, page) => sum + page.slots.length, 0);
}

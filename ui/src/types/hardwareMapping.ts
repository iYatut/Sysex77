export interface HardwareBinding {
  cc: number;
  channel: number;
  page: number;
  slotIndex: number;
}

export interface HardwareMapping {
  deviceId: string;
  templateId: string;
  inputPortMatch?: string;
  feedbackPortMatch?: string;
  activePage?: number;
  bindings: HardwareBinding[];
}

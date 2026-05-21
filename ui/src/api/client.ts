import { API_BASE_URL } from './config';

export class ApiError extends Error {
  readonly status: number;
  readonly body: unknown;

  constructor(message: string, status: number, body: unknown) {
    super(message);
    this.name = 'ApiError';
    this.status = status;
    this.body = body;
  }
}

async function parseJsonResponse(response: Response): Promise<unknown> {
  const text = await response.text();
  if (!text) {
    return null;
  }

  try {
    return JSON.parse(text) as unknown;
  } catch {
    return text;
  }
}

const API_OFFLINE_HINT =
  'Запустите Sysex77.exe — REST API слушает http://127.0.0.1:8765 (см. Sysex77_OUT.log: [ParamAPI] listening).';

function isProxyOrOfflineStatus(status: number): boolean {
  return status === 0 || status === 502 || status === 503 || status === 504;
}

export async function apiFetch<T>(path: string, init?: RequestInit): Promise<T> {
  const normalizedPath = path.startsWith('/') ? path : `/${path}`;
  const url = `${API_BASE_URL}${normalizedPath}`;

  let response: Response;
  try {
    response = await fetch(url, {
      ...init,
      headers: {
        Accept: 'application/json',
        ...(init?.headers ?? {}),
      },
    });
  } catch {
    throw new ApiError(API_OFFLINE_HINT, 0, null);
  }

  const body = await parseJsonResponse(response);

  if (!response.ok) {
    if (isProxyOrOfflineStatus(response.status)) {
      throw new ApiError(API_OFFLINE_HINT, response.status, body);
    }

    const message =
      typeof body === 'object' &&
      body !== null &&
      'error' in body &&
      typeof (body as { error: unknown }).error === 'string'
        ? (body as { error: string }).error
        : `HTTP ${response.status}`;

    throw new ApiError(message, response.status, body);
  }

  return body as T;
}

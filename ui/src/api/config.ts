const DEFAULT_PORT = 8765;

function resolveApiBaseUrl(): string {
  const fromEnv = import.meta.env.VITE_API_BASE_URL?.trim();
  if (fromEnv) {
    return fromEnv.replace(/\/$/, '');
  }

  if (import.meta.env.DEV) {
    return '/api';
  }

  const port = import.meta.env.VITE_API_PORT?.trim() || String(DEFAULT_PORT);
  return `http://127.0.0.1:${port}/api`;
}

export const API_BASE_URL = resolveApiBaseUrl();

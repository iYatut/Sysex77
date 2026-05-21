import { useEffect, useState } from 'react';
import { apiFetch, ApiError } from '../api/client';

export function ApiOfflineBanner() {
  const [offline, setOffline] = useState(false);
  const [checking, setChecking] = useState(true);

  useEffect(() => {
    let cancelled = false;

    async function ping() {
      setChecking(true);
      try {
        await apiFetch<unknown>('/params', { method: 'GET' });
        if (!cancelled) {
          setOffline(false);
        }
      } catch (err) {
        if (!cancelled) {
          const unreachable =
            err instanceof ApiError &&
            (err.status === 0 || err.status === 502 || err.status === 503 || err.status === 504);
          setOffline(unreachable);
        }
      } finally {
        if (!cancelled) {
          setChecking(false);
        }
      }
    }

    void ping();
    const interval = window.setInterval(() => void ping(), 8000);

    return () => {
      cancelled = true;
      window.clearInterval(interval);
    };
  }, []);

  if (checking || !offline) {
    return null;
  }

  return (
    <div className="alert alert--error api-offline-banner" role="alert">
      <strong>API недоступен.</strong> Vite проксирует <code>/api</code> на{' '}
      <code>127.0.0.1:8765</code>, но там никто не слушает. Запустите{' '}
      <code>Sysex77.exe</code> (Release:{' '}
      <code>Sysex77-master/Builds/VisualStudio2022/x64/Release/App/</code>), затем обновите
      страницу.
    </div>
  );
}

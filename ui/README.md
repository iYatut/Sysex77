# SY99 Parameters UI

React/TypeScript SPA (Vite) для просмотра метаданных параметров через JUCE Param API.

## Запуск

**Порядок важен:** сначала бэкенд, потом UI.

1. Соберите и запустите **Sysex77.exe** (в `Main.cpp` вызывается `getSy99ParamApiServer().start()` на порту **8765**).
   - Обычный путь после сборки: `Sysex77-master/Builds/VisualStudio2022/x64/Release/App/Sysex77.exe`
   - В логе `Sysex77_OUT.log` рядом с exe должна быть строка `[ParamAPI] listening on http://127.0.0.1:8765`
2. Установите зависимости и запустите dev-сервер:

```bash
cd ui
npm install
npm run dev
```

3. Откройте `http://localhost:5173/params`.

### Ошибка `ECONNREFUSED 127.0.0.1:8765`

Vite проксирует `/api` на JUCE API. Сообщение значит, что **Sysex77 не запущен** или порт занят другим процессом. Запустите exe и проверьте в PowerShell:

```powershell
Invoke-WebRequest http://127.0.0.1:8765/api/params -UseBasicParsing
```

Ожидается HTTP 200.

## API

- Dev: запросы идут на `/api` через Vite proxy → `http://127.0.0.1:8765`.
- Production / явный URL: задайте `VITE_API_BASE_URL` (см. `.env.example`).

## Типы

Типы `ParamMeta` генерируются из `fixtures/paramMeta.sample.json`:

```bash
npm run generate:types
```

Результат: `src/types/paramMeta.generated.ts`.

## Маршруты

| Путь | Описание |
|------|----------|
| `/params` | Таблица всех параметров с фильтрами |
| `/params/:id` | CMS-редактор Meta + текущие значения из dump |
| `/dump` | Таблица текущих значений LiveSynthState |
| `/templates` | Список шаблонов контроллеров |
| `/templates/:id` | Редактор страниц и слотов шаблона |

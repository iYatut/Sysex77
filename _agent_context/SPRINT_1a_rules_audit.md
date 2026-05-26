# Sprint 1a — Аудит правил проекта (2026-05-25)

**Статус:** завершён (только анализ).  
**Sprint 1b:** **done 2026-05-25** — правила опубликованы в [`07_architecture_index.md`](07_architecture_index.md).

---

## Идея проекта (зафиксировано)

| Слой | Роль | Authoritative? |
|------|------|----------------|
| **JUCE Sysex77** | Продукт: editor, Librairie, MIDI, parse, LiveSynthState | **Да** — runtime |
| **React `ui/` + Param API :8765** | Debug: catalog, Library review, bindings visibility | **Нет** — mirror |
| **Fixtures `_validate_*.py`** | Gate перед изменением parse/registry | **Да** — regression |
| **`sy99_param_bindings.json`** | Подтверждённые offsets + bindStatus (37 registry) | **Да** — для bindings |
| **`03_parameter_map.csv`** | Audit / tracker (~183 строк) | **Нет** — docs only |

**Не строить:** 11 хранилищ по ELMODE, новую «главную» param table, auto-promote review → bindings.

---

## Inventory — все источники правил

| # | Файл | Что задаёт | Вердикт | Действие в 1b |
|---|------|------------|---------|---------------|
| 1 | [`00_SPEC.md`](00_SPEC.md) | Goal, constraints, deliverables | ⚠️ UPDATE | Два UI; bindings > CSV; fixture gate |
| 2 | [`06_agent_prompts.md`](06_agent_prompts.md) | Пошаговые промпты C++ | ⚠️ UPDATE | Шапка: + `ui/` debug |
| 3 | [`library_binding_workflow.md`](library_binding_workflow.md) | 8-step цикл, bindStatus | ✅ KEEP | Ссылка из architecture index |
| 4 | [`library_reviews/README.md`](library_reviews/README.md) | Review handoff агенту | ✅ KEEP | + workflow «закрыть отчёт» |
| 5 | [`SYM7_library_sync_progress.md`](SYM7_library_sync_progress.md) | Nav single source, decision log | ✅ KEEP | DO NOT duplicate bank math |
| 6 | [`lm_8101_offsets.md`](lm_8101_offsets.md) | Bulk offsets, EFSDLV 0040 | ✅ KEEP + VISIBLE | Anti-dup: не править 8101 poison |
| 7 | [`09_confirmed_addresses.md`](09_confirmed_addresses.md) | Чистые live-адреса | ✅ KEEP | Запрет MIDI_MAP_OBSERVATIONS |
| 8 | [`ui/README.md`](../ui/README.md) | Запуск React, API proxy | ✅ KEEP | Связать с 00_SPEC; + маршруты Library |
| 9 | [`01_code_map.md`](01_code_map.md) | Карта файлов | ⚠️ DRIFT | Sprint 1c — sync с кодом |
| 10 | [`sy99_bulk_dump_request.md`](sy99_bulk_dump_request.md) | Dump request format | ⚠️ DRIFT | Sprint 1c |
| 11 | Inline C++ comments | authoritative / poison / fallback | ✅ KEEP | Вынести ключевые в R-* |
| 12 | Cursor user rules | minimal diff, git safety | ✅ KEEP | — |
| 13 | Plan `bankclick_consolidation` | Sprint roadmap | ✅ KEEP | Текущий фокус |

---

## Конфликты (старые правила vs реальность)

### K1 — «Нет React / npm»

| | |
|---|---|
| **Где** | `00_SPEC.md` L28–98, `06_agent_prompts.md` L3–6 |
| **Реальность** | `ui/` — Vite SPA, Library review, `:8765` API |
| **Вред** | Агент игнорирует React или плодит C++ вместо UI fix |
| **Решение** | **UPDATE:** JUCE = продукт; React = debug mirror (намеренно) |

### K2 — «Registry = CSV»

| | |
|---|---|
| **Где** | `00_SPEC.md` deliverables §1 |
| **Реальность** | Runtime registry = `sy99_param_bindings.json` + `kMetaTable`; CSV = audit |
| **Вред** | Правки CSV не попадают в editor |
| **Решение** | **UPDATE:** bindings.json authoritative для 37 params |

### K3 — «Bind UI через valueSysexIn»

| | |
|---|---|
| **Где** | `library_binding_workflow.md` §8 |
| **Реальность** | 37 registry → `LiveSynthState` + `applyLiveSynthStateToEditor`; valueSysexIn = legacy |
| **Вред** | Два inbound path (path A vs B) |
| **Решение** | **UPDATE** workflow §8: registry vs legacy split; unify = Sprint 8 |

### K4 — Нет «freeze новых слоёв»

| | |
|---|---|
| **Где** | — (не было явно) |
| **Реальность** | BankClick, Lazy0040, Invoke callbacks |
| **Вред** | «Новый этаж» поверх requestSysex |
| **Решение** | **ADD** R-NEW-1 в architecture index |

### K5 — Fixture gate спрятан

| | |
|---|---|
| **Где** | Только `library_binding_workflow.md` §5 |
| **Реальность** | Агенты меняют parse без `_validate_*.py` |
| **Решение** | **VISIBLE** в `00_SPEC` Constraints + R-NEW-2 |

### K6 — Дубли bulk map

| | |
|---|---|
| **Где** | `Sy99LibraryBulkRead.h`, `generate-master-catalog.py` REGISTRY_BULK_READ |
| **Реальность** | Два списка field→offset |
| **Вред** | Library inDump ≠ editor (SPTPNT class) |
| **Решение** | **ADD** R-NEW-3; Sprint 7 Python ← bindings |

### K7 — React API double `/api`

| | |
|---|---|
| **Где** | — (не документировано) |
| **Реальность** | `libraryReview.ts` had `/api/library/...` + base `/api` |
| **Вред** | 404 not found |
| **Решение** | **FIXED** Sprint 0; **ADD** R-NEW-4 в docs |

---

## Правила KEEP (смысл не менять)

| ID | Правило | Источник |
|----|---------|----------|
| R-KEEP-1 | Throttle ≤30 msg/s, echo guard 50 ms | `00_SPEC` Constraints |
| R-KEEP-2 | Не читать `MIDI_MAP_OBSERVATIONS.md` для адресов | `00_SPEC`, `09_confirmed_addresses` |
| R-KEEP-3 | Bank math только `Sy99LibraryNavigation.h` | SYM7 progress § Library navigation |
| R-KEEP-4 | Один param за итерацию + fixture PASS | `library_binding_workflow` |
| R-KEEP-5 | Minimal patch, не рефакторить без нужды | user rules |
| R-KEEP-6 | Review → agent → bindings вручную (не auto-promote) | workflow §6–7 |
| R-KEEP-7 | EFSDLV elmode 4/8: authoritative **0040 @100/@104** | bindings, lm_8101_offsets |
| R-KEEP-8 | Synth nav (3,4) never blocked by 0040 fetch | plan 8 scenarios |
| R-KEEP-9 | 3 слота данных (live / 8101 / 0040) — не сливать в один | architecture decision |
| R-KEEP-10 | Review draft sessionStorage + server JSON; закрытие вручную | Sprint 0 |

---

## Правила ADD (новые, для 1b)

| ID | Правило |
|----|---------|
| **R-NEW-1** | Новый `.h` / global callback — только после sprint gate + decision log |
| **R-NEW-2** | Правка `YamahaLmVoiceDump` / parse offset — `_validate_*.py` PASS до и после |
| **R-NEW-3** | Bulk field map — только в `sy99_param_bindings.json`, не третий hardcoded list |
| **R-NEW-4** | React `apiFetch`: пути `/library/...`, не `/api/library/...` (base уже `/api`) |
| **R-NEW-5** | Registry params: inbound → `LiveSynthState`; `valueSysexIn` = legacy only (цель Sprint 8) |
| **R-NEW-6** | Library nav: не дублировать bank math в Librairie / MidiDemo / VoicesTableModel |
| **R-NEW-7** | Library API read path = тот же resolve 0040, что JUCE ingest (Sprint 3) |

---

## Agent Decision Tree (куда править)

```
Задача                          → Куда
─────────────────────────────────────────────────────────
Metadata / bindStatus / catalog → sy99_param_bindings.json (+ ui/fixtures copy)
Parse offset / lm* fields       → YamahaLmVoiceDump + Sy99ParamRegistry + fixtures
Editor slider / TX              → Voice.h + applyLiveSynthStateToEditor
Library inDump / green dot      → Sy99LibraryApi (read), не отдельный parser
Navigation / slot A1–D16        → Sy99LibraryNavigation.h ONLY
Library review notes            → UI review → JSON; promote в bindings вручную
Новый async слой / callback     → STOP → sprint gate
```

---

## Anti-duplication checklist (для 1b)

Перед merge / sprint gate ответить «нет» на все пункты:

- [ ] Не дублирую bulk field map (есть запись в bindings.json)?
- [ ] Не дублирую bank math (использую Sy99LibraryNavigation)?
- [ ] Не добавляю второй inbound path для registry param?
- [ ] Не добавляю `.h`/callback без записи в decision log?
- [ ] Прогнал `_validate_*.py` если трогал parse/registry?
- [ ] React API path без двойного `/api`?

---

## Что НЕ является проблемой (не «чинить»)

| Зона | Почему OK |
|------|-----------|
| React ↔ C++ Param API mirror | Намеренный debug |
| `ui/fixtures/` ↔ `_agent_context/fixtures/` | Build vs agent |
| `sy99_master_catalog.json` ~214 vs registry 37 | Scope разный |
| `03_parameter_map.csv` | Audit only |

---

## Gate Sprint 1a

- [x] Inventory заполнен (этот файл)
- [x] Конфликты K1–K7 с решениями
- [x] KEEP / ADD / Decision Tree / Checklist
- [x] Идея проекта: JUCE product + React debug + fixtures gate

**Sprint 1b (2026-05-25):** правки `00_SPEC`, `07_architecture_index`, `06_agent_prompts`, `TEST_STATUS` smoke matrix, workflow §8 — **done**.

---

## Согласование с roadmap

| Sprint | Связь с правилами |
|--------|-------------------|
| 0 | R-NEW-4, R-KEEP-10, review workflow |
| **1b** | Применить этот аудит в docs |
| 2 | R-NEW-2 enforcement |
| 3 | R-NEW-7 |
| 5 | R-NEW-1, консолидация без новых слоёв |
| 6+ | R-KEEP-4, workflow |
| 8 | R-NEW-5 |

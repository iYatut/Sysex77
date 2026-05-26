# Ручное тестирование Sysex77 ↔ SY99/SY77

Ручная фиксация проверок **без** изменения кодовой логики MIDI. Детали реализации и адреса — в **`_agent_context/05_missing_audit.md`** и смежных файлах `_agent_context/`.

---

## Значения статусов

| Код | Значение |
|-----|----------|
| **PASS** | Проверено, работает как ожидается |
| **FAIL** | Проверено, не работает |
| **BLOCKED** | Нельзя проверить из‑за среды (сборка, маршрут MIDI, недоступные логи и т.п.) |
| **N/A** | Не применимо к этому прогону / столбцу |
| **TODO** | Ещё не проверяли |

## Правило интерпретации колонок

- **Status** отражает **только live**‑работу параметра: колонки **App→Synth** и **Synth→App** в текущей сессии.
- **Startup Sync**: пока не реализованы **startup patch dump** и/или **current patch request**, в этой колонке использовать **N/A** (нет основания ставить **FAIL** из‑за отсутствия функции).
- **PASS** / **FAIL** по **Startup Sync** — только **после** реализации и проверки этой функции.

---

## Current Manual Test Status

| Area | Parameter/Function | App→Synth | Synth→App | Startup Sync | Status | Notes |
|------|---------------------|-----------|-----------|--------------|--------|-------|
| Voice | ELMODE (`comboMode`) | PASS | PASS | N/A | PASS | Live: оба направления подтверждены. Startup Sync = N/A до patch dump / current patch request при старте. |
| Voice | WOL / Master Volume (`sliderMaster`) | PASS | PASS | N/A | PASS | Live: оба направления подтверждены. Startup Sync = N/A до стартовой подгрузки состояния. |
| Voice | WPBR (`sliderWPBR`) | PASS | PASS | N/A | PASS | SysEx по 02 00 00 28; live App→Synth и Synth→App подтверждены. Startup Sync = N/A до реализации стартовой синхронизации. |
| Voice | ATPBR (`sliderATPBR`) | PASS | PASS | N/A | PASS | App→Synth и Synth→App подтверждены после исправления кодирования Yamaha symmetric 12 для адреса 02 00 00 29. Отрицательная зона теперь совпадает с SY99: 1C..11, затем 00, затем 01..0C. Доп.: при активации/движении ATPBR с приложения на SY99 открывается страница 276 — похоже на page jump к нужному экрану; точный focus параметра не подтверждён. |
| Voice | PMASN (`comboPMASN`) | PASS | PASS | N/A | PASS | Live App→Synth и Synth→App подтверждены. SysEx по адресу 02 00 00 2A проходит последовательно по значениям 00..79 (0..121). Ограничение: в редакторе/обмене подтверждён номер значения, но не человекочитаемые названия/описания каналов/источников как на синтезаторе. |
| Voice | AFTMD (`comboAFTMD`) | PASS | PASS | N/A | PASS | Live App→Synth и Synth→App подтверждены. SysEx по адресу 02 00 00 42 проходит по значениям 00..04. Режимы совпадают в обе стороны: all, top, btm, hi, low. Замечание: текстовые подписи между редактором и SY99 местами различаются (например Bottom vs btm, split hi vs Hi, split low vs low), это отдельная UI/data-mapping задача, а не ошибка live-связи. |
| Voice | MCTUN (`sliderMCTUN`) | PASS | PASS | N/A | PASS | Live App→Synth и Synth→App подтверждены. SysEx по адресу 02 00 00 3A проходит последовательно, связь стабильна в обе стороны. Замечание: текстовые названия/подписи в редакторе могут не совпадать с формулировками на SY99; это отдельная UI/data-mapping задача, а не ошибка live-связи. |
| Voice | VNAM / Voice Name (`editName`) | FAIL | PASS | N/A | FAIL | Live: с синта в приложение — OK; имя из редактора на синт не уходит. Startup Sync = N/A до стартового запроса патча. |
| Mixer / Element | ELVL Level (`03 EE 00 00`, `ELEMENTnVOLUME`, `sliderMixerEl1..4Level`) | PASS | PASS | N/A | PASS | CONFIRMED WORKING 2026-05-19. Mixer Level row; dedicated transport per element. UI 0..127. |
| Mixer / Element | ELDT Detune (`03 EE 00 01`, `ELEMENTnFINE`, `Pitch::sliderFine`) | PASS | PASS | N/A | PASS | CONFIRMED WORKING 2026-05-19. Mixer ↔ Pitch synchronized; reparent transport; no new `valueSysexIn` subscriber. UI −7..+7. |
| Mixer / Element | ELNS Note Shift (`03 EE 00 02`, `ELEMENTnPITCH`, `Pitch::sliderPitch`) | PASS | PASS | N/A | PASS | CONFIRMED WORKING 2026-05-19. UI −64..+63; VV 0..127 (`VV=UI+64`); signed encode in `MidiObjects.h`; Mixer ↔ Pitch synchronized. |
| Mixer / Element | ENLL Note Limit Low (`03 EE 00 03`, `ELEMENTnNOTELIMITLOW`, `sliderMixerEl1..4NoteLimitLow`) | PENDING | PENDING | N/A | PASS | IMPLEMENTED 2026-05-20. UI note names `getMidiNoteName(_,true,true,3)`; same MidiSlider/valueSysexIn path as ELVL. Live SY99 test pending. |
| Mixer / Element | ENLH Note Limit High (`03 EE 00 04`, `ELEMENTnNOTELIMITHIGH`, `sliderMixerEl1..4NoteLimitHigh`) | PENDING | PENDING | N/A | PASS | IMPLEMENTED 2026-05-20. UI note names `getMidiNoteName(_,true,true,3)`; TEMP UI default 127 until patch restore; live SY99 test pending. |
| Mixer / Element | EVLL Velocity Limit Low (`03 EE 00 05`, `ELEMENTnVELOCITYLIMITLOW`, `sliderMixerEl1..4VelocityLimitLow`) | PENDING | PENDING | N/A | PASS | IMPLEMENTED 2026-05-20. UI 0..127; same MidiSlider/valueSysexIn path as ELVL. Live SY99 test pending. |
| Mixer / Element | EVLH Velocity Limit High (`03 EE 00 06`, `ELEMENTnVELOCITYLIMITHIGH`, `sliderMixerEl1..4VelocityLimitHigh`) | PENDING | PENDING | N/A | PASS | IMPLEMENTED 2026-05-20. UI 0..127; same path as EVLL. Live SY99 test pending. |
| Mixer / Element | EFSDLV Effect Send Level | TODO | TODO | N/A | **FAIL** | **Live RX (отдельный путь):** ANONIM 127/127, Classic 127/127, NiteHwk 127/100. **Library review 2026-05-25 (post-#30 capture vs LCD):** A6 **FAIL** (dump 127/100, LCD **0/0**); A7 **FAIL** (dump 127/127, LCD **0/0**). Review: A6 `f25ec3dc…`, A7 `04553df…`. JUCE editor sliders vs LCD — **TODO**. |
| App lifecycle | Стартовая синхронизация UI ← состояние синта | N/A | N/A | TODO | TODO | Implemented: Config «Startup sync on MIDI connect» (default OFF) sends edit-buffer dump request once when In+Out open. Hardware verify pending — see `SYM7_library_sync_progress.md`. |
| Editor UX | Exit gate (dirty patch) | N/A | N/A | N/A | **PASS** | 2026-05-23 HW: первый голос без диалога; save/gate после правок — OK. **Fix 2026-05-26:** правки с панели SY99 → dirty + exit gate при PC с синта; ingest param9 в live baseline diff. |
| Diagnostics | Исходящие / трассировка (Debug окно, DBG, FileLogger рядом с exe) | N/A | N/A | N/A | TODO | `Sysex77_OUT.log` — служебный лог. **SysEx-only** TX/RX — **`Sysex77_MIDI.log`** рядом с exe (без CC/PC/clock). Путь при старте: `[MidiStream] SysEx-only log: …`. |
| Library / Web | L1 manifest + L2 parse ladder (`/library/sy99/internal`) | N/A | N/A | N/A | **PARTIAL** | Sprint **3** 2026-05-25: Library API `parsed0040` = resolve + companion 0040 (R-NEW-7). API smoke A6/A7 PASS. HW review FAIL (LCD/OUTSEL) — Sprint 5. |
| Library / BankClick | Voice slot ingest → editor apply (offline `.syx`) | N/A | TODO | N/A | **PARTIAL** | **2026-05-25 HW:** captures split (`AUTOSYNC-VC-INT.syx` + `AUTOSYNC-0040VC-INT.syx`). A1 ingest **частичный FAIL**. A6/A7 Library review **FAIL** (EFSDLV + OUTSEL E1); editor vs LCD — TODO. |

### Test procedure — EFSDLV / BankClick 0040VC

1. ~~RX format~~ **PASS** — ANONIM / Classic / NiteHwk 115 B (fixtures `05`, `08`, `07`).
2. ~~Rebuild Sysex77 Debug x64~~ **PASS** — 2026-05-25.
3. ~~Auto-sync #30~~ **PASS** (ранее) — captures на диске; формат **split**: `AUTOSYNC-VC-INT.syx` (8101) + `AUTOSYNC-0040VC-INT.syx` (0040), не interleaved в одном файле.
4. ~~Library review **A6 Classic**~~ **FAIL** 2026-05-25 — review `f25ec3dc79a640be9e15922d409320d7`: dump **127/100**, LCD **0/0** (fixture 03 ожид. 127/127).
5. ~~Library review **A7 NiteHwk**~~ **FAIL** 2026-05-25 — review `04553dfdd12b41448a942f01e170235b`: dump **127/127**, LCD **0/0** (HW notes: live path 127/100).
6. Offline bundle — **PASS** 2026-05-25 (см. таблицу ниже). Real-file paired check: **FAIL** (split layout).
7. ~~Зафиксировать PASS/FAIL в EFSDLV row~~ **FAIL** A6/A7 (Library review); JUCE editor vs LCD — **TODO**.

---

## Sprint 2 — Phase 2a offline gate

**Дата прогона:** 2026-05-25. **Gate Sprint 3 (offline): PASS.**

| Script | Date | Result |
|--------|------|--------|
| `_validate_bulk_parse.py` | 2026-05-25 | **PASS** |
| `_validate_0040_bulk.py` | 2026-05-25 | **PASS** |
| `_validate_efmode_bulk.py` | 2026-05-25 | **PASS** |
| `_validate_efsdlv_elmode4.py` | 2026-05-25 | **PASS** |
| `_validate_library_bindings.py` | 2026-05-25 | **PASS** |
| `_validate_paired_autosync.py` | 2026-05-25 | **PASS** (synthetic only) |
| `_test_library_navigation.py` | 2026-05-25 | **PASS** (20 tests) |

**Recommended (не в bundle):** `_verify_registry_path.py` — **PASS** 2026-05-25.

**Optional real capture:**

| Check | Date | Result | Notes |
|-------|------|--------|-------|
| `_validate_paired_autosync.py` на `AUTOSYNC-VC-INT.syx` | 2026-05-25 | **FAIL** | 8101-only в VC file; 0040 в companion `AUTOSYNC-0040VC-INT.syx` |
| `_validate_manifest_offsets.py` | 2026-05-25 | **PASS** | 64 slots, mm=10 EP\|GrnDual OK, idx invalid=0/64 |

---

## Sprint 2 — Phase 2b HW smoke matrix (BankClick → editor vs SY99 LCD)

**Сессия:** 2026-05-25, Debug x64, MIDI-OX + `Sysex77_MIDI.log` / `Sysex77_OUT.log`.  
**Правило:** сравнение **SY99 LCD** ↔ **Sysex77 Common** после клика слота в Librairie. Fixture — справка, не gate.

### A1 ANONIM (mm=`00`) — **FAIL overall**

**SY99 LCD (authoritative):** elmode **2 AWM poly**; ELVL El.1=**52**, El.2=**127**; WPBR=**1**; OUTSEL El.1=**Group 1 only**; El.2=Group 1/2.

| Param | SY99 | Sysex77 editor | Status | Notes |
|-------|------|----------------|--------|-------|
| ELVL El.1 | 52 | 52 | **PASS** | |
| ELVL El.2 | 127 | 0 | **FAIL** | второй элемент не apply |
| WPBR | 1 | 2 | **FAIL** | 0040 tail / companion ingest |
| OUTSEL El.1 | Group 1 (raw **2**) | G1/G2 **(6)** | **FAIL** | LCD: только G1 горит |
| OUTSEL El.2 | Group 1/2 (raw **6**) | G1/G2 **(6)** | **PASS** | |

**Known issue:** патч A1 на синте ≠ fixture `01_init_anonim` (elmode/volumes). Smoke = согласованность editor↔LCD, не fixture.

### A6 EP:Classic (mm=`05`) — **FAIL overall** (Library review)

**Review:** `f25ec3dc79a640be9e15922d409320d7` — `http://localhost:5173/library/sy99/internal/5?review=f25ec3dc79a640be9e15922d409320d7`  
**Источник:** Library parse `AUTOSYNC-VC-INT.syx` ↔ **SY99 LCD** (authoritative). Fixture `03` — справка.

| Param | Library (dump) | SY99 LCD | Sysex77 editor | Status | Notes |
|-------|----------------|----------|----------------|--------|-------|
| EFSDLV E1 | **127** | **127** | **127** | **PASS** | JUCE OK; web fix: catalog fallback → 0040 @100 |
| EFSDLV E2 | **127** | **127** | **127** | **PASS** | JUCE OK; web был 0 (binding maxEl=0, без 0040 fallback) |
| OUTSEL E1 | **2** (G1) | **6** (G1/G2) | TODO | **FAIL** | тот же паттерн что A1 |
| OUTSEL E2 | **6** | **6** | TODO | **PASS** | confirmed_ok в review |

### A7 EP:NiteHwk (mm=`06`) — **FAIL overall** (Library review)

**Review:** `04553dfdd12b41448a942f01e170235b` — `http://localhost:5173/library/sy99/internal/6?review=04553dfdd12b41448a942f01e170235b`  
**Источник:** Library parse `AUTOSYNC-VC-INT.syx` ↔ **SY99 LCD** (authoritative).

| Param | Library (dump) | SY99 LCD | Sysex77 editor | Status | Notes |
|-------|----------------|----------|----------------|--------|-------|
| ELMODE | **4** | **4** | TODO | **PASS** | confirmed_ok |
| EFSDLV E1 | **127** | **0** | TODO | **FAIL** | capture ≠ LCD |
| EFSDLV E2 | **127** | **0** | TODO | **FAIL** | live RX path: 127/100; dump E2=127 не 100 |
| OUTSEL E1 | **2** (G1) | **6** (G1/G2) | TODO | **FAIL** | reconcile path; elmode 4 |
| OUTSEL E2 | **6** | **6** | TODO | **PASS** | confirmed_ok |

### Сводная таблица (smoke matrix)

| Param | Слот | Голос | mm | Status |
|-------|------|-------|-----|--------|
| ELVL | A1 | ANONIM | `00` | **PARTIAL** (E1 PASS, E2 FAIL) |
| WPBR | A1 | ANONIM | `00` | **FAIL** |
| OUTSEL E1 | A1 | ANONIM | `00` | **FAIL** |
| OUTSEL E2 | A1 | ANONIM | `00` | **PASS** |
| EFSDLV | A6 | EP:Classic | `05` | **PASS** | JUCE 127/127; web fix populateRegistryDumpFields 0040 fallback |
| OUTSEL E1 | A6 | EP:Classic | `05` | **FAIL** | review; dump 2 vs LCD 6 |
| OUTSEL E2 | A6 | EP:Classic | `05` | **PASS** | review confirmed_ok |
| ELMODE | A7 | EP:NiteHwk | `06` | **PASS** | review confirmed_ok |
| EFSDLV | A7 | EP:NiteHwk | `06` | **FAIL** | review `04553df…`; dump 127/127 vs LCD 0/0 |
| OUTSEL E1 | A7 | EP:NiteHwk | `06` | **FAIL** | review; dump 2 vs LCD 6 |
| OUTSEL E2 | A7 | EP:NiteHwk | `06` | **PASS** | review confirmed_ok |

**Phase 2b gate (Sprint 5):** **не PASS** — A1 FAIL; A6/A7 Library review FAIL (EFSDLV + OUTSEL E1); OUTSEL E1 **2 vs 6** — системный паттерн на A1/A6/A7.

---

## Sprint 3 — Library API 0040 resolve (R-NEW-7)

**Дата:** 2026-05-25. **Gate offline: PASS.** **Gate API smoke: PASS.**

**Изменение:** [`Sy99LibraryApi.cpp`](Sysex77-master/Source/Sy99LibraryApi.cpp) — `parsed0040` через `sy99Resolve0040FrameForVoiceSlot` (companion `AUTOSYNC-0040VC-INT.syx`), вместо inline `findPaired0040FrameAfter` только в VC-файле.

| Check | Result | Notes |
|-------|--------|-------|
| `_validate_bulk_parse.py` | **PASS** | |
| `_validate_0040_bulk.py` | **PASS** | |
| `_validate_efsdlv_elmode4.py` | **PASS** | |
| `_validate_library_bindings.py` | **PASS** | |
| `_verify_registry_path.py` | **PASS** | |
| `GET /api/library/pages/internal/voices/5` (A6) | **PASS** | WPBR/SPTPNT/EFSDLV `inDump=true`; EFSDLV raw0040 **127/100** |
| `GET /api/library/pages/internal/voices/6` (A7) | **PASS** | EFSDLV raw0040 **127/127** |
| voiceIndex vs mm (manifest) | **PASS** | mm=5→vi=5, mm=6→vi=6; `sy99CopyManifestToVoiceIndex` **не нужен** |
| Rebuild link | **BLOCKED** | `Sysex77.exe` запущен (LNK1168); `.cpp` скомпилирован — перезапуск exe для нового binary |

**Не в scope Sprint 3 (остаётся Sprint 5):** EFSDLV LCD **0/0** vs dump; OUTSEL E1 dump **2** vs LCD **6**; A1 ELVL E2 / WPBR BankClick apply.

**Следующий шаг:** Sprint 5 (BankClick ingest-only) — после HW close Sprint 4 scenario 4.

---

## Sprint 4 — HW navigation verify (scenarios 1–4)

**Дата offline gate:** 2026-05-25. **Gate offline: PASS.** **Gate HW 4/4: PARTIAL** (2/3/4 — см. ниже).

**Offline:**
```bash
python _agent_context/fixtures/_run_sprint4_gate.py
# или: python _agent_context/fixtures/_test_library_navigation.py  (20 tests)
```

**Расширение Sprint 4:** `_test_library_navigation.py` — зеркало `sy99HostSynthNavInSync` / `libraryOutboundNeedsFullTriple` (C++ path, не legacy ctx-only).

### HW matrix (scenarios 1–4)

| # | Сценарий | Offline | HW Status | Notes |
|---|----------|---------|-----------|-------|
| **1** | Librairie click → ingest → editor | N/A | **PARTIAL** | Nav/ingest path OK: `[BankClick] parsed8101`, companion 0040 из split capture. **Param vs LCD FAIL** (A1/A6/A7) — Sprint 5, не nav. |
| **2** | Outbound recall (tab/slot/◀▶) | **PASS** (20 tests) | **PASS** | HW 2026-05-23: A16→B1, PRE1 D4, PC-only in-sync ([`SYM7_library_sync_progress.md`](_agent_context/SYM7_library_sync_progress.md)). Re-verify: MIDI-OX CC0/CC32/PC. |
| **3** | Inbound PC → Librairie highlight | **PASS** (mm decode) | **PASS** | HW 2026-05-23: wrap A16→B1, no echo loop; echo guard 300 ms. Log: `[NAV audit] RX suppressed=0/1`. |
| **4** | Synth panel nav → abort 0040 | N/A | **TODO** | Procedure: Invoke #14 или pending `fetch0040 TX` → slot на SY99 до RX. Expect: `[BankClick] fetch0040 aborted: synth navigation`. R-KEEP-8: nav не блокируется. |

### HW procedure (когда SY99 подключён)

1. **Scenario 3** — panel A1→A16→B1; highlight без ping-pong PC.
2. **Scenario 2** — Librairie D1→D2 (PC-only); A16→B1; tab PRE1 + slot; MIDI-OX TX log.
3. **Scenario 4** — #14 Invoke A6 → press SY99 slot mid-fetch; abort log.
4. **Scenario 1** — click A1/A6/A7; `[BankClick]` logs; param accuracy отдельно (Sprint 5).

**Не в scope Sprint 4:** C++ changes; lazy 0040 wiring (Sprint 5); param ingest fixes.

**Sprint 4 HW gate close:** scenarios **2+3+4 PASS** + scenario **1 nav PASS** (param may stay PARTIAL until Sprint 5).

---

## Sprint 2 — HW smoke matrix (legacy table — см. Phase 2b выше)

| Param | Слот | Голос | mm | Fixture | Status |
|-------|------|-------|-----|---------|--------|
| ELVL | A1 | ANONIM | `00` | `01_init_anonim_07x1_voice.syx` | **PARTIAL** |
| WPBR | A1 | ANONIM | `00` | same | **FAIL** |
| OUTSEL E1 | A1 | ANONIM | `00` | same | **FAIL** |
| OUTSEL E2 | A1 | ANONIM | `00` | same | **PASS** |
| OUTSEL + EFSDLV | **A7** | **EP:NiteHwk** | **`06`** | `baseline_ep_nitehwk_voice.syx` | **TODO** |

**Fixture bundle:** см. Phase 2a — все **PASS** 2026-05-25.

---

## HW scenarios checklist (Sprint 4 — см. полную секцию выше)

См. [`07_architecture_index.md`](_agent_context/07_architecture_index.md) §4. Сценарии **5–8** — вне Sprint 4 gate.

| # | Сценарий | Status | Notes |
|---|----------|--------|-------|
| 1 | Клик Librairie → ingest → editor | **PARTIAL** | Nav OK; param vs LCD FAIL — Sprint 5 |
| 2 | Outbound recall (tab/slot) | **PASS** | Offline + HW 2026-05-23 |
| 3 | Inbound PC suppress | **PASS** | Offline + HW 2026-05-23 |
| 4 | Synth panel nav (abort 0040) | **TODO** | Procedure in Sprint 4 section |
| 5 | Full library sync #30–#32 | **PASS** | captures 2026-05-25 ~01:40 (split VC + 0040VC) |
| 6 | Live voice read (reset state) | TODO | |
| 7 | Manual dump RX FROM SY99 | TODO | |
| 8 | Invoke load #14/#15 (probe) | **PARTIAL** | `[InvokeLoad] TX` в логе 2026-05-25 (mm=06) |

---

## Sprint 5 — BankClick HW refresh (2026-05-23)

### 5.1 Router + mutex — **DONE (offline gate)**

**C++:** `sy99ScheduleBankClickHwRefresh` в [`Sy99Lazy0040Fetch.h`](Sysex77-master/Source/Sy99Lazy0040Fetch.h); вызов из [`Sy99BankClickSlot.h`](Sysex77-master/Source/Sy99BankClickSlot.h) после PC recall.

| Правило | Поведение |
|---------|-----------|
| PRE без companion 0040 | `[BankClick] hwRefresh slot` → slot 0040VC mm=N |
| Full capture (companion есть) | `[BankClick] hwRefresh edit-buffer` → mt=7F mm=00 |
| Invoke active | fetch/hwRefresh skip; invoke blocked при active capture |
| Slot reopen | `[BankClick] fetch0040 aborted: slot reopen` |
| Defer | `requestSysex` / sync / MIDI closed → drain как раньше |

**Offline gate:** `_validate_bulk_parse.py`, `_validate_efsdlv_elmode4.py`, `_run_sprint4_gate.py` — **PASS**.

**HW gate 5.1 (manual):** PRE mm=9 → `hwRefresh slot`; A6 click → `hwRefresh edit-buffer`; invoke+#14 не параллельно click.

**Sprint 5.2 — A6 Classic EFSDLV web (2026-05-25):**

| Check | Result | Notes |
|-------|--------|-------|
| JUCE editor A6 EFSDLV El.1/2 | **PASS** | LCD **127/127** (ground truth) |
| API `EFSDLV` el1 raw0040 | **PASS** | 127 |
| API `EFSDLV` el2 raw0040 | **FAIL→FIX** | был **0** — binding `8101\|0040` не матчил `0040`; fix `Sy99LibraryBulkRead` + registry fallback |
| Vite review `f25ec3dc…` | **STALE** | пометки 0/0 — пересоздать review после перезапуска exe |

**Rebuild:** закрыть `Sysex77.exe` → MSBuild → перезапуск → hard refresh Vite `:5173`.

**Не в scope 5.2:** OUTSEL E1 dump 2 vs LCD 6; EFLN1 el1 live-only.

---

## 2026-05-26 — EFLN golden matrix (P0) + Send 3 OFF + Nav recall

**Сессия:** Debug x64, `Sysex77_OUT.log` / `Sysex77_MIDI.log` (12:34–12:36), prior `Sysex77_MIDI.last.log` (12:13–12:16).

### P0 golden matrix (Send 1+3 → ui=5) — **PASS**

| Slot | Panel | Voice | EFLN 8101 | EFLN 0040 | UI (0040) |
|------|-------|-------|-----------|-----------|-----------|
| 5 | A6 | EP:Classic | 5 | 1 (Send 3 OFF) | 1 |
| 7 | A8 | EP:Belrose | 5 | 5 | 5 |
| 28 | B13 | SP:Crystal | 5 | 5 | 5 |
| 40 | C9 | BR:Splatz | 5 | 5 | 5 |

Offline: `_validate_bulk_parse.py` **PASS**; `_probe_efln_slots.py` on post-sync captures.

### EFLN reconcile removed (2026-05-26)

- Удалён `[EFSEND reconcile] El.2 EFLN <- El.1 bulk8101` из `resolveParam`.
- Удалён `efsendEl2SharesEl1Bulk8101` (El.2 EFSDVSNS/EFSDSCL больше не указывает на bulk8101 El.1).
- Правило: [`.cursor/rules/sy99-no-value-mirror.mdc`](.cursor/rules/sy99-no-value-mirror.mdc).

### Send 3 OFF — **0040 authoritative for UI**

| Source | Slot 5 Classic (Send 3 OFF) | Golden slots 7/28/40 |
|--------|----------------------------|----------------------|
| 8101 bank @ elvl+35 | 0x05 (Send 1+3) | 0x05 |
| 0040VC @99/@103 | 0x0B → **ui=1** | 0x0F → **ui=5** |

**Fix:** EFLN El.1/El.2 из **0040VC @99/@103** (elmode 4). Без mirror El.2←El.1.

**Fix (2026-05-26b):** `resolveParam` для EFLN не использовал 0040 — `bulk0040ForResolve()` отсекал raw из‑за `confirmedBulk0040=false` в meta; теперь trusted 0040 EFLN читается напрямую (как EFSDLV special path).

**Known gap:** EFSDVSNS/EFSDSCL El.2 — live param9 only до HW offset test.

**Decisive test (Store):** если после Store+Sync 8101 @+35=**0x01** — bank обновился; иначе UI=0040, bank=8101.

### Nav recall fix (code 2026-05-26)

| Issue | Fix |
|-------|-----|
| Recall deferred (MIDI Out not open) | `flushPendingLibraryRecall`: page bank select + `libraryForceNextRecallFullTriple` |
| After Full Sync recall `fullTriple=0` | `finishFullLibrarySyncOnUi`: force full triple before `sy99RestoreLibrarySlotFromPersistence` |

Files: `Sy99LibraryNavigation.h`, `MidiDemo.h`, `Librairie.h`.

---

## Next candidates to test

1. **A7 NiteHwk** — EFSDLV **127/100** LCD vs editor (без Invoke #14).
2. **A6 Classic** — EFSDLV **127/127** LCD vs editor.
3. A1 known issues — elmode 2 AWM poly, companion 0040 ingest (WPBR/ELVL E2/OUTSEL E1).

---

См. также: **`_agent_context/07_architecture_index.md`** (rules, Decision Tree, smoke matrix), **`_agent_context/05_missing_audit.md`** (карта адресов и реализации в коде).

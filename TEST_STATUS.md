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
| App lifecycle | Стартовая синхронизация UI ← состояние синта | N/A | N/A | TODO | TODO | Implemented: Config «Startup sync on MIDI connect» (default OFF) sends edit-buffer dump request once when In+Out open. Hardware verify pending — see `SYM7_library_sync_progress.md`. |
| Diagnostics | Исходящие / трассировка (Debug окно, DBG, FileLogger рядом с exe) | N/A | N/A | N/A | TODO | `Sysex77_OUT.log` — служебный лог. **SysEx-only** TX/RX — **`Sysex77_MIDI.log`** рядом с exe (без CC/PC/clock). Путь при старте: `[MidiStream] SysEx-only log: …`. |

### Test procedure for next item

Следующий параметр здесь пока не задан.

---

## Next candidates to test

На данный момент список следующих кандидатов не заполнен; новые строки добавляются по мере следующего регресса.

---

См. также: **`_agent_context/05_missing_audit.md`** (карта адресов и реализации в коде).

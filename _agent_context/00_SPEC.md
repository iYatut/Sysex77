# SY99 Editor — Agent Specification

**Читай первым:** [`07_architecture_index.md`](07_architecture_index.md) — правила R-NEW/R-KEEP, Decision Tree, data flow, smoke matrix.  
Затем этот файл, **`02_sysex_format.md`**, **`01_code_map.md`**, для bindings — **`fixtures/sy99_param_bindings.json`**, для bulk — **`sy99_bulk_dump_request.md`**, для live-адресов — **`09_confirmed_addresses.md`** (не `MIDI_MAP_OBSERVATIONS.md`).

## Project Goal

Build a complete **Yamaha SY99** patch editor with realtime **parameter-change SysEx** control. The running codebase is **Sysex77** (JUCE desktop app under `Sysex77-master/`): extend it toward covering all parameters described in the SY99 MIDI documentation.

**Scope note:** `_agent_context/03_parameter_map.csv` — **audit tracker** (~183 строк), не runtime registry. **37 confirmed params** — [`fixtures/sy99_param_bindings.json`](fixtures/sy99_param_bindings.json) + `Sy99ParamRegistry.cpp` `kMetaTable`. Полное покрытие SY99 — расширение из **`sy99_sysex_complete.md`**.

## Два UI (K1 — зафиксировано Sprint 1b)

| UI | Путь | Роль |
|----|------|------|
| **JUCE Sysex77** | `Sysex77-master/` | **Продукт:** editor, Librairie, MIDI, LiveSynthState |
| **React debug mirror** | `ui/` + Param API `:8765` | Catalog, Library review, bindings visibility — **не** замена editor |

React намеренно существует для отладки bindings и review workflow. См. [`ui/README.md`](../ui/README.md).

## Tech Stack

- **Framework:** [JUCE](https://juce.com/) (modules per `Sysex77-master/JuceLibraryCode/`, `AppConfig.h`)
- **Language:** C++ (headers under `Sysex77-master/Source/*.h`, entry `Main.cpp`)
- **Build tool:** **Visual Studio** solutions generated for JUCE — `Sysex77-master/Builds/VisualStudio2022/Sysex77.sln` (also `VisualStudio2026/Sysex77.sln`)
- **Build:** open the `.sln` in Visual Studio and build the **App** target, or from *Developer Command Prompt for VS* (example):

  ```text
  msbuild "Sysex77-master\Builds\VisualStudio2022\Sysex77.sln" /p:Configuration=Release /p:Platform=x64
  ```

  *(Exact output path follows the JUCE exporter: typically under `Builds/VisualStudio2022/x64/Release/App/`.)*

- **Run:** launch the built **Sysex77** application executable from the build output folder (no committed `.exe` in tree at last audit).

- **MIDI lib:** **JUCE `juce_audio_devices`** / MIDI (`MidiInput`, `MidiOutput`, `MidiMessage::createSysExMessage`) — see `01_code_map.md` MIDI section.

- **Debug UI (optional):** **React + Vite** in `ui/` — `npm run dev` (proxy → Param API `:8765`). **Не** основной editor; см. [`07_architecture_index.md`](07_architecture_index.md).

- **Regression gate:** Python fixtures `_agent_context/fixtures/_validate_*.py` — **обязательны** перед merge изменений parse/registry (R-NEW-2).

**Note:** JUCE build — без npm для App target. npm только для `ui/` debug mirror.

## Key Files

High-signal files from `01_code_map.md` (8–12):

1. `Sysex77-master/Source/MidiDemo.h` — MIDI I/O, tabs, monitor queue, `handleIncomingMidiMessage` / `handleAsyncUpdate`, `valueSysexIn`
2. `Sysex77-master/Source/MidiSysex.h` — OSC ↔ SysEx bridge, `sendSysex`, `sendRaw`, voice/pan OSC routes
3. `Sysex77-master/Source/MidiObjects.h` — `MidiSlider` / `MidiButton` / `MidiCombo`, templates, inbound matching
4. `Sysex77-master/Source/Voice.h` — Voice page: mode combo, master volume, name editor
5. `Sysex77-master/Source/Pan.h` — Pan EG sliders + SysEx addresses (`0x0A` block)
6. `Sysex77-master/Source/Hook.h` — EG hooks; `Segment::sendOsc` (paired rate/level SysEx)
7. `Sysex77-master/Source/DeviceModel.h` — SY77/TG77/SY99 selection, SysEx device ID
8. `Sysex77-master/Source/Librairie.h` — bank send/receive dump plumbing (`sendRaw`, `arraySysex`)
9. `Sysex77-master/Source/AWMVue.h` — AWM UI + wave table SysEx
10. `Sysex77-master/Source/Main.cpp` — application entry

## SysEx Format

```text
F0 43 1n 34 [gg hh ll pp] [0V vv] F7
```

In this project the common case is a **9-byte** payload to `MidiMessage::createSysExMessage`: **`43`, device, `34`, then bytes `[3]…[8]`** (address in `[3]…[6]`, value typically `[7]`/`[8]`). Mapping to `(gg,hh,ll,pp)` is documented in **`_agent_context/02_sysex_format.md`**.

## Current Status

Derived from **`03_parameter_map.csv`** and **`05_missing_audit.md`** (snapshot; recount if those files change).

| Metric | Value |
|--------|-------|
| **Total rows in `03_parameter_map.csv` (excl. header)** | **183** (расширение групп `03`/`05`/`07`/`08`, см. Prompt #11) |
| **Tracked SY99-ish rows excluding meta `ZZ00`** | **182** |
| **Manual Voice Common params in CSV (`source=manual`, excl. `ZZ00`)** | **34** |
| **Implemented (send): rows in audit with ✅ in send column** | **50** (+ AWM Amp EG remap в `WaveEg.h` — Prompt #12) |
| **Implemented (receive): rows with full ✅ send + ✅ receive** | **49** (+ Amp EG блок кроме частичных ⚠️) |
| **Partial receive (audit)** | **1** (`WaveEg.h`: `sliderSlope`/PARS ⚠️ приём; Hook drag всё ещё шлёт legacy шаблоны) |
| **Missing UI (Voice Common CSV rows with ❌ send)** | **0** — все 22 параметра `02 00 00 28..43` имеют UI (commonPanel в Voice.h) |
| **Outstanding TODOs** | Prompt #12: библиотека (Import/Export/RequestVoice). **LIBSYNC** (патчи `sysex77_library_patch_sync_increment.patch` ≡ `sysex77_full_with_library_sync.patch`): уже в коде — см. `05_missing_audit` § Library ↔ Voice sync. Amp EG remap — основные rate/level; TODO — Hook→канон, L0/L1/L4/RL1/RL2, bulk→полный UI. |
| **Realtime safety** | Throttle 30 msg/s + debounce (`ThrottleFlushTimer`) + echo guard 50 мс — `MidiSysex.h`/`MidiDemo.h`. |
| **Library sync [LIBSYNC]** | Парсинг кадров `F0…F7`, `/77SendVoice`, зеркало имени в Voice — см. код + `05_missing_audit.md`; полный bulk→параметрический UI — нет. |

Interpretation: Pan EG and related controls are implemented in code but **not** listed as separate rows in `03_parameter_map.csv`; the audit adds them explicitly.

## Non-Goals (do NOT implement in this session)

- Parity feature-complete **TG77/SY77** SysEx maps (different products; `DeviceModel` mentions them but focus here is **SY99**)
- Production-grade **bulk dump/restore** editor UX (library: per-slot SEND + имя синхронизированы — полный bulk→виджеты всё ещё отдельная задача)
- **MIDI file** playback / sequencing
- **Audio preview** / internal synth (editor + hardware SY99 only)

## Constraints

- Do **not** refactor existing working code unless needed for the requested change; prefer minimal patches (`user_rules`).
- Do **not** add new **npm** packages without approval — **N/A** today (no Node project); same intent for new heavy dependencies in JUCE/CMake if introduced later.
- Prefer **parameter-change SysEx** during edits; avoid spamming **full bulk** dumps for routine slider moves (library `sendRaw` is for intentional bank transfers).
- **Throttling:** target **≤ 30 parameter messages/sec** during continuous drag (encoder scrub) — enforced in `MidiSysex.h::sendSysex` (33 ms интервал, `lastSysexSendTime`). Финальное значение слайдера никогда не теряется: подавленный SysEx сохраняется в `pendingSysex[9]`, `ThrottleFlushTimer` (35 мс one-shot) дотягивает его до синта, если новое значение не пришло. Прямые вызовы `sendToOutputs` (bulk-кнопка, VNAM-пакет) пропускают throttle намеренно.
- **Echo guard:** ignore reflected **own** SysEx for **~50 ms** after send — реализовано в `MidiDemo.h::handleAsyncUpdate` через кольцевой буфер `echoGuard[16]` и `isRecentEcho()`; обновление `valueSysexIn` блокируется, если адрес `[3..6]` совпал с недавним отправленным. `flushPendingSysex()` тоже регистрирует адрес.
- **Do NOT read `MIDI_MAP_OBSERVATIONS.md` directly** when deriving addresses for code or CSV. Use **`_agent_context/09_confirmed_addresses.md`** — a pre-filtered extract containing only CONFIRMED / post-fix RECORDED sections. The raw file contains legacy addresses (`0x03`/`07`, `0x07`/`06`) and archive dumps that predate the MidiDemo buffer fix and will produce wrong mappings.
- **Fixture gate (R-NEW-2):** перед merge правок `YamahaLmVoiceDump`, `Sy99ParamRegistry`, offsets в bindings — прогнать bundle из [`07_architecture_index.md`](07_architecture_index.md) §10; FAIL = stop.
- **Inbound paths (K3):** 37 registry params → `LiveSynthState` + `applyLiveSynthStateToEditor`; `valueSysexIn` = legacy only (R-NEW-5). Unify = Sprint 8.

## Deliverables expected from agent

**Primary runtime registry (K2):** [`fixtures/sy99_param_bindings.json`](fixtures/sy99_param_bindings.json) — authoritative для 37 confirmed params. `03_parameter_map.csv` — audit/tracker, не заменяет bindings.

Map work to these equivalents:

1. **Parameter registry:** extend **`sy99_param_bindings.json`** (+ sync `kMetaTable` / `params_meta.json`), keep **`03_parameter_map.csv`** as audit trail.
2. **Send/receive routing (registry):** `LiveSynthState` + `Sy99ParamRegistry` + `applyLiveSynthStateToEditor`.
3. **Send/receive routing (legacy):** **`MidiSysex.h`** / **`MidiDemo.h`** / `valueSysexIn` + widget `setMidiSysex`.
4. **UI:** JUCE **`MidiSlider`/`MidiCombo`** in `Voice.h`, Common tab — product editor. React `ui/` — debug mirror only.
5. **Patch/state model:** **`LiveSynthState`** + **`ValueTrees.h`** / `Voice.h` ValueTree.
6. **Tests:** `_validate_*.py` fixtures (mandatory for parse changes); manual HW smoke per [`TEST_STATUS.md`](../TEST_STATUS.md).

When the user explicitly wants **React-only** UI changes, work in `ui/`; editor fixes stay in JUCE unless promoting bindings.

## Related agent context files

| File | Role |
|------|------|
| **`07_architecture_index.md`** | **Rules, Decision Tree, data flow, smoke matrix** — read first |
| `01_code_map.md` | File inventory + MIDI send/receive map |
| `02_sysex_format.md` | Byte layout vs Yamaha notation |
| `03_parameter_map.csv` | Audit tracker (~183 rows); **not** runtime registry |
| `fixtures/sy99_param_bindings.json` | **Authoritative** bindings (37 registry params) |
| `04_observed_logs.md` | Structured log placeholders (Pan EG) |
| `05_missing_audit.md` | CSV ↔ code coverage + Group A/B |
| `06_agent_prompts.md` | Step-by-step implementation prompts (C++/JUCE) |
| `09_confirmed_addresses.md` | **Clean extract** — confirmed SysEx addresses; use instead of `MIDI_MAP_OBSERVATIONS.md` |
| `sy99_sysex_complete.md` | Long-form SY99 SysEx notes (parameter change) |
| `sy99_bulk_dump_request.md` | Bulk Dump Request/Response (LM 0040VC/MU/SY/MS), Memory_type/#, auto-dump strategy |
| `library_binding_workflow.md` | 8-step bind cycle + PROMOTE |
| `SPRINT_1a_rules_audit.md` | Audit K1–K7 (archive; rules inline in 07) |
| `SYM7_library_sync_progress.md` | **Agent nav board** — SYM7 auto-sync research, capture log, gap matrix, parity backlog |
| `ui/README.md` | React debug UI — run, API proxy, Library routes |
| `SY99E2_reference.txt` | PDF / manual pointers |

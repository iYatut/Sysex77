# SYM7 Library Auto-Sync — Agent Progress Board

> **Цель:** зафиксировать MIDI-команды SY Manager (SYM7) для автоматической синхронизации библиотеки и довести Sysex77 до parity.  
> **Обновлено:** 2026-05-23  
> **План:** `.cursor/plans/sym7_library_sync_778685a8.plan.md` (не редактировать из агента)

---

## Quick start (read order)

1. **Этот файл** — status + backlog + handoff
2. [`sy99_bulk_dump_request.md`](sy99_bulk_dump_request.md) — официальный протокол bulk request/response
3. [`05_missing_audit.md`](05_missing_audit.md) § Library ↔ Voice sync [LIBSYNC]
4. Код: [`Sy99DumpRequest.h`](../Sysex77-master/Source/Sy99DumpRequest.h), [`Sy99BulkLibraryCapture.h`](../Sysex77-master/Source/Sy99BulkLibraryCapture.h), [`Librairie.h`](../Sysex77-master/Source/Librairie.h), [`LiveSynthState.h`](../Sysex77-master/Source/LiveSynthState.h)
5. Capture protocol: [`fixtures/sym7_captures/README.md`](fixtures/sym7_captures/README.md)
6. Validation: [`fixtures/_validate_dump_request.py`](fixtures/_validate_dump_request.py)

**Не путать:** parameter-change `F0 43 1n 34 …` — live edit, не library bulk sync.

---

## Status dashboard

| Phase | Description | Status | Evidence | Blocker |
|-------|-------------|--------|----------|---------|
| 0 | Baseline spec + Sysex77 inventory | **done** | § Command catalog (baseline), § Gap matrix | — |
| 1 | SYM7 hardware capture | **done** (monitor paste) | `01_startup_full_20260523_parsed.md` | RX responses not in paste — capture MOTU In separately |
| 2 | Spec update from capture | **partial** | `sy99_bulk_dump_request.md` §15–16 | Tail variant A/B не подтверждён SYM7 log |
| 3a | Dump request builders (VC/MU/SY/MS) | **done** | `Sy99DumpRequest.h` | — |
| 3b | Auto-sync 64 voices orchestrator | **done** | `Librairie.h` REQUEST VOICE menu | Hardware verify pending |
| 3c | Startup edit-buffer on MIDI connect | **done** | `MidiDemo.h`, Config `StartupSyncOnConnect` | Default OFF; hardware verify pending |
| 4 | Agent workflow | **done** | этот файл | — |

---

## Command catalog

### Baseline (manual / spec — verified headers)

| Trigger | Dir | Hex frame (abbrev) | mt | mm | Interval | Verified |
|---------|-----|-------------------|----|----|----------|----------|
| Voice bulk request (manual min) | TX | `F0 43 2n 7A 4C 4D 20 20 30 30 34 30 56 43 00 00 F7` | — | — | — | spec ✅ |
| Voice bulk request variant A | TX | `… 0040VC 00 00 [mt] [mm] [cs] F7` | varies | varies | — | SY77 extrapolation ⚠️ |
| Voice bulk response | RX | `F0 43 0n 7A … 0040VC … F7` | — | — | — | spec ✅ |
| Voice response (8101 block) | RX | `… 8101VC …` | — | — | — | fixtures ✅ |
| Multi request | TX | `… 0040MU 00 00 [tail?] F7` | 00 | 00..0F | — | spec ✅ |
| System request | TX | `… 0040SY 00 00 [tail?] F7` | — | — | — | spec ✅ |
| Effect request | TX | `… 0040MS 00 00 [tail?] F7` | — | — | — | spec ✅ |
| Device Inquiry | TX | `F0 7E 7F 06 01 F7` | — | — | — | spec ✅ |
| Library recall (slot click) | TX | CC0, CC32, PC ch1 | — | — | — | Sysex77 ✅ (context: page+CC32+mm/16) |
| Inbound PC from SY99 | RX | PC ch1 (+ batch CC0/CC32) | — | — | — | Sysex77 ⚠️ pending `_test_library_navigation.py` PASS + HW A16→B1 |

### Sysex77 programmatic (implemented)

| Trigger | Dir | Hex / behavior | mt | mm | Verified |
|---------|-----|----------------|----|----|----------|
| REQUEST VOICE → edit/a1/b1 test | TX | variant A, timeout → B | 7F/00/00 | 00/00/10 | fixtures partial |
| Sync from SY99 Start | TX | variant A | 7F | 00 | pending HW |
| Auto-sync 64 internal (menu) | TX | loop mm=0..3F, variant A→B retry | 00 | 0..63 | pending HW |
| Startup sync (Config ON + MIDI connect) | TX | edit buffer request | 7F | 00 | pending HW |

### SYM7 observed (fill after capture)

| Trigger | Dir | Hex frame | mt | mm | Interval ms | Verified |
|---------|-----|-----------|----|----|-------------|----------|
| Patch Viewer open | TX | _pending_ | — | — | — | ❌ |
| Download single edit | TX | _pending_ | 7F | 00 | — | ❌ |
| Download A1 | TX | _pending_ | 00 | 00 | — | ❌ |
| Download 64 voices | TX | _pending_ ×64 | 00 | 0..3F | _pending_ | ❌ |
| Download everything | TX | _pending_ | — | — | — | ❌ |

---

## SYM7 capture log

| Session | File | Scenario | Notes |
|---------|------|----------|-------|
| — | _(none yet)_ | — | См. [`fixtures/sym7_captures/README.md`](fixtures/sym7_captures/README.md) |

---

## Gap matrix: SYM7 vs Sysex77

| Capability | SYM7 (expected) | Sysex77 | Status |
|------------|-----------------|---------|--------|
| Single voice dump request | ✅ | ✅ REQUEST VOICE / Sync from SY99 | done |
| 64× programmatic voice requests | ✅ | ✅ Auto-sync 64 menu | done (HW verify pending) |
| Multi / System / Effect requests | ✅ | ✅ builders in `Sy99DumpRequest.h` | builders only |
| Download everything orchestration | ✅ | ✅ menu «Auto-sync full library» | done (HW verify pending) |
| Patch Viewer mirror on connect | ✅ | ❌ | needs SYM7 capture |
| Startup edit-buffer sync | ✅? | ✅ opt-in Config | done (default OFF) |
| Inbound PC → library slot | ✅ | ✅ | **partial** — `Sy99LibraryNavigation.h`; gate: `_test_library_navigation.py` + HW |
| Full bulk → UI sliders | ✅ | ❌ | out of scope |

---

## Implementation backlog

| ID | Task | File(s) | Priority | Status |
|----|------|---------|----------|--------|
| B1 | SYM7 capture sessions 1–6 | `fixtures/sym7_captures/` | P0 | blocked (user HW) |
| B2 | Confirm tail variant from SYM7 log | `sy99_bulk_dump_request.md` | P0 | blocked |
| B3 | Multi/SY/MS auto-sync menus | `Librairie.h` | P2 | todo |
| B4 | Download-everything orchestrator | `Sy99BulkLibraryCapture.h`, `Librairie.h` | P1 | **done** (HW verify pending) |
| B5 | Merge 64 AUTOSYNC files → one bank UX | `Librairie.h` | P3 | todo |
| B6 | `_validate_dump_request.py` AUTOSYNC cases | `fixtures/` | P1 | partial |
| B7 | Library nav regression gate | `fixtures/_test_library_navigation.py`, `Sy99LibraryNavigation.h` | P0 | **done** (HW verify pending) |

---

## Decision log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-05-23 | Default dump tail = **variant A** (mt/mm/cs); retry **B** on timeout | Matches existing `Librairie.h` / `LiveSynthState.h` |
| 2026-05-23 | 64 voices = **64 separate requests**, not one bank request | Official spec §4 |
| 2026-05-23 | Auto-sync saves **one combined** `AUTOSYNC-VOICE64-*.syx` | Practical parity with manual 05:64 capture |
| 2026-05-23 | Startup sync **default OFF** | Avoid surprise MIDI traffic; Config checkbox |
| _pending_ | SYM7 tail variant | Needs hardware capture |
| _pending_ | Inter-request delay ms | SYM7 timeout unknown; Sysex77 uses idle threshold 12s |
| 2026-05-23 | Library nav: canonical **(page, mm 0..63)**; UI **A1–D16** derived; recall context **page+CC32+mm/16** | SY99 panel = 16 buttons; PRE1/PRE2 = separate 64; `Sy99LibraryNavigation.h` |

---

## Validation results

| Check | Command | Result | Date |
|-------|---------|--------|------|
| edit/a1/b1 baselines vs REQTEST | `python _agent_context/fixtures/_validate_dump_request.py` | REQTEST captures missing (expected without HW session); AUTOSYNC SKIP | 2026-05-23 |
| Library nav regression | `python _agent_context/fixtures/_test_library_navigation.py` | **6/6 PASS** (A16→B1, A1→A16, PRE1 full triple) | 2026-05-23 |
| SYM7 TX log diff | manual | pending | — |

---

## Agent handoff notes

**Last session (2026-05-23):**
- Created this progress board + capture README
- Implemented `Sy99DumpRequest.h` MU/SY/MS builders + generic LM0040 builder
- Implemented auto-sync 64 voices in `Librairie.h` (REQUEST VOICE menu)
- Implemented optional startup edit-buffer sync (`Config.h` + `MidiDemo.h`)
- Updated `sy99_bulk_dump_request.md` §15–16, `00_SPEC.md`, `TEST_STATUS.md`

**Next agent should:**
1. Run SYM7 capture per README when hardware available
2. Fill § SYM7 observed in Command catalog
3. Run `_validate_dump_request.py` after AUTOSYNC hardware test
4. If SYM7 uses variant B only, switch default tail in orchestrator

---

## Agent workflow

1. Read **Status dashboard** → pick first non-done row
2. Work task → update Evidence + Decision log
3. Update **Agent handoff notes** at end of session
4. Do **not** edit the plan file in `.cursor/plans/`

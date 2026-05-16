# Agent Prompts — Пошаговые задачи для реализации SY99 редактора

> **Стек проекта: C++/JUCE. Нет JavaScript, TypeScript, npm.**  
> Рабочие файлы: `Sysex77-master/Source/*.h`, `_agent_context/*.csv`, `_agent_context/*.md`.  
> Выполнять промпты **строго по порядку**. Каждый промпт — самостоятельная задача.  
> Не переходить к следующему, пока предыдущий не завершён и не проверён.

---

## Промпт #1 — Аудит параметров (только чтение, без изменений кода)

```
Read _agent_context/00_SPEC.md first, then 01_code_map.md, 05_missing_audit.md,
03_parameter_map.csv, and sy99_sysex_complete.md.

Task:
1. Find any parameters in 03_parameter_map.csv that are NOT in 05_missing_audit.md.
   Add them to the audit table with status ❓ unknown.
2. Find any parameter addresses in sy99_sysex_complete.md that are NOT in
   03_parameter_map.csv. List them at the bottom of 05_missing_audit.md in a new
   section "Parameters missing from CSV" with status ❓ unknown.
3. Verify that Group A and Group B sections exist in 05_missing_audit.md.
   If they are missing, add them according to the criteria:
   - Group A: UI control exists, but send or receive is broken/missing.
   - Group B: No UI control and no SysEx send.

Do NOT modify any source code.
Output: updated 05_missing_audit.md only.
```

---

## Промпт #2 — Расширение таблицы параметров (03_parameter_map.csv)

```
Read _agent_context/00_SPEC.md, 02_sysex_format.md, sy99_sysex_complete.md,
03_parameter_map.csv, 09_confirmed_addresses.md.

IMPORTANT: This is a C++/JUCE project. There are no JS/TS files.
Do NOT create parameterMap.js or any JavaScript file.

IMPORTANT: Do NOT read Sysex77-master/Source/MIDI_MAP_OBSERVATIONS.md directly.
Use 09_confirmed_addresses.md as the pre-filtered extract of confirmed data.

Task: Extend _agent_context/03_parameter_map.csv with ALL parameter groups from
sy99_sysex_complete.md that are not yet in the CSV.

Required groups to add (if not already present):
- Pan EG (group 0A): use ONLY addresses from 09_confirmed_addresses.md (block 0x0A).
  Rates HT/R1–RR2 (b6=02–08), Levels L0–RL2 (b6=09–0F), SLP (b6=10),
  Pan Source (b6=00), Source Depth (b6=01). Use source=code (implemented in Pan.h).
- AFM Operator EG (group 0x56): R1 (b6=00), R2 (b6=01), HT (b6=0D).
  Use source=manual (not yet in Sysex77).
- AWM element EG (group 0x07): R2 E1 (b6=0x50), R2 E3 (b6=0x51) — confirmed.
  Other AWM params from sy99_sysex_complete.md if found with clear addresses.
  Use source=code if found in AWMVue.h / WaveEg.h.
- Voice Common extended (group 02): any NN values from sy99_sysex_complete.md
  not yet in the CSV.
- Effects params: only from sy99_sysex_complete.md with clear hex addresses.

Rules for each new row:
- group_id: short snake_case identifier.
- offset_hex: exactly 8 hex chars = gg hh ll pp (e.g. "0A000003").
- source=code if param is implemented in Sysex77-master/Source/*.h.
- source=manual if from sy99_sysex_complete.md but NOT yet in code.
- Do not duplicate existing rows.

Do NOT modify any source code.
Output: updated 03_parameter_map.csv only.
```

---

## Промпт #3 — Throttle и Echo Guard в MidiSysex.h / MidiDemo.h

```
Read _agent_context/00_SPEC.md (Constraints section), 01_code_map.md (MIDI send section),
09_confirmed_addresses.md.
Read Sysex77-master/Source/MidiSysex.h (full file).
Read Sysex77-master/Source/MidiDemo.h (sendToOutputs, handleIncomingMidiMessage,
handleAsyncUpdate, oscMessageReceived sections).

IMPORTANT: Do NOT read MIDI_MAP_OBSERVATIONS.md directly.
Address truth for Pan/AFM/AWM is in 09_confirmed_addresses.md.

Tech stack: C++/JUCE. MIDI send path:
  MidiSlider/MidiButton/MidiCombo → OSC /SYSEX →
  MidiDemo::oscMessageReceived (MidiSysex.h) → sendSysex → sendToOutputs.

Task: Add two protections to the outgoing SysEx path:

1. THROTTLE — limit outgoing parameter-change SysEx to max 30 messages/sec total.
   - Add a member variable: juce::uint32 lastSysexSendTime = 0;
   - In sendSysex (MidiSysex.h), before calling sendToOutputs: check
     juce::Time::getMillisecondCounter() - lastSysexSendTime < 33.
     If true, skip the send (acceptable for slider drags). Update lastSysexSendTime on send.
   - Mark the block with comment // THROTTLE: 30msg/sec limit

2. ECHO GUARD — after sending a SysEx, ignore reflected incoming messages for 50ms.
   - Add member: struct EchoEntry { uint8 addr[4]; juce::uint32 sentAt; };
     and a small array (e.g. EchoEntry echoGuard[16]) in MidiDemo.
   - In sendSysex: store sysexdata[3..6] + current timestamp into echoGuard (ring buffer).
   - In handleAsyncUpdate, before updating valueSysexIn: check if incoming data[3..6]
     matches any recent echoGuard entry with sentAt within last 50ms. If so, skip update.
   - Mark the block with comment // ECHO GUARD: ignore own reflections for 50ms

Constraints:
- Do NOT change any slider ranges, UI layout, or OSC routes.
- Do NOT break the existing send path.
- Use only JUCE API (no std::chrono, no new dependencies).

Output: modified MidiSysex.h and/or MidiDemo.h only.
```

---

## Промпт #4 — Исправление Group A: VNAM* отправка и ELMODE приём

```
Read _agent_context/00_SPEC.md, 05_missing_audit.md (Group A section).
Read Sysex77-master/Source/MidiSysex.h (oscSendMidiMessage branch, approx. lines 181–187).
Read Sysex77-master/Source/Voice.h (textEditorReturnKeyPressed ~line 336,
  comboMode / comboBoxChanged ~line 364, constructor for VoicePage).
Read Sysex77-master/Source/MidiObjects.h (MidiCombo class, valueChanged method ~line 495).

Group A — UI exists but send/receive is broken:

1. VNAM0…VNAM9 (addresses 02 00 00 01…0A): SysEx send is broken.
   Root cause (01_code_map.md): the loop in MidiSysex.h oscSendMidiMessage branch has
   condition `0 == message[0].getInt32()` which is always false for a non-zero OSC argument,
   so no voice-name character SysEx is ever sent.
   Fix: correct the loop condition so it iterates over the array of OSC messages
   and sends each character's SysEx via sendSysex. The sysexdata template with
   sysexdata[6] incremented per character (01…0A) is already prepared in Voice.h.

2. ELMODE (address 02 00 00 00): send ✅ but receive ❌.
   Root cause: comboMode in Voice.h is a plain juce::ComboBox, not a MidiCombo,
   so incoming SysEx matching valueSysexIn [3..6] = {02,00,00,00} never updates it.
   Fix option A (minimal): in VoicePage::valueChanged (or add it), check if
     valueSysexIn matches {02,00,00,00} and call comboMode.setSelectedItemIndex(value).
   Fix option B (preferred): replace comboMode with MidiCombo and call setMidiSysex
     with template {43, sysexEngine, 34, 02, 00, 00, 00, 00, 0}.

Constraints:
- Do NOT change UI layout or add new visible controls.
- Do NOT modify slider ranges or other parameter routes.

Output: modified MidiSysex.h and Voice.h only.
```

---

## Промпт #5 — Добавление Group B: вкладка Voice Common в Voice.h

```
Read _agent_context/00_SPEC.md, 05_missing_audit.md (Group B section).
Read Sysex77-master/Source/Voice.h (VoicePage class, constructor, resized(), paint(),
  existing tab structure or child component layout).
Read Sysex77-master/Source/MidiObjects.h (MidiSlider constructor, setMidiSysex(int[9]),
  setRangeAndRound; MidiCombo constructor and addItem usage).
Read _agent_context/03_parameter_map.csv — columns: offset_hex, param_name,
  range_min, range_max, display_unit.

Tech stack: C++/JUCE. No JavaScript. No React.

Group B — 22 Voice Common parameters with NO UI and NO SysEx send
(addresses 02 00 00 28 through 02 00 00 43):
  WPBR(28), ATPBR(29), PMASN(2A), PMRNG(2B), AMASN(2C), AMRNG(2D),
  FMASN(2E), FMRNG(2F), PNLASN(30), PNLRNG(31), COASN(32), CORNG(33),
  PNBASN(34), PNBRNG(35), EGBASN(36), EGBRNG(37), WLASN(38), WLLML(39),
  MCTUN(3A), RNDP(3B), AFTMD(42), SPTPNT(43).

Task: Add a new section/component "Common Params" to VoicePage in Voice.h:

For each parameter:
- Create a MidiSlider (for continuous params: WPBR, ATPBR, PMRNG, AMRNG, FMRNG,
  PNLRNG, CORNG, PNBRNG, EGBRNG, WLLML, MCTUN, RNDP, SPTPNT)
  OR MidiCombo (for assign/enum params: PMASN, AMASN, FMASN, PNLASN, COASN,
  PNBASN, EGBASN, WLASN, AFTMD).
- Call setMidiSysex with array:
    int sysex[9] = {0x43, sysexEngine, 0x34, 0x02, 0x00, 0x00, NN, 0x00, 0};
  where NN = last byte of offset_hex from CSV (e.g. WPBR: NN=0x28).
- For MidiSlider: call setRangeAndRound(range_min, range_max, 1).
- For MidiCombo: addItem for each valid value (use range_min…range_max or enum labels).
- Add a juce::Label next to each control with text = param_name from CSV.
- Declare controls as member variables in VoicePage.
- Add them with addAndMakeVisible() in the constructor.
- Lay out in resized() — a simple vertical or grid layout is fine.
  Group visually: [Pitch Mod] [Amp Mod] [Filter Mod] [Pan] [EG Bias] [Volume] [Other].

Constraints:
- Do NOT remove or modify existing controls.
- Do NOT add effects, element-level, or AWM parameters in this task.
- Keep the new section collapsible or in a separate sub-panel if the page is already crowded.

Output: modified Voice.h only.
```

---

## Промпт #6 — Итоговый аудит и обновление документации

```
Read _agent_context/00_SPEC.md (Current Status table).
Read _agent_context/05_missing_audit.md (full file).
Read Sysex77-master/Source/Voice.h (updated in Prompt #5).
Read Sysex77-master/Source/MidiSysex.h (updated in Prompts #3 and #4).

Task: Update documentation to reflect completed work.

1. In 05_missing_audit.md, update statuses for parameters addressed in Prompts #3–#5:
   - VNAM0…VNAM9: send_implemented ⚠️ → ✅ (if fixed in Prompt #4).
   - ELMODE: receive_implemented ❌ → ✅ (if fixed in Prompt #4).
   - All 22 Group B parameters: both columns ❌ → ✅ (if controls added in Prompt #5).
   - Add note in the Notes column: "added Prompt #N" or "fixed Prompt #N".

2. In 00_SPEC.md, update the "Current Status" metrics table:
   - Recalculate "Implemented (send)", "Implemented (receive)", "Missing UI" counts.

Do NOT modify any source code.
Output: updated 05_missing_audit.md and 00_SPEC.md only.
```

---

> **Итог.** После выполнения всех шести промптов Sysex77 покрывает полный Voice Common SY99  
> с двусторонней SysEx-связью, throttle-защитой и echo guard.  
> Следующий шаг: расширить аудит на Elements 1–8 и Effects (отдельная серия промптов).

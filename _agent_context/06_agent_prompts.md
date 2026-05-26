# Agent Prompts — Пошаговые задачи для реализации SY99 редактора

> **Первым:** [`07_architecture_index.md`](07_architecture_index.md) + [`00_SPEC.md`](00_SPEC.md).  
> **Продукт:** C++/JUCE (`Sysex77-master/Source/*.h`). **Debug mirror:** React `ui/` + Param API `:8765`.  
> **Registry:** `sy99_param_bindings.json` + `LiveSynthState`; `valueSysexIn` = legacy only (R-NEW-5).  
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

---

## Промпт #9 — Pan.h: входящая синхронизация графа Pan EG

```
Read _agent_context/00_SPEC.md, 01_code_map.md (Pan / Hook section),
05_missing_audit.md (строка про "граф Pan EG"), 09_confirmed_addresses.md (block 0x0A).
Read Sysex77-master/Source/Pan.h, Hook.h, ADSR.h (SADSR setSegmentRate/Level API).
Read Sysex77-master/Source/MidiDemo.h (valueSysexIn — глобальный Value var-array
из 6 элементов: data[3..8]).

Tech stack: C++/JUCE. Виджеты MidiSlider слушают valueSysexIn и обновляют
свой Slider::setValue(...) при совпадении sysexData[3..6] с пришедшим адресом.
В Pan.h ChangeBroadcaster от MidiSlider Value уже триггерит
syncSegmentsFromSliders() — но это путь через слайдеры. Прямой парсинг входящих
сообщений делает синхронизацию надёжнее (особенно когда слайдер не виден или
release-level короче, чем dispatcher).

Task: расширить Pan::valueChanged(Value&) обработкой valueSysexIn для
адресов `0A [elem] 00 03..0F` (Rate/Level), `0A [elem] 00 02` (HT) и
`0A [elem] 00 10` (SLP):

1. Хранить текущий element offset в члене `int elementOffset` (заполняется
   в setElementNumber).
2. В Pan() / setElementNumber вызвать `valueSysexIn.addListener(this)`,
   в ~Pan() — removeListener.
3. В valueChanged(Value&) первой веткой проверить
   `value.refersToSameSourceAs(valueSysexIn)`:
   - распарсить b3=0x0A, b4=elementOffset, b5=0x00;
   - по b6 вызвать egPan.setSegmentRate / setSegmentLevel /
     setReleaseLevel / setRelease / setHoldTime / setLoopPoint
     с конвертацией raw → percentage из существующих lambdas;
   - использовать `dontSendNotification`-семантику (НЕ трогать слайдеры
     и НЕ слать SysEx);
   - вернуться без вызова syncSegmentsFromSliders().
4. Слайдер-логика send (sliderValueChanged) НЕ меняется.

Constraints:
- Изменения ТОЛЬКО в Pan.h.
- Не отправлять исходящих SysEx из обработчика valueSysexIn.
- Сохранять isUpdating guard.

Output: modified Pan.h only.
```

---

## Промпт #10 — WaveEg.h: аудит и фикс адреса sliderR2 (AWM Amp EG R2)

```
Read _agent_context/00_SPEC.md, 09_confirmed_addresses.md (block 0x07),
sy99_sysex_complete.md (Group 07 — AWM Element Data, particularly NN=0x50/0x51),
05_missing_audit.md (строка AWM EG R2 (E1)/(E3)).
Read Sysex77-master/Source/WaveEg.h (setElementNumber/applyElem/setMidiSysex
для sliderR2 — сейчас segment-1 rate).

Контекст: setElementNumber собирает шаблон `sysexdata2[9] = { 0x43, 0x10, 0x34,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00 }` и далее меняет только sysexdata2[4] (elem)
и sysexdata2[6] (NN). Это значит, что все слайдеры egWave (sliderR1..RR2 и
sliderL0..RL2) отправляют параметр в **group 0x00 (Multi Common Data)** — это
неверный группа per sy99_sysex_complete.md. Корректная группа — 0x07 (AWM Element
Data). sliderAwmR2E1/E3 уже подключены к подтверждённым адресам 07/00/00/50 и
07/40/00/51 — а sliderR2 (segment 1 rate в графе egWave) семантически = PAR2
(Amp EG Decay Rate / R2) = `0x07/EE/00/0x51` per service manual.

Task:
1. Зафиксировать, что текущий шаблон sliderR2 = `00 EE 00 03` — wrong group.
2. Заменить setMidiSysex на корректный массив
   `{ 0x43, 0x10, 0x34, 0x07, elemOffset, 0x00, 0x51, 0x00, 0 }`.
3. Обновить _agent_context/05_missing_audit.md — добавить строку про fix.

Constraints:
- Не трогать другие сегменты (sliderR1/R3/R4/RR1/RR2, sliderL0..RL2) —
  отдельный аудит Amp EG, для отдельного промпта.
- Не менять диапазоны (setRangeAndRound 0,63).
- Не вводить новых UI-контролов.

Output: modified WaveEg.h + 05_missing_audit.md.
```

---

## Промпт #11 — Расширение CSV: Elements 1–4 + Effects (Groups 0x03/0x05/0x07/0x08)

```
Read _agent_context/00_SPEC.md, 03_parameter_map.csv (формат столбцов),
sy99_sysex_complete.md (Group 02, 03, 05, 07, 08), 09_confirmed_addresses.md.

Tech stack: C++/JUCE. Шаблон SysEx: F0 43 10 34 GG EE 00 NN 0V VV F7.
Element selector EE: 0x00=E1, 0x20=E2, 0x40=E3, 0x60=E4.

Task: добавить строки в 03_parameter_map.csv (source=manual) для всех параметров
групп 0x03, 0x05, 0x07, 0x08 из sy99_sysex_complete.md, которых ещё нет в CSV.

Соглашения:
- Канон: E1 (b4=0x00) одна строка на параметр; E2..E4 — те же адреса с b4
  заменой 0x20/0x40/0x60 (отражается в комментарии notes).
- offset_hex — ровно 8 hex chars (`gghllpp` где gg=group, hh=elem, ll=00, pp=NN).
- group_id: `NE_E1_*` (Normal Element), `AFMC_E1_*` (AFM Element Common),
  `AWM_E1_*` (AWM Element), `EFX_*` (Effects).
- Если у параметра нет известного UI в Sysex77 — статус ❌ в 05_missing_audit.md.
- Если у параметра нет данных с железа и формат под вопросом (биты, multi-byte) —
  статус ❓.

В 05_missing_audit.md добавить секции:
- "Elements 1–4 + Effects — расширение CSV"
- Под-таблицы по группам 0x03, 0x05, 0x07, 0x08 с колонками
  `param_name | address | UI_control | send | receive | notes`.

Constraints:
- Не добавлять рабочий код. Только CSV + audit.
- Не дублировать существующие строки (AWM_E1_R2 / AWM_E3_R2 уже есть для
  адресов 07/00/00/50 и 07/40/00/51 — добавить новую запись AWM_E1_PARI/PAR2
  с разнесённой семантикой согласно manual).
- Эффекты с переменной семантикой параметров (EF1PRM1..10) оставить
  range_min/max пустыми; в notes описать «диапазон зависит от EF1TYPE».

Output: updated 03_parameter_map.csv + 05_missing_audit.md.
```

---

## Промпт #12 — Library skeleton (`Librairie.h`) + WaveEg Amp EG remap

Реализовано инкрементом `sysex77_library_only_increment.patch` / `sysex77_next_library_increment.patch` (одно и то же по коду). Детали — `_agent_context/05_missing_audit.md`, секция «Library & WaveEg (Prompt #12)».

**Дополнение — синхронизация библиотеки ↔ голос ([LIBSYNC]):** патчи **`sysex77_library_patch_sync_increment.patch`** и **`sysex77_full_with_library_sync.patch`** (код одинаковый по сути) перенесены в дерево поверх этого скелета. Полная таблица и ограничения — секция «**Library ↔ Voice sync**» в `05_missing_audit.md`. При поиске в чат‑логах не путать с **Промпт #9 ниже про Pan.h**.

### Часть A — Librairie.h

```
Read Sysex77-master/Source/Librairie.h, BankTableModel.h, VoicesTableModel.h,
MidiDemo.h (globals), MidiSysex.h (OSC handlers).

Task: добавить в LibrairiePage базовые операции библиотеки без поломки bulk-pipeline.

1) IMPORT — FileChooser → копирование `.syx` в appDirPath; refresh списка.
2) EXPORT — FileChooser save для текущего bankSelected.
3) SEND VOICE — выполнено в LIBSYNC: парсинг кадров и OSC `/77SendVoice` (`MidiSysex.h`), не fallback на `/77SendBank`.
4) REQUEST VOICE — тот же receive-пайплайн (timer + btStop).
5) saveDump — файлы `DUMP-YYYYMMDD-HHMMSS.syx`, проверка пустого дампа.

Constraints:
- Не менять формат сохранения (raw concat).
- std::unique_ptr<FileChooser> chooser как член класса.
- После LIBSYNC: `/77SendVoice` разрешён (слайсер по `F0…F7` в `BankTableModel`).

Output: Librairie.h + обновление 05_missing_audit.md и 06_agent_prompts.md.
```

### Часть B — WaveEg.h Amp EG remap (`b3=0x07`)

```
Переадресовать sliderR1/R2/R3/R4/RR1, sliderL2/L3, sliderSlope на NN 0x50..0x57
per sy99_sysex_complete.md (группа 0x07). E1: sliderR1 диапазон 0..62.

Constraints:
- L0/L1/L4/RL1/RL2 остаются на legacy b3=0x00 до подтверждения NN.
- Hook/addSegment второй шаблон не переписывать в этом промпте.

Output: WaveEg.h + 05_missing_audit.md + 00_SPEC.md (метрики).
```


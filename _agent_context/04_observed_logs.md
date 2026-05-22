# Наблюдаемые SysEx-логи — Pan EG и смежные параметры

**Источник данных:** `Sysex77-master/Source/MIDI_MAP_OBSERVATIONS.md`  
*(Данные записаны вручную из монитора Sysex77 после исправления очереди в `MidiDemo.h`.  
Отдельного файла `analiz-midi.txt` не существует — журнал наблюдений ведётся в `MIDI_MAP_OBSERVATIONS.md`.)*

**Краткое содержание:** блоки ниже показывают **формат лога**, **адрес главного параметра** и **сопутствующие адреса** которые SY99 шлёт при том же жесте — то что агент должен уметь различать. Дополнительно зафиксирован **Voice Common — ELMODE** (ручной тест двусторонней связи) и **канонический эталон привязки SY99 — Output Group El.1 / MCTEN/OUTSEL**.

---

## Reference binding pattern — Output Group El.1 / MCTEN/OUTSEL (WORKING)

**Дата журнала:** 2026-05-19  
**Статус:** **confirmed working reference pattern** — использовать как шаблон для будущих привязок параметров SY99 (в т.ч. bit-packed и non-slider UI).

| Поле | Значение |
|------|----------|
| **UI name** | Output Group El.1 |
| **technical/manual name** | MCTEN/OUTSEL |
| **address** | `03 00 00 08` |
| **full SysEx** | `F0 43 1n 34 03 00 00 08 00 VV F7` |
| **ValueTree property** | `ELEMENT1OUTSEL` |
| **transport control** | `sliderMixerEl1Outsel` |
| **mapping** | off=`0x00`, G1=`0x02`, G2=`0x04`, both=`0x06` |
| **inbound** | `valueSysexIn` → `MidiSlider::valueChanged` → `ValueTree` → UI binding |
| **outbound** | UI → `setValue` → `sliderValueChanged` → `/SYSEX` → `sendToOutputs` |
| **status** | confirmed working reference pattern |

**Код:** `Voice.h` (`setupMixerElOutsel`, `sliderMixerEl1Outsel`), `MixerSection.h` (`MixerOutputGroupBinding` — UI-кнопки Group 1/2; read-modify-write полного байта VV; без параллельного `valueSysexIn`-listener на binding).  
**UI:** `VoiceCommonPage` → Mixer → El.1 → Output Group (две toggle-кнопки).  
**Bit-packed:** биты MCTEN сохраняются через read-modify-write; transport owner остаётся привязан к тому же байту/адресу `03 00 00 08`.

---

## [CONFIRMED WORKING] Effect Mode (EFMODE) и Effect Send El.1

**Дата журнала:** 2026-05-21  
**Статус:** **CONFIRMED WORKING** (JUCE Mixer + live SysEx); bulk EFMODE — **не подтверждён**.

### EFMODE — глобальный режим эффектов

| Поле | Значение |
|------|----------|
| **address** | `08 00 00 20` |
| **full SysEx** | `F0 43 1n 34 08 00 00 20 00 VV F7` |
| **UI range** | 0 Off / 1 Serial / 2 Parallel |
| **ValueTree property** | `EFMODE` |
| **transport control** | `VoicePage::sliderEfmode` (hidden receive-only) |
| **visibility rule** | Off → скрыть Send 1 и Send 3; Serial → только Send 1; Parallel → Send 1 + Send 3 |

### EFLN1EL — выбор линий Send (El.1 only)

| Поле | Значение |
|------|----------|
| **address** | `03 00 00 09` (E1 `b4=00`; El.2–4 не реализованы на SY99) |
| **full SysEx** | `F0 43 1n 34 03 00 00 09 00 VV F7` |
| **bitmask** | Send 1=`0x01`, Send 3=`0x04`, оба=`0x05` |
| **ValueTree property** | `ELEMENT1EFSENDSEL` |
| **UI binding** | `MixerEffectSendSelectBinding` (Send 1 / Send 3 toggles) |

### Level / Vel / Scaling (El.1)

| Параметр | address | UI range | ValueTree |
|----------|---------|----------|-----------|
| EFSDLV (Level) | `03 00 00 0A` | 0…127 | `ELEMENT1EFSENDLVL` |
| EFSDVSNS (Vel) | `03 00 00 0B` | −7…+7 | `ELEMENT1EFSENDVSNS` |
| EFSDSCL (Scaling) | `03 00 00 0C` | −7…+7 | `ELEMENT1EFSENDSCL` |

**Signed7 ladder:** raw `0x00`→0, `0x01`→+1, `0x02`→+2 … `0x07`→+7; `0x79`→−7 … `0x7F`→−1.

**Bulk 8101VC:** EFLN1EL @ ELVL E1 + Δ подтверждён (fixture 03 → `0x05`). Offsets Level/Vel/Scaling @ efln+1…+3 — **кандидаты** (fixture 03 bulk lvl=0 vs LCD 127).

**Код:** `Voice.h`, `MixerSection.h`, `Sy99ParamRegistry` (`EFMODE`, `EFLN1EL`, `EFSDLV`, `EFSDVSNS`, `EFSDSCL`).

---

## [CONFIRMED WORKING] Element Level (ELVL)

**Дата журнала:** 2026-05-19  
**Статус:** **CONFIRMED WORKING** — App→Synth, Synth→App.

| Поле | Значение |
|------|----------|
| **address** | `03 EE 00 00` (E1 `EE=00`, E2 `20`, E3 `40`, E4 `60`) |
| **full SysEx** | `F0 43 1n 34 03 EE 00 00 00 VV F7` |
| **meaning** | Element Level |
| **UI range** | 0 … 127 |
| **ValueTree property** | `ELEMENTnVOLUME` |
| **transport control** | `VoicePage::sliderMixerEl1Level` … `sliderMixerEl4Level` (dedicated `MidiSlider` per element; no reparent) |
| **Mixer** | `VoiceCommonPage` → Mixer → Level row (`attachStripLevelControl`) |
| **inbound / outbound** | existing `MidiSlider` path + `valueSysexIn` (one transport per element; not batch-duplicated) |

---

## [CONFIRMED WORKING] Element Detune (ELDT)

**Дата журнала:** 2026-05-19  
**Статус:** **CONFIRMED WORKING** — App→Synth, Synth→App, Mixer ↔ Pitch panel synchronized.

| Поле | Значение |
|------|----------|
| **address** | `03 EE 00 01` (E1 `EE=00`, E2 `20`, E3 `40`, E4 `60`) |
| **full SysEx** | `F0 43 1n 34 03 EE 00 01 00 VV F7` |
| **meaning** | Element Detune |
| **UI range** | −7 … +7 |
| **ValueTree property** | `ELEMENTnFINE` |
| **transport control** | `Pitch::sliderFine` (reparent to Mixer Detune row on Common tab) |
| **inbound / outbound** | existing `MidiSlider` + `valueSysexIn` (no second subscriber) |
| **Mixer** | `VoiceCommonPage` → `attachStripDetuneControl` / `restorePitchDetuneSliders` |

---

## [CONFIRMED WORKING] Element Note Shift (ELNS)

**Дата журнала:** 2026-05-19  
**Статус:** **CONFIRMED WORKING** — App→Synth, Synth→App, Mixer ↔ Pitch panel synchronized.

| Поле | Значение |
|------|----------|
| **address** | `03 EE 00 02` (E1 `EE=00`, E2 `20`, E3 `40`, E4 `60`) |
| **full SysEx** | `F0 43 1n 34 03 EE 00 02 00 VV F7` |
| **meaning** | Element Note Shift |
| **UI range** | −64 … +63 |
| **SYSEX VV range** | 0 … 127 |
| **conversion** | UI→VV: `VV = clamp(UI, −64..+63) + 64`; VV→UI: `UI = clamp(VV, 0..127) − 64` |
| **ValueTree property** | `ELEMENTnPITCH` |
| **transport control** | `Pitch::sliderPitch` + `setElementNoteShiftSignedSysexEncode(true)` |
| **inbound / outbound** | existing `MidiSlider` + `valueSysexIn` (no second subscriber) |
| **Mixer** | `VoiceCommonPage` → `attachStripNoteShiftControl` / `restorePitchDetuneSliders` (shared attach/restore with Detune) |

---

## [IMPLEMENTED] Element Note Limit Low (ENLL)

**Дата журнала:** 2026-05-20  
**Статус:** **IMPLEMENTED** — transport wired; live SY99 App↔Synth test pending.

| Поле | Значение |
|------|----------|
| **address** | `03 EE 00 03` (E1 `EE=00`, E2 `20`, E3 `40`, E4 `60`) |
| **full SysEx** | `F0 43 1n 34 03 EE 00 03 00 VV F7` |
| **meaning** | Element Note Limit Low |
| **UI range** | 0 … 127 (display: MIDI note names) |
| **UI display** | `MidiMessage::getMidiNoteName(note, true, true, 3)` e.g. `C3`, `G#4` |
| **ValueTree property** | `ELEMENTnNOTELIMITLOW` |
| **transport control** | `VoicePage::sliderMixerEl1NoteLimitLow` … `sliderMixerEl4NoteLimitLow` |
| **Mixer** | `VoiceCommonPage` → Note Limit Low row (`attachStripNoteLimitLowControl`) |
| **inbound / outbound** | existing `MidiSlider` + `valueSysexIn` (same path as ELVL) |

---

## [IMPLEMENTED] Element Note Limit High (ENLH)

**Дата журнала:** 2026-05-20  
**Статус:** **IMPLEMENTED** — transport wired; live SY99 App↔Synth test pending.

| Поле | Значение |
|------|----------|
| **address** | `03 EE 00 04` (E1 `EE=00`, E2 `20`, E3 `40`, E4 `60`) |
| **full SysEx** | `F0 43 1n 34 03 EE 00 04 00 VV F7` |
| **meaning** | Element Note Limit High |
| **UI range** | 0 … 127 (display: MIDI note names) |
| **UI display** | `MidiMessage::getMidiNoteName(note, true, true, 3)` e.g. `C3`, `G#4` |
| **ValueTree property** | `ELEMENTnNOTELIMITHIGH` |
| **transport control** | `VoicePage::sliderMixerEl1NoteLimitHigh` … `sliderMixerEl4NoteLimitHigh` |
| **Mixer** | `VoiceCommonPage` → Note Limit High row (`attachStripNoteLimitHighControl`) |
| **inbound / outbound** | existing `MidiSlider` + `valueSysexIn` (same path as ENLL) |
| **UI default (temp)** | 127 at far right when `ELEMENTnNOTELIMITHIGH` not in ValueTree — until patch/pattern restore |

---

## [IMPLEMENTED] Element Velocity Limit Low (EVLL)

**Дата журнала:** 2026-05-20  
**Статус:** **IMPLEMENTED** — transport wired; live SY99 App↔Synth test pending.

| Поле | Значение |
|------|----------|
| **address** | `03 EE 00 05` (E1 `EE=00`, E2 `20`, E3 `40`, E4 `60`) |
| **full SysEx** | `F0 43 1n 34 03 EE 00 05 00 VV F7` |
| **meaning** | Element Velocity Limit Low |
| **UI range** | 0 … 127 |
| **ValueTree property** | `ELEMENTnVELOCITYLIMITLOW` |
| **transport control** | `VoicePage::sliderMixerEl1VelocityLimitLow` … `sliderMixerEl4VelocityLimitLow` |
| **Mixer** | `VoiceCommonPage` → Velocity Limit Low row (`attachStripVelocityLimitLowControl`) |
| **inbound / outbound** | existing `MidiSlider` + `valueSysexIn` (same path as ELVL) |

---

## [IMPLEMENTED] Element Velocity Limit High (EVLH)

**Дата журнала:** 2026-05-20  
**Статус:** **IMPLEMENTED** — transport wired; live SY99 App↔Synth test pending.

| Поле | Значение |
|------|----------|
| **address** | `03 EE 00 06` (E1 `EE=00`, E2 `20`, E3 `40`, E4 `60`) |
| **full SysEx** | `F0 43 1n 34 03 EE 00 06 00 VV F7` |
| **meaning** | Element Velocity Limit High |
| **UI range** | 0 … 127 |
| **ValueTree property** | `ELEMENTnVELOCITYLIMITHIGH` |
| **transport control** | `VoicePage::sliderMixerEl1VelocityLimitHigh` … `sliderMixerEl4VelocityLimitHigh` |
| **Mixer** | `VoiceCommonPage` → Velocity Limit High row (`attachStripVelocityLimitHighControl`) |
| **inbound / outbound** | existing `MidiSlider` + `valueSysexIn` (same path as EVLL) |

---

## [LESSON LEARNED] Common tab — batch `MidiSlider` subscribers (regression)

**Дата журнала:** 2026-05-19  
**Статус:** regression documented; mitigated by rollback + binding policy.

**Симптом:** вкладка **Common** «пропадала» / UI зависал после попытки привязать сразу пачку новых `MidiSlider` на Mixer (Detune, Note Shift, Limits и т.д.) — по одному транспорту на параметр × 4 элемента.

**Причина:** каждый `MidiSlider` в конструкторе подписывается на глобальный `valueSysexIn` и пишет в лог на **каждый** входящий SysEx; десятки новых подписчиков → шторм обновлений на UI-потоке. Дублирование адресов (напр. Detune уже на `Pitch::sliderFine`) усугубляло нагрузку.

**Безопасный паттерн (обязателен для Mixer Group 0x03):**

1. **Один параметр за раз** — не батчить несколько новых `MidiSlider` в одном PR/сессии.
2. **Reuse existing transport** — reparent существующего `MidiSlider` + то же свойство ValueTree (`Pitch::sliderFine` / `sliderPitch`), **без** второго `valueSysexIn`-listener.
3. Если отдельный transport неизбежен (как **Level**) — только выделенные слайдеры `sliderMixerEl*n*Level`, уже заведённые в `VoicePage`; не плодить дубликаты на тот же адрес.
4. После reparent — сброс bounds + `resized()` полосы (`relayoutLiveStripRows`).

**Откат:** batch-binding Detune/Note Shift/Limits отменён; Detune и Note Shift восстановлены через reparent (см. секции ELDT/ELNS выше).

---

## Voice Common: ELMODE — ручной тест и соответствие

**Дата журнала:** 2026-05-17  
**CSV / мануал:** `VC_M_00`, параметр **`ELMODE`**, `offset_hex` = **`02000000`** (`03_parameter_map.csv`).  
**Код редактора:** `VoicePage::comboMode` → отправка параметр-изменений; приём по `valueSysexIn`: адрес **`02 00 00 00`**, сырое значение **`b8` = 0…10** → `comboMode.setSelectedId(b8 + 1)` (`Sysex77-master/Source/Voice.h`).  
**Вердикт:** двусторонняя связь признана **стабильной**: перебор режимов от первого пункта комбо до **DRUM SET** даёт монотонную лестницу `vv`, UI и синх сходятся.

### Наблюдаемые кадры (монитор Sysex77)

Преамбула в логе: `Controller Data Button increment/decrement … Channel 1` — описание MidiDemo; полезное — кадры:

```text
[#1]  SysEx: F0 43 10 34 02 00 00 00 00 00 F7    ; ELMODE raw 0  → режим пункт 1
[#2]  SysEx: F0 43 10 34 02 00 00 00 00 01 F7
[#3]  SysEx: F0 43 10 34 02 00 00 00 00 02 F7
[#4]  SysEx: F0 43 10 34 02 00 00 00 00 03 F7
[#5]  SysEx: F0 43 10 34 02 00 00 00 00 04 F7
[#6]  SysEx: F0 43 10 34 02 00 00 00 00 05 F7
[#7]  SysEx: F0 43 10 34 02 00 00 00 00 06 F7
[#8]  SysEx: F0 43 10 34 02 00 00 00 00 07 F7
[#9]  SysEx: F0 43 10 34 02 00 00 00 00 08 F7
[#10] SysEx: F0 43 10 34 02 00 00 00 00 09 F7
[#11] SysEx: F0 43 10 34 02 00 00 00 00 0A F7    ; ELMODE raw 10 → DRUM SET (пункт 11)
```

*Примечание:* в текстовой консоли `02` иногда отображается как `2` — эквивалентно **`byte[3] = 0x02`**.

### Соответствие байт ↔ параметр

| Поле payload (9 байт после `43 10 34`) | Значение | Смысл |
|---------------------------------------|----------|--------|
| `43 10 34` | фикс. | универсальный шаблон parameter change (device `10`, модель-байт `34`) |
| `02 00 00 00` | фикс. | адрес **`ELMODE`** (группа Voice Common Data, см. CSV `02000000`) |
| `00` перед `vv` | `byte[7] = 0x00` | как в большинстве 9-байтных параметров этого проекта |
| последний байт перед `F7` | **`00`…`0A`** | сырое **ELMODE** 0…10 (семантика мануала: 11 режимов включая Drum) |

Мапинг **сирец SY99 ↔ пункт комбо (1-based):** `comboId = ELMODE_raw + 1`.

### Фильтрация для агента при разборе логов ELMODE

- Брать кадры с **`43 10 34 02 00 00 00 00 vv`** где **`vv`** ∈ **`00…0A`**.  
- Не смешивать с Pan (`b3=0A` в этом проекте = Pan EG блок) или AWM (`b3=07`/`03` для других сцен).

### ⚠ Путаница с именем голоса (VNAM)

Те же строки монитора с адресом **`02 00 00 00 00 vv`** (**`b6 = 00`**) — это **не** символы имени голоса, а **ELMODE** (см. таблица выше).  
Имя (**VNAM0…9**) задаётся **десятью** отдельными кадрами с **`b6 = 01..0A`** (параметры `02000001` … `0200000A`): последний байт — символ ASCII (7-бит после `0x20`).

Ручное наблюдение (2026-05): **синт → комп** имени работает через эти сообщения при редактировании имени на SY99.

**Комп → синт** имени исторически срывался по нескольким причинам:

1. **`sysexEngine`** в `MidiDemo.h` мог оставаться **0** до захода в Config; тогда шли кадры `43 00 34 …` вместо рабочих `43 10 34 …` как у успешного ELMODE. **Исправление по умолчанию:** инициализация `sysexEngine` значением **`0x10`**.
2. Отправка VNAM висела только на **Enter**; при уходе с поля имени после набора символы могли не уйти — добавлен **`textEditorFocusLost`** + флаг реального редактирования (`voiceNameDirtyForMidiOut`).
3. **Нижний монитор MIDI** (`midiMonitor`) мог **забирать фокус клавиатуры** — ввод «не работал» в поле имени, а ответы синта попадали между отдельными `sendToOutputs` и путали обмен. **Исправления:** у монитора `setWantsKeyboardFocus(false)` и `setMouseClickGrabsKeyboardFocus(false)`; пачка из 10 VNAM уходит через **`MidiDemo::sendMidiMessagesToOutputsShunted`** с **одним** окном `boolStopReceive` на весь пакет. Ветка OSC `/77MidiMessage` принимает **int32 или float** для счётчика сообщений.

**Статус (2026-05, решение пользователя):** поле имени (**`VoicePage::editName`**) в UI **оставляем**. **Комп → синт** имени как **рабочего сценария не фиксируем** и **далее по отладке не ведём** — по вашей оценке для реальной работы **не хватает набора символов / «алфавита»** (ограничения SY99 против свободного ввода в редакторе или наоборот). Исправления выше сохранены в коде на будущее; возврат к теме только по явному запросу.

Код см. **`VoicePage::flushVoiceNameToSynth()`**, **`MidiDemo::sendMidiMessagesToOutputsShunted`**, **`MidiSysex.h`** (ветка `/77MidiMessage`).

---

## Формат строки лога (Sysex77 monitor)

```
SysEx: 43 10 34 [b3] [b4] [b5] [b6] [b7] [b8]
```

Или в коротком виде (без ведущих нулей):
```
43 10 34 A 0 0 6 0 3F
```

Полный кадр на шине: `F0 43 10 34 [b3 b4 b5 b6 b7 b8] F7` (9 байт payload).

---

## Блок 1: PAN EG Rate R4 (жест 0 → 63)

**Главный адрес:** `b3=0A b4=00 b5=00 b6=06` → `43 10 34 0A 00 00 06 00 xx`  
**Ожидаемый лог (post-fix RECORDED):** `[#1]…[#63]` — `byte[8]` подряд `01→02→…→3F`

```text
SysEx: 43 10 34 A 0 0 6 0 01
SysEx: 43 10 34 A 0 0 6 0 02
SysEx: 43 10 34 A 0 0 6 0 03
... (63 строки подряд) ...
SysEx: 43 10 34 A 0 0 6 0 3F
```

**Кодирование:** `byte[8]` = прямое значение 0…63. `byte[7]` всегда `0x00`.

### Сопутствующие сообщения в этом блоке

| Адрес | Что это | Как отличить |
|-------|---------|--------------|
| `43 10 34 07 00 00 50 00 xx` | AWM EG R2 (E1) — **не Pan R4** | `b3=07`, а не `0A` |
| `43 10 34 03 00 00 07 00 xx` | Legacy Pan R1 — **старый путь** | `b3=03`, только на графике |
| `43 10 34 02 00 00 42 00 01` | AFTMD (Voice Common) — фоновый при навигации | `b3=02`, всегда `b8=01` |

**Правило фильтрации для R4:** брать только строки с `b3=0A` + `b6=06`.

---

## Блок 2: PAN EG Rate R3 (жест 0 → 63)

**Главный адрес:** `b3=0A b4=00 b5=00 b6=05` → `43 10 34 0A 00 00 05 00 xx`  
**Ожидаемый лог (post-fix RECORDED):** `[#1]…[#63]` — `byte[8]` подряд `01→…→3F`

```text
SysEx: 43 10 34 A 0 0 5 0 01
SysEx: 43 10 34 A 0 0 5 0 02
... (63 строки) ...
SysEx: 43 10 34 A 0 0 5 0 3F
```

**Архивный полный спуск 63→0** (`[#927]…[#990]`, 64 строки):
```text
SysEx: 43 10 34 A 0 0 5 0 3F
SysEx: 43 10 34 A 0 0 5 0 3E
... (64 строки) ...
SysEx: 43 10 34 A 0 0 5 0 00
```

---

## Блок 3: PAN EG Rate R2 (жест 0→63 и 63→0)

**Главный адрес:** `b3=0A b4=00 b5=00 b6=04` → `43 10 34 0A 00 00 04 00 xx`

**Подъём** (post-fix RECORDED, `[#1]…[#63]`):
```text
SysEx: 43 10 34 A 0 0 4 0 01
... (63 строки) ...
SysEx: 43 10 34 A 0 0 4 0 3F
```

**Спуск** (архив `[#862]…[#925]`, 64 строки):
```text
SysEx: 43 10 34 A 0 0 4 0 3F
... (64 строки) ...
SysEx: 43 10 34 A 0 0 4 0 00
```

### ⚠ Загрязнение этого блока

В композитных логах рядом с `b3=0A`/`b6=04` (Pan R2) встречается:
```text
SysEx: 43 10 34 07 00 00 50 00 3E   ← это AWM EG R2 (E1), b3=07
```
Агент должен игнорировать строки с `b3=07` при фильтрации Pan R2.

---

## Ключевые паттерны — "главный адрес" vs "шум"

### Таблица различения для Pan EG (вкладка Rate)

| Тип | Признак | Действие агента |
|-----|---------|-----------------|
| **Главный параметр** | `b3=0A`, `b6` из диапазона `02…08`, монотонная лестница ±1 | ✅ Записать в CSV / использовать в коде |
| **Другой элемент** | `b3=0A`, тот же `b6`, но `b4=20/40/60` вместо `00` | ℹ️ Тот же параметр другого элемента — не дублировать адрес |
| **Legacy / граф** | `b3=03`, `b6=07` (R1) или `b3=07`, `b6=06` (HT) | ❌ Старый путь, заменён `0A`-блоком |
| **AWM шум** | `b3=07`, `b6=50` (E1) или `b6=51` (E3) | ❌ AWM EG, не Pan — другой блок |
| **Фоновый Voice Common** | `b3=02`, `b6=42` (`AFTMD`) | ❌ Навигационный артефакт |
| **Нет лестницы** | Скачки > 1, пропуски, < 5 строк | ❌ Шум / ускорение энкодера |

### Правило одного жеста

При правильном захвате (Clear → один параметр → INC/DEC):
- **63 сообщения** `01…3F` — если стартовали не с `0`
- **64 сообщения** `00…3F` или `3F…00` — полный диапазон

Если строк значительно больше — в логе несколько параметров или Legacy-копии по элементам.

### Почему SY99 шлёт "лишние" сообщения

1. **График Pan EG** — движение общей точки дёргает сразу HT + R1 → два адреса подряд.  
   Числовое поле на вкладке Rate даёт только один адрес.
2. **Legacy multi-element** — блок `0x03` рассылал изменение R1 по всем элементам  
   (`b4=00/20/40/60` одновременно). Канонический `0x0A` этого не делает.
3. **Контекст AWM** — если до Pan EG редактировали AWM, SY99 может продолжать  
   слать фоновые AWM-сообщения при навигации.

---

## Полный канон Pan EG (для агента — сводная таблица)

| Параметр | b3 | b6 | Диапазон | Кодирование |
|----------|----|----|----------|-------------|
| HT | `0A` | `02` | 0…63 | прямое |
| R1 | `0A` | `03` | 0…63 | прямое |
| R2 | `0A` | `04` | 0…63 | прямое |
| R3 | `0A` | `05` | 0…63 | прямое |
| R4 | `0A` | `06` | 0…63 | прямое |
| RR1 | `0A` | `07` | 0…63 | прямое |
| RR2 | `0A` | `08` | 0…63 | прямое |
| L0 | `0A` | `09` | −32…+31 | raw−0x20 |
| L1 | `0A` | `0A` | −32…+31 | raw−0x20 |
| L2 | `0A` | `0B` | −32…+31 | raw−0x20 |
| L3 | `0A` | `0C` | −32…+31 | raw−0x20 |
| L4 | `0A` | `0D` | −32…+31 | raw−0x20 |
| RL1 | `0A` | `0E` | −32…+31 | raw−0x20 |
| RL2 | `0A` | `0F` | −32…+31 | raw−0x20 |
| SLP | `0A` | `10` | 0…3 (S1–S4) | прямое |
| Pan Source | `0A` | `00` | 0…2 | 0=Vel/1=Note/2=LFO |
| Source Depth | `0A` | `01` | 0…127 | прямое |

Для элементов E2/E3/E4 — заменить `b4` на `20`/`40`/`60`. Адрес остаётся тем же.

---

## Адреса которые НЕ нужно реализовывать (они — шум)

| Адрес | Почему не нужен |
|-------|----------------|
| `b3=07, b6=06` | Legacy HT — заменён `0A`/`02` |
| `b3=03, b6=07` | Legacy R1 — заменён `0A`/`03` |
| `b3=03, b6=08` | Фрагмент без идентификации — не трогать |
| `b3=02, b6=42` в контексте Pan | AFTMD навигационный артефакт — уже в Voice Common CSV |

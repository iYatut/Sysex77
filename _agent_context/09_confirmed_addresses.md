# Подтверждённые SysEx-адреса SY99 (выжимка из MIDI_MAP_OBSERVATIONS.md)

> **Источник:** `Sysex77-master/Source/MIDI_MAP_OBSERVATIONS.md`  
> **Принцип отбора:** только разделы с пометкой **CONFIRMED** или **post-fix RECORDED**  
> (монотонные лестницы ±1 после исправления очереди в `MidiDemo.h`).  
>
> ⚠ **Агенту:** читать **этот файл**, не `MIDI_MAP_OBSERVATIONS.md` напрямую.  
> `MIDI_MAP_OBSERVATIONS.md` содержит legacy-адреса и архивные дампы с ошибками буфера.

---

## Формат сообщения (9 байт между F0 и F7)

```
F0  43  10  34  [b3]  [b4]  [b5]  [b6]  [b7]  [b8]  F7
                block  elem   00   addr   00   value
```

- `b4` = element offset: `0x00`=E1, `0x20`=E2, `0x40`=E3, `0x60`=E4
- `b7` всегда `0x00` (MSB значения не используется)
- `b8` = само значение (LSB)

---

## Блок 0x0A — Pan EG (Rate, Level, SLP, Pan Source)

Кадр E1: `F0 43 10 34 0A 00 00 [b6] 00 [val] F7`  
Для E2/E3/E4 заменить `b4`: `20` / `40` / `60`.

### Вкладка Rate — кодирование `b8` = прямое 0…63

| Параметр | b6   | Диапазон экрана | Кодирование b8 |
|----------|------|-----------------|----------------|
| HT       | `02` | 0…63            | прямое         |
| R1       | `03` | 0…63            | прямое         |
| R2       | `04` | 0…63            | прямое         |
| R3       | `05` | 0…63            | прямое         |
| R4       | `06` | 0…63            | прямое         |
| RR1      | `07` | 0…63            | прямое         |
| RR2      | `08` | 0…63            | прямое         |

> Note: в post-fix дампах b8 идёт как `01…3F` (63 сообщения); отдельного кадра для `0x00`  
> в лестнице нет — пользователь стартовал с ненулевого значения. Полный диапазон 0…63  
> подтверждён архивными сериями (64 сообщения `00…3F`) и мануалом.

### Вкладка Level — кодирование `b8 = raw`, display = raw − 0x20

| Параметр | b6   | Диапазон экрана | Кодирование b8        |
|----------|------|-----------------|-----------------------|
| L0       | `09` | −32…+31         | raw 0x00…0x3F         |
| L1       | `0A` | −32…+31         | raw 0x00…0x3F         |
| L2       | `0B` | −32…+31         | raw 0x00…0x3F         |
| L3       | `0C` | −32…+31         | raw 0x00…0x3F         |
| L4       | `0D` | −32…+31         | raw 0x00…0x3F         |
| RL1      | `0E` | −32…+31         | raw 0x00…0x3F         |
| RL2      | `0F` | −32…+31         | raw 0x00…0x3F         |
| SLP      | `10` | S1…S4 (0…3)     | прямое 0x00…0x03      |

> SLP RECORDED: `[#0]…[#3]` — подряд `xx = 00…03` при S1…S4. Пятого значения нет.

### Element Pan (Common → Element Pan → F8 edit)

| Параметр     | b6   | Диапазон | Кодирование b8     |
|--------------|------|----------|--------------------|
| Pan Source   | `00` | 0…2      | 0=Velocity, 1=Note, 2=LFO |
| Source Depth | `01` | 0…127    | прямое (RECORDED: `7F…00` = 128 сообщений) |

> Эти два параметра в `Pan.h` **не подключены** к MidiSlider (только журнал).

---

## Блок 0x56 — AFM Operator EG

Кадр E1: `F0 43 10 34 56 00 00 [b6] 00 [val] F7`  
Для E2/E3/E4 заменить `b4` аналогично Pan EG.

| Параметр | b6   | Диапазон | Кодирование b8 | Evidence |
|----------|------|----------|----------------|---------|
| R1       | `00` | 0…63     | прямое         | подъём `00…3F`, 64 сообщ. |
| R2       | `01` | 0…63     | прямое         | подъём `00…3F`, 64 сообщ. |
| HT       | `0D` | 0…63     | прямое         | спуск `3F…00`, 64 сообщ. |

> b6 = `0x1B` встречается как фрагмент (3–5 значений) без подтверждённой лестницы — **не использовать**.

---

## Блок 0x07 — AWM Element EG

| Параметр    | b4   | b6   | Диапазон | Кодирование b8       | Evidence |
|-------------|------|------|----------|----------------------|---------|
| EG R2 (E1)  | `00` | `50` | 0…62     | display ≈ raw − 1    | `[#1]…[#63]`: `01…3F` |
| EG R2 (E3)  | `40` | `51` | 0…63     | прямое               | монотонный спуск |

> b4=`0x40`, b6=`0x4F` (рядом с AWM R2 E3) — 0–1, возможно режим; **не подтверждено**.  
> b4=`0x40`, b6=`0x17` — одно значение (3); **не использовать**.

---

## Блок 0x03 — Element Level / ELVL (NN `0x00`)

| Element | b4 (EE) | Address |
|---------|---------|---------|
| E1      | `0x00`  | `03 00 00 00` |
| E2      | `0x20`  | `03 20 00 00` |
| E3      | `0x40`  | `03 40 00 00` |
| E4      | `0x60`  | `03 60 00 00` |

Кадр: `F0 43 10 34 03 EE 00 00 00 [VV] F7`

| Поле | Значение |
|------|----------|
| UI range | 0 … 127 |
| ValueTree | `ELEMENTnVOLUME` |
| transport | `sliderMixerEl1Level` … `sliderMixerEl4Level` |
| status | **CONFIRMED WORKING** (2026-05-19) |

Журнал: `04_observed_logs.md` § *Element Level*.

---

## Блок 0x03 — Element Detune / ELDT (NN `0x01`)

| Element | b4 (EE) | Address |
|---------|---------|---------|
| E1      | `0x00`  | `03 00 00 01` |
| E2      | `0x20`  | `03 20 00 01` |
| E3      | `0x40`  | `03 40 00 01` |
| E4      | `0x60`  | `03 60 00 01` |

Кадр: `F0 43 10 34 03 EE 00 01 00 [VV] F7`

| Поле | Значение |
|------|----------|
| UI range | −7 … +7 |
| ValueTree | `ELEMENTnFINE` |
| transport | `Pitch::sliderFine` (+ Mixer row via reparent) |
| status | **CONFIRMED WORKING** (2026-05-19) |

Журнал: `04_observed_logs.md` § *Element Detune*.

---

## Блок 0x03 — Element Note Shift / ELNS (NN `0x02`)

| Element | b4 (EE) | Address |
|---------|---------|---------|
| E1      | `0x00`  | `03 00 00 02` |
| E2      | `0x20`  | `03 20 00 02` |
| E3      | `0x40`  | `03 40 00 02` |
| E4      | `0x60`  | `03 60 00 02` |

Кадр: `F0 43 10 34 03 EE 00 02 00 [VV] F7`

| Поле | Значение |
|------|----------|
| UI range | −64 … +63 |
| VV range | 0 … 127 |
| conversion | UI→VV: `VV = UI + 64`; VV→UI: `VV − 64` (with clamp) |
| ValueTree | `ELEMENTnPITCH` |
| transport | `Pitch::sliderPitch` + signed encode in `MidiObjects.h` |
| status | **CONFIRMED WORKING** (2026-05-19) |

Журнал: `04_observed_logs.md` § *Element Note Shift*.

---

## Блок 0x03 — Element Note Limit Low / ENLL (NN `0x03`)

| Element | b4 (EE) | Address |
|---------|---------|---------|
| E1      | `0x00`  | `03 00 00 03` |
| E2      | `0x20`  | `03 20 00 03` |
| E3      | `0x40`  | `03 40 00 03` |
| E4      | `0x60`  | `03 60 00 03` |

Кадр: `F0 43 10 34 03 EE 00 03 00 [VV] F7`

| Поле | Значение |
|------|----------|
| UI range | 0 … 127 |
| UI display | `MidiMessage::getMidiNoteName(note, true, true, 3)` |
| ValueTree | `ELEMENTnNOTELIMITLOW` |
| transport | `sliderMixerEl1NoteLimitLow` … `sliderMixerEl4NoteLimitLow` |
| status | **IMPLEMENTED** (2026-05-20); live SY99 test pending |

Журнал: `04_observed_logs.md` § *Element Note Limit Low*.

---

## Блок 0x03 — Element Note Limit High / ENLH (NN `0x04`)

| Element | b4 (EE) | Address |
|---------|---------|---------|
| E1      | `0x00`  | `03 00 00 04` |
| E2      | `0x20`  | `03 20 00 04` |
| E3      | `0x40`  | `03 40 00 04` |
| E4      | `0x60`  | `03 60 00 04` |

Кадр: `F0 43 10 34 03 EE 00 04 00 [VV] F7`

| Поле | Значение |
|------|----------|
| UI range | 0 … 127 |
| UI display | `MidiMessage::getMidiNoteName(note, true, true, 3)` |
| ValueTree | `ELEMENTnNOTELIMITHIGH` |
| transport | `sliderMixerEl1NoteLimitHigh` … `sliderMixerEl4NoteLimitHigh` |
| status | **IMPLEMENTED** (2026-05-20); live SY99 test pending |

Журнал: `04_observed_logs.md` § *Element Note Limit High*.

---

## Блок 0x03 — Element Velocity Limit Low / EVLL (NN `0x05`)

| Element | b4 (EE) | Address |
|---------|---------|---------|
| E1      | `0x00`  | `03 00 00 05` |
| E2      | `0x20`  | `03 20 00 05` |
| E3      | `0x40`  | `03 40 00 05` |
| E4      | `0x60`  | `03 60 00 05` |

Кадр: `F0 43 10 34 03 EE 00 05 00 [VV] F7`

| Поле | Значение |
|------|----------|
| UI range | 0 … 127 |
| ValueTree | `ELEMENTnVELOCITYLIMITLOW` |
| transport | `sliderMixerEl1VelocityLimitLow` … `sliderMixerEl4VelocityLimitLow` |
| status | **IMPLEMENTED** (2026-05-20); live SY99 test pending |

Журнал: `04_observed_logs.md` § *Element Velocity Limit Low*.

---

## Блок 0x03 — Element Velocity Limit High / EVLH (NN `0x06`)

| Element | b4 (EE) | Address |
|---------|---------|---------|
| E1      | `0x00`  | `03 00 00 06` |
| E2      | `0x20`  | `03 20 00 06` |
| E3      | `0x40`  | `03 40 00 06` |
| E4      | `0x60`  | `03 60 00 06` |

Кадр: `F0 43 10 34 03 EE 00 06 00 [VV] F7`

| Поле | Значение |
|------|----------|
| UI range | 0 … 127 |
| ValueTree | `ELEMENTnVELOCITYLIMITHIGH` |
| transport | `sliderMixerEl1VelocityLimitHigh` … `sliderMixerEl4VelocityLimitHigh` |
| status | **IMPLEMENTED** (2026-05-20); live SY99 test pending |

Журнал: `04_observed_logs.md` § *Element Velocity Limit High*.

---

## Блок 0x03 — Output Group / MCTEN/OUTSEL (NN `0x08`)

> **Name cross-reference (same parameter):**
> - **UI name:** Output Group
> - **Technical / manual name:** MCTEN/OUTSEL (transcription also lists MCTEN/OUTOSEL/OUT1SEL)
> - **Legacy project name:** Voice group (`oscVoiceGrp1..4` in `MidiSysex.h`)
>
> **Element 1 address:** `03 00 00 08`  
> **Elements 2–4:** same NN, element offset in `b4`:
>
> | Element | b4 (EE) | Address `[b3 b4 b5 b6]` |
> |---------|---------|-------------------------|
> | E1      | `0x00`  | `03 00 00 08`           |
> | E2      | `0x20`  | `03 20 00 08`           |
> | E3      | `0x40`  | `03 40 00 08`           |
> | E4      | `0x60`  | `03 60 00 08`           |
>
> Кадр E1: `F0 43 10 34 03 00 00 08 00 [VV] F7`

### OUTSEL — подтверждённый 4-state mapping (`b8` / VV)

| UI state | VV   | Notes                          |
|----------|------|--------------------------------|
| off      | `0x00` | обе группы выключены           |
| G1       | `0x02` | Group 1 (bit `0x02`)           |
| G2       | `0x04` | Group 2 (bit `0x04`)           |
| both     | `0x06` | обе группы (`outselMask 0x06`) |

Байт bit-packed: биты MCTEN могут делить тот же `b8` с OUTSEL — для переключения групп нужен **read-modify-write** полного байта.

> ⚠ **Journal note:** до 2026-05-19 `_agent_context` перечислял параметр по адресу и имени
> (MCTEN/OUTSEL / «Voice group»), но **не документировал** 4-state mapping OUTSEL (`0x00` /
> `0x02` / `0x04` / `0x06`). Добавлено по подтверждению с железа при реализации Mixer Output Group El.1.

### Reference binding pattern (WORKING — use for future SY99 bindings)

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

Полная запись журнала: `04_observed_logs.md` § *Reference binding pattern*.

---

## Адреса, которые выглядят как одно, но являются другим

Эти паттерны встречаются в старых/архивных дампах файла `MIDI_MAP_OBSERVATIONS.md`  
и **не должны** использоваться в редакторе или CSV:

| Legacy-адрес | Контекст в логе | Реальный канон |
|-------------|-----------------|----------------|
| `0x07` / `0x06` | Старый HT Pan (до фикса буфера) | `0x0A` / `0x02` |
| `0x03` / `0x07` | Старый R1 Pan (архив) | `0x0A` / `0x03` |
| `0x03` / `0x08` | Короткий фрагмент в архиве без имени | **MCTEN/OUTSEL / Output Group** — см. блок 0x03 NN `0x08` выше |

### ⚠ Специальный случай: адрес `0x02` / `0x42`

В таблице UNKNOWN в `MIDI_MAP_OBSERVATIONS.md` строка:  
`byte[3]=0x02, byte[4]=0x00, byte[6]=0x42, byte[8]=1, контекст="Pan EG nav"`

Это **НЕ параметр Pan**. `02 00 00 42` = **AFTMD** (Zoned AfterTouch mode) из Voice Common  
(см. `03_parameter_map.csv`, строка `VC_M_AFTMD`). Появился в логе как фоновое сообщение  
при навигации, не как результат редактирования Pan EG.

---

## Итог: что брать в CSV / код

| Использовать | Не использовать |
|-------------|-----------------|
| Блок `0x0A` полностью (все Rate/Level/SLP/Source) | `0x07`/`06`, `0x03`/`07` |
| Блок `0x03` NN `00` (ELVL Level; E1..E4) | |
| Блок `0x03` NN `01` (ELDT Detune; E1..E4) | |
| Блок `0x03` NN `02` (ELNS Note Shift; E1..E4) | |
| Блок `0x03` NN `08` (Output Group / MCTEN/OUTSEL; E1..E4) | |
| Блок `0x56`: R1, R2, HT | `0x56`/`1B` (фрагмент) |
| Блок `0x07`: R2 E1 (`50`) и R2 E3 (`51`) | `0x07`/`4F` (неподтверждено) |

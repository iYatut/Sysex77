# Аудит параметров: карта CSV ↔ код Sysex77

Источники: **`_agent_context/01_code_map.md`**, **`_agent_context/03_parameter_map.csv`**, проверка **`Sysex77-master/Source/`** (`Voice.h`, `Pan.h`, `MidiSysex.h`, `MidiDemo.h`, `MidiObjects.h`).

**Адрес в таблице** — четыре байта полезной нагрузки параметра **`[3][4][5][6]`** в сообщении вида `F0 43 … 34 … F7` (как в `01_code_map.md`). Для строк из CSV поле **`offset_hex`** трактуется как **`GGhhllpp`** → **`GG hh ll pp`** (например `0200003F` → **`02 00 00 3F`**).

**Статусы:** ✅ done · ⚠️ partial · ❌ missing · ❓ unknown

**Критерии**

| Статус | Отправка (`send_implemented`) | Приём (`receive_implemented`) |
|--------|-------------------------------|--------------------------------|
| ✅ | Есть явный путь в коде (`MidiSlider`/`MidiButton`/OSC→`sendSysex`/хук `Segment::sendOsc` и т.д.) | Входящий 9-байтовый SysEx с тем же префиксом **`43 … 34`** обрабатывается (`valueSysexIn` → `valueChanged` виджета с совпадающим шаблоном) |
| ⚠️ | Реализовано частично (обходной OSC, заглушка, только часть режимов, или код подозрителен) | Частично или только при совпадении с виджетом |
| ❌ | Нет привязки в UI / сломанный путь | Нет сопоставления с виджетом |
| ❓ | Недостаточно данных без прогона на железе | То же |

---

## Таблица

| param_name | address | UI_control | send_implemented | receive_implemented | notes |
|------------|---------|------------|------------------|---------------------|-------|
| ELMODE | 02 00 00 00 | `VoicePage::comboMode` + OSC `adresseOpMode` → `MidiSysex.h` | ✅ | ❌ | Отправка через OSC; **`MidiCombo`/`valueSysexIn` для режима нет** — синх не подтверждён с синта. Ручной диапазон SY99 **0…10**, в UI индексы **0…9** (**10 режимов**) — см. `OBS_IMPL` в CSV. |
| VNAM0…VNAM9 | 02 00 00 01 … 0A | `VoicePage::editName` (`textEditorReturnKeyPressed`) | ⚠️ | ❌ | Шаблон адреса верный (`sysexdata[6]` от **01** по символам); цепочка **`oscSendMidiMessage`** в `MidiSysex.h` с циклом **`0 == message[0].getInt32()`** фактически **не отправляет** — см. `01_code_map.md`. |
| WPBR | 02 00 00 28 | — | ❌ | ❌ | Voice Common из CSV; в перечисленных модулях `01_code_map` нет слайдера. |
| ATPBR | 02 00 00 29 | — | ❌ | ❌ | То же. |
| PMASN | 02 00 00 2A | — | ❌ | ❌ | То же. |
| PMRNG | 02 00 00 2B | — | ❌ | ❌ | То же. |
| AMASN | 02 00 00 2C | — | ❌ | ❌ | То же. |
| AMRNG | 02 00 00 2D | — | ❌ | ❌ | То же. |
| FMASN | 02 00 00 2E | — | ❌ | ❌ | То же. |
| FMRNG | 02 00 00 2F | — | ❌ | ❌ | То же. |
| PNLASN | 02 00 00 30 | — | ❌ | ❌ | То же. |
| PNLRNG | 02 00 00 31 | — | ❌ | ❌ | То же. |
| COASN | 02 00 00 32 | — | ❌ | ❌ | То же. |
| CORNG | 02 00 00 33 | — | ❌ | ❌ | То же. |
| PNBASN | 02 00 00 34 | — | ❌ | ❌ | То же. |
| PNBRNG | 02 00 00 35 | — | ❌ | ❌ | То же. |
| EGBASN | 02 00 00 36 | — | ❌ | ❌ | То же. |
| EGBRNG | 02 00 00 37 | — | ❌ | ❌ | То же. |
| WLASN | 02 00 00 38 | — | ❌ | ❌ | То же. |
| WLLML | 02 00 00 39 | — | ❌ | ❌ | То же. |
| MCTUN | 02 00 00 3A | — | ❌ | ❌ | То же. |
| RNDP | 02 00 00 3B | — | ❌ | ❌ | То же. |
| WOL | 02 00 00 3F | `VoicePage::sliderMaster` (+ дублирующий OSC `oscTotalVoiceVolume` в `MidiSysex.h`) | ✅ | ✅ | `MidiSlider::setMidiSysex`; совпадает с **`OBS_IMPL_3F`** в CSV. |
| AFTMD | 02 00 00 42 | — | ❌ | ❌ | Нет в списке контролов Voice в `01_code_map`. |
| SPTPNT | 02 00 00 43 | — | ❌ | ❌ | То же. |
| ELMODE_impl_code | 02 00 00 00 | (дубликат строки ELMODE в CSV `OBS_IMPL_00`) | ✅ | ❌ | Отражает расхождение **индекс combo 0…9** vs ручной **ELMODE 0…10**. |
| WOL_impl_code | 02 00 00 3F | (дубликат WOL / `OBS_IMPL_3F`) | ✅ | ✅ | Дубль записи в CSV для трассировки «как в коде». |
| _reference_files_ | 00 00 00 00 | — | — | — | Строка **`ZZ00`** в CSV — метаданные, не параметр SY99. |
| **PAN_EG_Rate_R1** *(ошибочная связка)* | **07 00 00 50** | — | ❓ | ❓ | **Не Pan EG R1.** По `sy99_sysex_complete.md` / `MIDI_MAP_OBSERVATIONS` это **AWM element group `07`**, **NN=`50`** (часто **Velocity Sens / смежный AWM EG R2 E1** в логах). **Не использовать** как адрес Pan R1. |
| PNR1 / PAN EG Rate R1 | 0A 00 00 03 | `Pan.h` `sliderR1` (`MidiSlider`) | ✅ | ✅ | Element **2/3:** байт **`[4]`** → **`20` / `40`** (`applyElem`). Хуки графика шлют те же шаблоны через `Segment::sendOsc`. |
| PNR2 / PAN EG Rate R2 | 0A 00 00 04 | `Pan.h` `sliderR2` | ✅ | ✅ | То же. |
| PNR3 / PAN EG Rate R3 | 0A 00 00 05 | `Pan.h` `sliderR3` | ✅ | ✅ | То же. |
| PNR4 / PAN EG Rate R4 | 0A 00 00 06 | `Pan.h` `sliderR4` | ✅ | ✅ | То же. |
| PNRR1 / PAN EG Rate RR1 | 0A 00 00 07 | `Pan.h` `sliderRR1` | ✅ | ✅ | То же. |
| PNRR2 / PAN EG Rate RR2 | 0A 00 00 08 | `Pan.h` `sliderRR2` | ✅ | ✅ | То же. |
| PAN EG Hold (HT) Rate | 0A 00 00 02 | `Pan.h` `sliderHT` | ✅ | ✅ | В журнале также упоминается legacy **`07 00 00 06`** — в текущем **`Pan.h`** канон **`0A`/`02`**. |
| PNSLP | 0A 00 00 10 | `Pan.h` `sliderSLP` | ✅ | ✅ | Диапазон UI **0…3** (`MIDI_MAP_OBSERVATIONS`). |
| PNL0…PNL4, PNRL1…PNRL2 | 0A 00 00 09…0E, 0F | `Pan.h` `sliderL0`…`sliderRL2` | ✅ | ✅ | Уровни Pan EG; **`sliderRL2`** привязан к **`0A`/`0F`** в `setElementNumber`. |
| *(граф Pan EG)* | *(те же адреса сегментов)* | `Hook` / `Segment::sendOsc` | ✅ | ⚠️ | Перетаскивание шлёт **два** SysEx (rate+level); обратная синхронизация крючков с входящим MIDI **не описана** как полная в `01_code_map`. |

---

## Группы для приоритетной доработки

> Группы используются в `06_agent_prompts.md` при планировании задач.

### Группа A — UI-контрол есть, отправка/приём сломаны или отсутствуют

| param_name | address | Проблема |
|------------|---------|----------|
| ELMODE | 02 00 00 00 | ✅ send / ❌ receive — `comboMode` не является `MidiCombo`, `valueSysexIn` не сопоставляется |
| VNAM0…VNAM9 | 02 00 00 01…0A | ⚠️ send — цикл `oscSendMidiMessage` в `MidiSysex.h` (~181–187) имеет условие `0 == message[0].getInt32()`, которое всегда ложно для ненулевого OSC-аргумента |

### Группа B — нет UI-контрола, нет отправки (Voice Common, адреса `02 00 00 28…43`)

| param_name | address | Диапазон |
|------------|---------|----------|
| WPBR | 02 00 00 28 | 0…12 |
| ATPBR | 02 00 00 29 | −12…12 |
| PMASN | 02 00 00 2A | 0…121 |
| PMRNG | 02 00 00 2B | 0…127 |
| AMASN | 02 00 00 2C | 0…121 |
| AMRNG | 02 00 00 2D | 0…127 |
| FMASN | 02 00 00 2E | 0…121 |
| FMRNG | 02 00 00 2F | 0…127 |
| PNLASN | 02 00 00 30 | 0…121 |
| PNLRNG | 02 00 00 31 | 0…127 |
| COASN | 02 00 00 32 | 0…121 |
| CORNG | 02 00 00 33 | 0…127 |
| PNBASN | 02 00 00 34 | 0…121 |
| PNBRNG | 02 00 00 35 | 0…127 |
| EGBASN | 02 00 00 36 | 0…121 |
| EGBRNG | 02 00 00 37 | 0…127 |
| WLASN | 02 00 00 38 | 0…121 |
| WLLML | 02 00 00 39 | 0…127 |
| MCTUN | 02 00 00 3A | 0…65 |
| RNDP | 02 00 00 3B | 0…7 |
| AFTMD | 02 00 00 42 | enum (all/top/btm/hi/low) |
| SPTPNT | 02 00 00 43 | 0…127 |

---

## Приоритеты

1. **Исправить отправку имени голоса (`VNAM*`)** — ветка **`oscSendMidiMessage`** в `MidiSysex.h` помечена в `01_code_map.md` как логически сломанная; без этого строки CSV **VNAM0…9** остаются ⚠️/❌ по факту.

2. **Режим элементов (`ELMODE`)** — добавить приём с синта (**`receive_implemented`**) или явную синхронизацию через ValueTree, и сверить **11-й** режим SY99 (**DRUM_SET** и т.д.) с combo **0…9**.

3. **Voice Common с адресами `02 00 00 28`…`43`** (модуляция, границы громкости, micro tune, zoned AT…) — целиком **❌** в UI; для паритета с сервис-мануалом имеет смысл либо вкладка Common, либо отдельный аудит «не дублировать уже реализованное в других экранах».

4. **Не смешивать логи Pan и AWM** при аудите адресов: держать опору **`0A 00 00 03…08`** для Pan EG Rate и отдельно документировать **`07 … 50`** / **`07 … 51`** как AWM (`05_missing_audit` / `MIDI_MAP_OBSERVATIONS.md`).

5. **Pan EG через граф** — уточнить ⚠️ приём: нужен ли разбор входящих сообщений для обновления **`Hook`** без движения слайдера.

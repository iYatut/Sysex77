# Аудит параметров: карта CSV ↔ код Sysex77

Источники: **`_agent_context/01_code_map.md`**, **`_agent_context/03_parameter_map.csv`**, проверка **`Sysex77-master/Source/`** (`Voice.h`, `Pan.h`, `MidiSysex.h`, `MidiDemo.h`, `MidiObjects.h`).

**Адрес в таблице** — четыре байта полезной нагрузки параметра **`[3][4][5][6]`** в сообщении вида `F0 43 … 34 … F7` (как в `01_code_map.md`). Для строк из CSV поле **`offset_hex`** трактуется как **`GGhhllpp`** → **`GG hh ll pp`** (например `0200003F` → **`02 00 00 3F`**).

**Статусы:** ✅ done · ⚠️ partial · ❌ missing · ❓ unknown

> **Realtime SysEx safety:** в `MidiSysex.h::sendSysex` throttle ≤ 30 msg/sec
> (≈33 мс шаг) для параметр-чейнджей со слайдеров. Подавленное значение
> сохраняется в `pendingSysex[9]` и через `ThrottleFlushTimer` (35 мс one-shot)
> отправляется `flushPendingSysex()`, если новое значение его не вытеснило —
> так финальное положение слайдера всегда доходит до синта.
> В `MidiDemo.h::handleAsyncUpdate` echo-guard 50 мс — кольцевой буфер из
> 16 записей `(addr[3..6], sentAt)` блокирует обновление `valueSysexIn`,
> если совпадает с недавно отправленным адресом.

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
| ELMODE | 02 00 00 00 | `VoicePage::comboMode` + OSC `adresseOpMode` → `MidiSysex.h`; receive в `VoicePage::valueChanged(valueSysexIn)` | ✅ | ✅ | Добавлен 11-й режим **DRUM SET** (combo IDs 1…11). Приём через `valueSysexIn` сопоставляет `02 00 00 00` и зовёт `setSelectedId(mode+1)`. Fixed Prompt #4. |
| VNAM0…VNAM9 | 02 00 00 01 … 0A | `VoicePage::editName` (`textEditorReturnKeyPressed`); receive в `VoicePage::valueChanged(valueSysexIn)` | ✅ | ✅ | Отправка: цикл `oscSendMidiMessage` чинён, `sysexdata[6]` инкрементируется, пустые позиции — пробелы. Приём: `voiceNameBuffer[10]` накапливает входящие символы, `editName.setText(..., dontSendNotification)` обновляет UI без триггера обратной отправки (защита `receivingVNAM`). Fixed Prompt #4 + Prompt #7. |
| WPBR | 02 00 00 28 | `VoicePage::sliderWPBR` (MidiSlider) | ✅ | ✅ | Added Prompt #5 (commonPanel). |
| ATPBR | 02 00 00 29 | `VoicePage::sliderATPBR` (MidiSlider, rotary, диапазон -12…12) | ✅ | ✅ | Added Prompt #5. |
| PMASN | 02 00 00 2A | `VoicePage::comboPMASN` (MidiCombo) | ✅ | ✅ | Added Prompt #5. |
| PMRNG | 02 00 00 2B | `VoicePage::sliderPMRNG` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
| AMASN | 02 00 00 2C | `VoicePage::comboAMASN` (MidiCombo) | ✅ | ✅ | Added Prompt #5. |
| AMRNG | 02 00 00 2D | `VoicePage::sliderAMRNG` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
| FMASN | 02 00 00 2E | `VoicePage::comboFMASN` (MidiCombo) | ✅ | ✅ | Added Prompt #5. |
| FMRNG | 02 00 00 2F | `VoicePage::sliderFMRNG` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
| PNLASN | 02 00 00 30 | `VoicePage::comboPNLASN` (MidiCombo) | ✅ | ✅ | Added Prompt #5. |
| PNLRNG | 02 00 00 31 | `VoicePage::sliderPNLRNG` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
| COASN | 02 00 00 32 | `VoicePage::comboCOASN` (MidiCombo) | ✅ | ✅ | Added Prompt #5. |
| CORNG | 02 00 00 33 | `VoicePage::sliderCORNG` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
| PNBASN | 02 00 00 34 | `VoicePage::comboPNBASN` (MidiCombo) | ✅ | ✅ | Added Prompt #5. |
| PNBRNG | 02 00 00 35 | `VoicePage::sliderPNBRNG` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
| EGBASN | 02 00 00 36 | `VoicePage::comboEGBASN` (MidiCombo) | ✅ | ✅ | Added Prompt #5. |
| EGBRNG | 02 00 00 37 | `VoicePage::sliderEGBRNG` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
| WLASN | 02 00 00 38 | `VoicePage::comboWLASN` (MidiCombo) | ✅ | ✅ | Added Prompt #5. |
| WLLML | 02 00 00 39 | `VoicePage::sliderWLLML` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
| MCTUN | 02 00 00 3A | `VoicePage::sliderMCTUN` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
| RNDP | 02 00 00 3B | `VoicePage::sliderRNDP` (MidiSlider, 0…7) | ✅ | ✅ | Added Prompt #5. |
| WOL | 02 00 00 3F | `VoicePage::sliderMaster` (+ дублирующий OSC `oscTotalVoiceVolume` в `MidiSysex.h`) | ✅ | ✅ | `MidiSlider::setMidiSysex`; совпадает с **`OBS_IMPL_3F`** в CSV. |
| AFTMD | 02 00 00 42 | `VoicePage::comboAFTMD` (MidiCombo: all/top/btm/hi/low) | ✅ | ✅ | Added Prompt #5. |
| SPTPNT | 02 00 00 43 | `VoicePage::sliderSPTPNT` (MidiSlider) | ✅ | ✅ | Added Prompt #5. |
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
| PNSCSEL / Pan Source | 0A 00 00 00 | `Pan.h` `comboPanSrc` (`MidiCombo` — Velocity/Note/LFO) | ✅ | ✅ | Added Prompt #7. Применяет `applyElem` для E2/E3/E4. |
| PNSCDPT / Pan Source Depth | 0A 00 00 01 | `Pan.h` `sliderPanSrcDpt` (`MidiSlider` 0..127) | ✅ | ✅ | Added Prompt #7. RECORDED 7F..00 → 128 сообщений (`09_confirmed_addresses.md`). |
|| AFM Op EG R1 | 56 *e* 00 00 | `Operator.h` `sliderAfmR1` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Block 0x56, b6=0x00, range 0–63, element offset via b4 in `setElementNumber`. |
|| AFM Op EG R2 | 56 *e* 00 01 | `Operator.h` `sliderAfmR2` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Block 0x56, b6=0x01, range 0–63. |
|| AFM Op EG HT | 56 *e* 00 0D | `Operator.h` `sliderAfmHT` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Block 0x56, b6=0x0D, range 0–63. |
|| AWM EG R2 (E1) | 07 00 00 50 | `WaveEg.h` `sliderAwmR2E1` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Hardcoded address (b4=0x00, b6=0x50); range 0–62 (display=raw−1). |
|| AWM EG R2 (E3) | 07 40 00 51 | `WaveEg.h` `sliderAwmR2E3` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Hardcoded address (b4=0x40, b6=0x51); range 0–63 direct. |

---

## Группы для приоритетной доработки

> Группы используются в `06_agent_prompts.md` при планировании задач.

### Группа A — UI-контрол есть, отправка/приём сломаны или отсутствуют

| param_name | address | Статус |
|------------|---------|----------|
| ELMODE | 02 00 00 00 | ✅ ✅ — приём добавлен в `VoicePage::valueChanged(valueSysexIn)`; combo расширен 11-м пунктом `DRUM SET`. |
| VNAM0…VNAM9 | 02 00 00 01…0A | ✅ ⚠️ — отправка чинена: цикл в `MidiSysex.h::oscSendMidiMessage` теперь `i < count && i < oscMidiMessage.size()`, `oscMidiMessage` не `const`, в `Voice.h::textEditorReturnKeyPressed` адресный байт `[6]` инкрементируется по позициям и неиспользованные слоты добиваются `0x20`. Приём отдельных символов имени с синта не реализован — отдельная задача. |

### Группа B — Voice Common параметры `02 00 00 28…43` *(реализованы Prompt #5)*

Все 22 параметра ниже теперь имеют контрол и двусторонний SysEx через `commonPanel` в `Voice.h`.

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

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
| *(граф Pan EG)* | *(те же адреса сегментов)* | `Hook` / `Segment::sendOsc` + `Pan::valueChanged(valueSysexIn)` | ✅ | ✅ | Перетаскивание шлёт **два** SysEx (rate+level). Обратная синхронизация: `Pan::valueChanged(valueSysexIn)` парсит `0A [elem] 00 03..10` и вызывает `egPan.setSegmentRate/Level/Release/ReleaseLevel/HoldTime/LoopPoint` с `dontSendNotification` — без исходящих send. Element фильтруется по b4 (`elementOffset`). Fixed Prompt #9. |
| PNSCSEL / Pan Source | 0A 00 00 00 | `Pan.h` `comboPanSrc` (`MidiCombo` — Velocity/Note/LFO) | ✅ | ✅ | Added Prompt #7. Применяет `applyElem` для E2/E3/E4. |
| PNSCDPT / Pan Source Depth | 0A 00 00 01 | `Pan.h` `sliderPanSrcDpt` (`MidiSlider` 0..127) | ✅ | ✅ | Added Prompt #7. RECORDED 7F..00 → 128 сообщений (`09_confirmed_addresses.md`). |
|| AFM Op EG R1 | 56 *e* 00 00 | `Operator.h` `sliderAfmR1` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Block 0x56, b6=0x00, range 0–63, element offset via b4 in `setElementNumber`. |
|| AFM Op EG R2 | 56 *e* 00 01 | `Operator.h` `sliderAfmR2` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Block 0x56, b6=0x01, range 0–63. |
|| AFM Op EG HT | 56 *e* 00 0D | `Operator.h` `sliderAfmHT` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Block 0x56, b6=0x0D, range 0–63. |
|| AWM EG R2 (E1) | 07 00 00 50 | `WaveEg.h` `sliderAwmR2E1` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Hardcoded address (b4=0x00, b6=0x50); range 0–62 (display=raw−1). |
|| AWM EG R2 (E3) | 07 40 00 51 | `WaveEg.h` `sliderAwmR2E3` (MidiSlider) | ✅ | ✅ | Added Prompt #8. Hardcoded address (b4=0x40, b6=0x51); range 0–63 direct. |
|| AWM Amp EG R2 (per element via egWave) | 07 *e* 00 51 | `WaveEg.h` `sliderR2` (graph segment-1 rate) | ✅ | ✅ | Prompt #12: Amp EG remap — PAR2 на `07`/NN=`51`; см. секцию Library & WaveEg. Дубликат с `sliderAwmR2E3` для E3. |

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

## Elements 1–4 + Effects — расширение CSV (Prompt #9 audit)

> Добавлены строки в `03_parameter_map.csv` (E1 как канон; E2/E3/E4 — те же адреса
> с b4 = `0x20` / `0x40` / `0x60`). Все без UI и без подтверждения железом —
> отмечены **❓** (нет hardware-данных) или **❌** (известны, но нет контрола).

### Group 0x03 — Normal Voice Element Data (E1)

| param_name | address | UI_control | send | receive | notes |
|------------|---------|------------|------|---------|-------|
| ELVL | 03 00 00 00 | — | ❌ | ❌ | Element Level. Нет UI. |
| ELDT | 03 00 00 01 | — | ❌ | ❌ | Element Detune (-7..+7). |
| ELNS | 03 00 00 02 | — | ❌ | ❌ | Element Note Shift (-64..+63). |
| ENLL | 03 00 00 03 | — | ❌ | ❌ | Element Note Limit Low. |
| ENLH | 03 00 00 04 | — | ❌ | ❌ | Element Note Limit High. |
| EVLL | 03 00 00 05 | — | ❌ | ❌ | Element Velocity Limit Low. |
| EVLH | 03 00 00 06 | — | ❌ | ❌ | Element Velocity Limit High. |
| PANNM | 03 00 00 07 | — | ❌ | ❌ | PAN data set table select (0..31). |
| MCTEN/OUTSEL | 03 00 00 08 | — | ❓ | ❓ | Битовые поля: Micro Tuning enable + Output select. |
| EFLN1EL | 03 00 00 0A | — | ❓ | ❓ | Effect send lines 1-4 select (bit-packed). |
| EFSDLV | 03 00 00 0B | — | ❌ | ❌ | Effect send level (0..127). |
| EFSDVL | 03 00 00 0C | — | ❌ | ❌ | Effect send velocity sense (-7..+7). |

### Group 0x05 — AFM Element Common (E1)

| param_name | address | UI_control | send | receive | notes |
|------------|---------|------------|------|---------|-------|
| ALGNUM | 05 00 00 00 | — | ❌ | ❌ | Algorithm number (0..44). `Operator.h::sliderAlgo` есть, но без точной привязки к `0x05/0x00`. |
| FPR1 | 05 00 00 01 | — | ❌ | ❌ | AFM Pitch EG Rate 1. |
| FPR2 | 05 00 00 02 | — | ❌ | ❌ | AFM Pitch EG Rate 2. |
| FPR3 | 05 00 00 03 | — | ❌ | ❌ | AFM Pitch EG Rate 3. |
| FPRR1 | 05 00 00 04 | — | ❌ | ❌ | AFM Pitch EG key off Rate 1. |
| FPL0 | 05 00 00 05 | — | ❌ | ❌ | AFM Pitch EG Level 0. |
| FPL1 | 05 00 00 06 | — | ❌ | ❌ | AFM Pitch EG Level 1. |
| FPL2 | 05 00 00 07 | — | ❌ | ❌ | AFM Pitch EG Level 2. |
| FPL3 | 05 00 00 08 | — | ❌ | ❌ | AFM Pitch EG Level 3. |
| FPRL1 | 05 00 00 09 | — | ❌ | ❌ | AFM Pitch EG key off Level 1. |
| FPEGR | 05 00 00 0A | — | ❌ | ❌ | Pitch EG Range (0..3). |
| FPRS | 05 00 00 0B | — | ❌ | ❌ | Pitch EG Rate Scaling (0..7). |
| FVPSW | 05 00 00 0C | — | ❌ | ❌ | Velocity Switch (off/on). |
| FLFSPD | 05 00 00 0D | — | ❌ | ❌ | Main LFO Speed. |
| FLFDLY | 05 00 00 0E | — | ❌ | ❌ | Main LFO Delay. |
| FLFPMD | 05 00 00 0F | — | ❌ | ❌ | Main LFO Pitch Mod Depth. |
| FLFAMD | 05 00 00 10 | — | ❌ | ❌ | Main LFO Amp Mod Depth. |
| FLFFMD | 05 00 00 11 | — | ❌ | ❌ | Main LFO Filter Mod Depth. |
| FLFWAV | 05 00 00 12 | — | ❌ | ❌ | Main LFO Wave (0..5). |
| FLINTP | 05 00 00 13 | — | ❌ | ❌ | Main LFO Initial Phase. |
| SLFWD | 05 00 00 15 | — | ❓ | ❓ | Sub LFO Wave (0..3). |
| SLFS | 05 00 00 16 | — | ❓ | ❓ | Sub LFO Speed. |
| SLFDM | 05 00 00 17 | — | ❓ | ❓ | Sub LFO delay/decay mode. |
| SLFDT | 05 00 00 18 | — | ❓ | ❓ | Sub LFO Delay/Decay time. |
| SLPMD | 05 00 00 19 | — | ❓ | ❓ | Sub LFO Pitch Mod Depth mode. |

### Group 0x07 — AWM Element Data (E1)

| param_name | address | UI_control | send | receive | notes |
|------------|---------|------------|------|---------|-------|
| WSOURCE | 07 00 00 00 | — | ❓ | ❓ | AWM Wave Source. |
| AWMWAVE | 07 00 00 01 | `AWMVue.h` `sliderWaveForm` (косвенно) | ⚠️ | ⚠️ | 2-байтное значение; в коде есть таблица волн с индексом в [8]. |
| PPM | 07 00 00 02 | `AWMVue.h` `btFixed` | ⚠️ | ⚠️ | Fixed/Normal mode. Привязка через `btFixed` (требует сверки). |
| PNOTE | 07 00 00 03 | — | ❓ | ❓ | Fixed mode note. |
| PPF | 07 00 00 04 | `AWMVue.h` `sliderFine` | ⚠️ | ⚠️ | Fine tune; точная привязка адреса требует сверки. |
| PMLPMS | 07 00 00 05 | — | ❌ | ❌ | Pitch Mod Sensitivity. |
| PPR1..PPRR1 | 07 00 00 06..09 | — | ❌ | ❌ | AWM Pitch EG Rates. |
| PPL0..PPRL1 | 07 00 00 0A..0E | — | ❌ | ❌ | AWM Pitch EG Levels. |
| PPEGR | 07 00 00 0F | — | ❌ | ❌ | AWM Pitch EG Range. |
| PPRS | 07 00 00 10 | — | ❌ | ❌ | AWM Pitch EG Rate Scaling. |
| PVPSW | 07 00 00 11 | — | ❌ | ❌ | AWM Velocity Switch. |
| PLFSPD..PLINTP | 07 00 00 12..18 | — | ❌ | ❌ | AWM LFO speed/delay/depths/wave/phase. |
| PAEGMD | 07 00 00 4F | — | ❓ | ❓ | AWM Amp EG mode (normal/hold). |
| PARI | 07 00 00 50 | `WaveEg.h` `sliderR1`, `sliderAwmR2E1` | ✅ | ✅ | Prompt #12: `sliderR1` на AMP EG PARI (E1 диапазон 0..62). `sliderAwmR2E1` — жёсткий E1; семантика совпадает. |
| PAR2 | 07 00 00 51 | `WaveEg.h` `sliderR2`; `sliderAwmR2E3` для E3 | ✅ | ✅ | Prompt #12 remap. |
| PAR3 | 07 00 00 52 | `WaveEg.h` `sliderR3` | ✅ | ✅ | Prompt #12 remap (manual NN 0x52). |
| PAR4 | 07 00 00 53 | `WaveEg.h` `sliderR4` | ✅ | ✅ | Prompt #12 remap. |
| PARR1 | 07 00 00 54 | `WaveEg.h` `sliderRR1` | ✅ | ✅ | Prompt #12 remap. |
| PAL2 | 07 00 00 55 | `WaveEg.h` `sliderL2` | ✅ | ✅ | Prompt #12 remap. |
| PAL3 | 07 00 00 56 | `WaveEg.h` `sliderL3` | ✅ | ✅ | Prompt #12 remap. |
| PARS | 07 00 00 57 | `WaveEg.h` `sliderSlope` | ✅ | ⚠️ | Prompt #12 remap на NN `57`; кодирование −7..+7 без железной верификации. |
| PABP1..4 | 07 00 00 58..5B | — | ❌ | ❌ | Output Level Scaling Break Points. |
| PAOS1..4 | 07 00 00 5C..5F | — | ❓ | ❓ | Output Level Scaling Offsets (-128..+127, формат под вопросом). |
| PAVSON | 07 00 00 60 | — | ❌ | ❌ | AWM Velocity Sensitivity. |
| PARVSW | 07 00 00 61 | — | ❌ | ❌ | Attack Rate Velocity Switch. |
| PAMS | 07 00 00 62 | — | ❌ | ❌ | Amplitude Mod Sensitivity. |

### Group 0x08 — Effect Data (Prompt #11)

| param_name | address | UI_control | send | receive | notes |
|------------|---------|------------|------|---------|-------|
| EFMODE | 08 00 00 20 | — | ❌ | ❌ | Effect mode (off/serial/parallel). |
| EF1TYPE | 08 00 00 21 | — | ❌ | ❌ | Effect 1 type (0..60). |
| EF1PRM1..10 | 08 00 00 22..2B | — | ❓ | ❓ | Effect 1 parameters; диапазон зависит от типа. |
| EF1OUTLV1/2 | 08 00 00 2C..2D | — | ❌ | ❌ | Effect 1 output levels. |
| EF2TYPE | 08 00 00 2E | — | ❌ | ❌ | Effect 2 type. |
| EF2PRM1..10 | 08 00 00 2F..38 | — | ❓ | ❓ | Effect 2 parameters. |
| EF2EFBAL1 | 08 00 00 39 | — | ❌ | ❌ | Effect 2 mix level. |
| EF2OUTLV1/2 | 08 00 00 3A..3B | — | ❌ | ❌ | Effect 2 output levels. |
| OUT1EFBAL/OUT2EFBAL | 08 00 00 3C..3D | — | ❌ | ❌ | Output effect balance. |
| CTRL1PRM/ASN/MIN/MAX | 08 00 00 3E..41 | — | ❓ | ❓ | Controller 1 mapping. |
| CTRL2PRM/ASN/MIN/MAX | 08 00 00 42..45 | — | ❓ | ❓ | Controller 2 mapping. |
| EFLFOWV/SP, ER50DL, EFLFOPH | 08 00 00 46..49 | — | ❌ | ❌ | Effect LFO wave/speed/delay/phase. |

---

## Library & WaveEg (Prompt #12)

### Library skeleton (`Sysex77-master/Source/Librairie.h`)

Добавлены кнопки для базовой PC-editor UX:

| Button         | Назначение |
|----------------|------------|
| `btImport`     | FileChooser → копирование внешнего `.syx` в `appDirPath` и обновление списка банков (через `loadBankRequest` + `sendChangeMessage`). |
| `btExport`     | FileChooser save → копирование текущего `bankSelected` наружу. |
| `btSendVoice`  | **LIBSYNC:** после парсинга банка — OSC `/77SendVoice` `(fileName, slot 0…63)` → `MidiSysex.h` режет один кадр `F0…F7` из `.syx` и `sendRaw`. Ранее skeleton с fallback на `/77SendBank` **снят**. |
| `btRequestVoice` | Переиспользует receive-pipeline (`requestSysex=true; startTimer(500); btStop.setVisible(true)`). Пользователь должен инициировать «DUMP OUT → Edit Voice» на синте. |

Также:

- `saveDump` теперь создаёт файл `DUMP-YYYYMMDD-HHMMSS.syx` (через `getNonexistentSibling`), не перезаписывая прежние дампы.
- `labelInfoLine` показывает статусы операций (Imported, Exported, Receiving…, Send voice…).
- Все listeners удаляются в деструкторе.
- `std::unique_ptr<FileChooser> chooser` хранится как член для асинхронного API.

Частично / дальнейшие инкременты:

1. **LIBSYNC (`sysex77_full_with_library_sync.patch` / `sysex77_library_patch_sync_increment.patch`)**: парсинг кадров `F0…F7` и таблицы `voiceSysexFileOffsets`/`voiceSysexFileLengths` в `BankTableModel.h`; клики по слотам A–D обновляют `bankSelectedVoiceIndex` и зеркалируют имя в `Voice.h` через `bankSelectedVoiceNameValue`; `/77SendVoice` шлёт один кадр (`MidiSysex.h`). Полная распаковка bulk → слайдеры UI — всё ещё **не** в скоупе (см. аудит патча).
2. **После сохранённого дампа** авто-выбор банка: `UNNAMED.SYX`, иначе **самый свежий** `DUMP-*.syx`, иначе первая строка списка.
3. **Voice request SysEx**: long-form запрос single-voice не зафиксирован в `sy99_sysex_complete.md`; добавлять только после подтверждения с железа.
4. **VoicesTableModel double-click → send voice** — по желанию (повтор SEND VOICE).

### WaveEg Amp EG remap (`Sysex77-master/Source/WaveEg.h`)

Переадресация слайдеров с legacy `b3=0x00` (Multi Common) на канонический AWM Amp EG `b3=0x07` per `sy99_sysex_complete.md` (Group 07, NN=0x50..0x57).

| Slider       | b3   | b6   | Параметр (manual)                                | Диапазон |
|--------------|------|------|--------------------------------------------------|----------|
| `sliderR1`   | 0x07 | 0x50 | PARI (Amp EG Attack Rate)                        | E1: 0..62 (display=raw-1) / E2..E4: 0..63 |
| `sliderR2`   | 0x07 | 0x51 | PAR2 (Amp EG Decay Rate)                         | 0..63 |
| `sliderR3`   | 0x07 | 0x52 | PAR3                                             | 0..63 |
| `sliderR4`   | 0x07 | 0x53 | PAR4                                             | 0..63 |
| `sliderRR1`  | 0x07 | 0x54 | PARR1 (KeyOff Rate 1)                            | 0..63 |
| `sliderL2`   | 0x07 | 0x55 | PAL2 (Level 2)                                   | 0..63 |
| `sliderL3`   | 0x07 | 0x56 | PAL3 (Level 3)                                   | 0..63 |
| `sliderSlope`| 0x07 | 0x57 | PARS (Amp EG Rate Scaling)                       | -7..+7 |

Element offset (b4) формула: `0x00`/`0x20`/`0x40`/`0x60` для E1..E4 — применяется во всех восьми слайдерах.

Не сделано (TODO):

1. **sliderL0 / sliderL1 / sliderL4 / sliderRL1 / sliderRL2** остаются на legacy `b3=0x00` — manual не содержит NN для этих уровней Amp EG (в sy99_sysex_complete.md перечислены только PAL2 (0x55) и PAL3 (0x56)). Возможные кандидаты: PABP1..4 (`0x58..0x5B`) — но это Break Points (output level scaling), а не EG levels. Оставлено как **❓** до подтверждения с железа.
2. **Hook drag (Segment::sendOsc)** всё ещё использует второй (legacy) шаблон, переданный в `addSegment`. Сами слайдеры (после исправления) шлют правильные адреса; перетаскивание точки графика — нет. Отдельный TODO.
3. **Дубликаты `sliderAwmR2E1` / `sliderAwmR2E3`** теперь функционально повторяют `sliderR2` (с тем же `0x07`/`0x51` для E3) — оставлены, потому что у E1 PARI имеет неклассический диапазон (0..62). Унификация — отдельный milestone.
4. **PARS encoding** (-7..+7 → байт) — UI пишет сырое значение через MidiSlider; вероятно two's-complement (как ATPBR), но без hardware-теста не подтверждено.

### Обновление статуса CSV строк

Строки в `05_missing_audit.md` секция «Group 0x07 — AWM Element Data (E1)» теперь имеют реализацию для PARI/PAR2/PAR3/PAR4/PARR1/PAL2/PAL3/PARS. См. также `03_parameter_map.csv`.

---

## Library ↔ Voice sync ([LIBSYNC])

Этот блок соответствует патчам **`sysex77_library_patch_sync_increment.patch`** и **`sysex77_full_with_library_sync.patch`** (одинаковые изменения по смыслу; в коде они уже учтены поверх актуальной `Librairie.h`/DUMP-имен).

> Историческое имя в патч-тексте — «Prompt #9 / LIBSYNC». В **`06_agent_prompts.md`** номер **#9 занят промптом Pan.h** — при чтении старых диффов сверять по имени файла патча или по тегу `[LIBSYNC]` в коде.

| Поведение | Где | Статус |
|-----------|-----|--------|
| Парсинг `.syx`: кадры `F0…F7`, имена из `frameStart + 33` (10 байт), printable ASCII → пробел | `BankTableModel::changeListenerCallback` + `voiceSysexFileOffsets` / `voiceSysexFileLengths` | ✅ |
| Имена в Bank A–D | `arrayListVoices` + `paintListBoxItem` | ✅ |
| Слот 0…63, клик по строке | `selectLibraryVoiceSlot` в `VoicesTableModel.h` | ✅ |
| Отправка одного кадра | OSC `/77SendVoice` → `MidiSysex.h` → `sendRaw` по offset/length | ✅ |
| SEND VOICE | `Librairie.h` (`btSendVoice`; раскладка кнопок шире, чем в минимальном патче) | ✅ |
| Автовыбор банка после `loadBankRequest` | Приоритет: **`UNNAMED.SYX`** → затем **`DUMP-*`**.syx с максимальным `LastModificationTime` → иначе первая строка | ✅ |
| Имя в `VoicePage::editName` без исходящего VNAM | `bankSelectedVoiceNameValue` + `dontSendNotification` | ✅ |

**Ограничения**

- Из bulk не поднимаются слайдеры — только синхронизация имени; полный распаковщик SY99 — отдельная задача.
- Смещение имени **`+33`** от `F0` эмпирическое; без гарантии для чужих форматов дампов.
- Таблицы offset/length — глобальные `static` в `MidiDemo.h`, живут только для **последнего успешного парсинга** выбранного банка.
- `/77SendVoice`: один непрерывный `sendRaw` кадра, без чанкования (как и полный `/77SendBank`).

---

## Приоритеты

1. **Исправить отправку имени голоса (`VNAM*`)** — ветка **`oscSendMidiMessage`** в `MidiSysex.h` помечена в `01_code_map.md` как логически сломанная; без этого строки CSV **VNAM0…9** остаются ⚠️/❌ по факту.

2. **Режим элементов (`ELMODE`)** — добавить приём с синта (**`receive_implemented`**) или явную синхронизацию через ValueTree, и сверить **11-й** режим SY99 (**DRUM_SET** и т.д.) с combo **0…9**.

3. **Voice Common с адресами `02 00 00 28`…`43`** (модуляция, границы громкости, micro tune, zoned AT…) — целиком **❌** в UI; для паритета с сервис-мануалом имеет смысл либо вкладка Common, либо отдельный аудит «не дублировать уже реализованное в других экранах».

4. **Не смешивать логи Pan и AWM** при аудите адресов: держать опору **`0A 00 00 03…08`** для Pan EG Rate и отдельно документировать **`07 … 50`** / **`07 … 51`** как AWM (`05_missing_audit` / `MIDI_MAP_OBSERVATIONS.md`).

5. **WaveEg Hook drag** — сегменты графа всё ещё используют второй (legacy) SysEx-шаблон в `addSegment`; после Prompt #12 слайдеры шлют `b3=0x07`, перетаскивание крючков — нет (см. секцию Library & WaveEg).

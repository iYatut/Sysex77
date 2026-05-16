# YAMAHA SY99 — ПОЛНЫЙ СПРАВОЧНИК MIDI SysEx (из сервисного мануала)

## Формат Parameter Change

```
F0 43 1n 34 GG T2 00 NN 0V VV F7
```

| Байт | Значение |
|------|---------|
| F0   | SysEx start |
| 43   | Yamaha |
| 1n   | 10 + MIDI channel (0-15), обычно 10 = ch1 |
| 34   | Model SY99 |
| GG   | Group (тип данных) |
| T2   | Element/Memory selector |
| 00   | всегда 0 |
| NN   | Parameter number (N2 в мануале) |
| 0V   | старший бит значения (обычно 00) |
| VV   | значение параметра (7 бит) |
| F7   | SysEx end |

---

## Таблица групп (GG байт)

| GG (hex) | Группа | Формат |
|----------|--------|--------|
| 00 | Multi Common Data | F0 43 1n 34 00 00 00 NN 00 VV F7 |
| 01 | Multi Channel Data | F0 43 1n 34 01 ch 00 NN 00 VV F7 |
| 02 | Voice Common Data | F0 43 1n 34 02 00 00 NN 00 VV F7 |
| 03 | Voice Element Data (Normal) | F0 43 1n 34 03 EE 00 NN 00 VV F7 |
| 04 | Drum Set Data | F0 43 1n 34 04 note 00 NN V1 VV F7 |
| 05 | AFM Element Common | F0 43 1n 34 05 EE 00 NN 00 VV F7 |
| 05 7F 7F | AFM Operator Enable | F0 43 1n 34 05 EE 7F 7F 00 VV F7 |
| 06-56 | AFM Operator Data | F0 43 1n 34 OP EE 00 NN V1 VV F7 |
| 07 | AWM Element Data | F0 43 1n 34 07 EE 00 NN V1 VV F7 |
| 08 | Effect Data | F0 43 1n 34 08 00 00 NN 00 VV F7 |
| 09 | Filter Data | F0 43 1n 34 09 EE 00 NN V1 VV F7 |
| 0A | PAN Data (Dynamic PAN EG) | F0 43 1n 34 0A MM 00 NN 00 VV F7 |
| 0B | Micro Tuning Data | F0 43 1n 34 0B MM p NN V1 VV F7 |
| 0D | Master Control Data | F0 43 1n 34 0D 00 00 NN 00 VV F7 |
| 0E | Waveform Data | F0 43 1n 34 0E ww 05 NN 00 VV F7 |
| 0F | System Setup Data | F0 43 1n 34 0F 00 00 NN 00 VV F7 |
| 56 | (AFM Operator 6 last) | F0 43 1n 34 56 EE 00 NN V1 VV F7 |

**T2 / Element Number (EE):**
- $00 = Element 1
- $20 = Element 2
- $40 = Element 3
- $60 = Element 4

**AFM Operator (T1 byte для группы operator):**
- $06 = OP1, $16 = OP2, $26 = OP3, $36 = OP4, $46 = OP5, $56 = OP6

---

## Group 02 — Voice Common Data

`F0 43 1n 34 02 00 00 NN 00 VV F7`

| NN (hex) | Имя | Диапазон | Описание |
|----------|-----|----------|---------|
| 00 | ELMODE | 0-10 | Element Mode (0=1AFM_mono…10=DRUM_SET) |
| 01-0A | VNAM0-9 | ascii | Voice Name (10 chars) |
| 28 | WPBR | 0-12 | Wheel Pitch Bend Range |
| 29 | ATPBR | -12..+12 | AfterTouch Pitch Bend Range |
| 2A | PMASN | 0-121 | Pitch Mod Device Assign |
| 2B | PMRNG | 0-127 | Pitch Mod Range |
| 2C | AMASN | 0-121 | Amplitude Mod Device Assign |
| 2D | AMRNG | 0-127 | Amplitude Mod Range |
| 2E | FMASN | 0-121 | Filter Mod Device Assign |
| 2F | FMRNG | 0-127 | Filter Mod Range |
| 30 | PNLASN | 0-121 | PAN Level Device Assign |
| 31 | PNLRNG | 0-127 | PAN Level Range |
| 32 | COASN | 0-121 | Filter Cutoff Bias Device Assign |
| 33 | CORNG | 0-127 | Filter Cutoff Range |
| 34 | PNBASN | 0-121 | PAN Bias Device Assign |
| 35 | PNBRNG | 0-127 | PAN Bias Range |
| 36 | EGBASN | 0-121 | EG Bias Device Assign |
| 37 | EGBRNG | 0-127 | EG Bias Range |
| 38 | WLASN | 0-121 | Voice Volume Device Assign |
| 39 | WLLML | 0-127 | Volume Limit Low |
| 3A | MCTUN | 0-65 | Micro Tuning table select |
| 3B | RNDP | 0-7 | Random Pitch fluctuation |
| 3F | WOL | - | Wave Volume |
| 42 | AFTMD | all/top/btm/hi/low | Zoned AfterTouch mode |
| 43 | SPTPNT | 0-127 | Zoned AfterTouch split point |

---

## Group 03 — Normal Voice Element Data

`F0 43 1n 34 03 EE 00 NN 00 VV F7`

| NN (hex) | Имя | Диапазон | Описание |
|----------|-----|----------|---------|
| 00 | ELVL | 0-127 | Element Level |
| 01 | ELDT | -7..+7 | Element Detune |
| 02 | ELNS | -64..+63 | Element Note Shift |
| 03 | ENLL | 0-127 | Element Note Limit Low |
| 04 | ENLH | 0-127 | Element Note Limit High |
| 05 | EVLL | 0-127 | Element Velocity Limit Low |
| 06 | EVLH | 0-127 | Element Velocity Limit High |
| 07 | PANNM | 0-31 | PAN data set table select |
| 08 | MCTEN/OUTOSEL/OUT1SEL | bits | Micro Tuning + Output select |
| 09 | - | - | (reserved) |
| 0A | EFLN1EL | bits | Effect send lines 1-4 select |
| 0B | EFSDLV | 0-127 | Effect send level |
| 0C | EFSDVL | -7..+7 | Effect send velocity sense |

---

## Group 05 — AFM Element Common

`F0 43 1n 34 05 EE 00 NN 00 VV F7`

| NN (hex) | Имя | Диапазон | Описание |
|----------|-----|----------|---------|
| 00 | ALGNUM | 0-44 | Algorithm number |
| 01 | FPR1 | 0-63 | Pitch EG Rate 1 |
| 02 | FPR2 | 0-63 | Pitch EG Rate 2 |
| 03 | FPR3 | 0-63 | Pitch EG Rate 3 |
| 04 | FPRR1 | 0-63 | Pitch EG key off Rate 1 |
| 05 | FPL0 | -64..+63 | Pitch EG Level 0 |
| 06 | FPL1 | -64..+63 | Pitch EG Level 1 |
| 07 | FPL2 | -64..+63 | Pitch EG Level 2 |
| 08 | FPL3 | -64..+63 | Pitch EG Level 3 |
| 09 | FPRL1 | -64..+63 | Pitch EG key off Level 1 |
| 0A | FPEGR | 0-3 | Pitch EG Range |
| 0B | FPRS | 0-7 | Pitch EG Rate Scaling |
| 0C | FVPSW | off/on | Velocity Switch |
| 0D | FLFSPD | 0-99 | Main LFO Speed |
| 0E | FLFDLY | 0-99 | Main LFO Delay |
| 0F | FLFPMD | 0-127 | Main LFO Pitch Mod Depth |
| 10 | FLFAMD | 0-127 | Main LFO Amp Mod Depth |
| 11 | FLFFMD | 0-127 | Main LFO Filter Mod Depth |
| 12 | FLFWAV | 0-5 | Main LFO Wave |
| 13 | FLINTP | 0-99 | Main LFO Initial Phase |
| 15 | SLFWD | 0-3 | Sub LFO Wave |
| 16 | SLFS | 0-127 | Sub LFO Speed |
| 17 | SLFDM | - | Sub LFO delay/decay mode |
| 18 | SLFDT | 0-127 | Sub LFO Delay/Decay time |
| 19 | SLPMD | - | Sub LFO Pitch Mod Depth |

---

## Group 07 — AWM Element Data

`F0 43 1n 34 07 EE 00 NN V1 VV F7`

| NN (hex) | Имя | Диапазон | Описание |
|----------|-----|----------|---------|
| 00 | WSOURCE | 0-4 | AWM Wave Source |
| 01 | AWMWAVE | 0-255 (2 bytes) | AWM Wave number |
| 02 | PPM | normal/fixed | Frequency Mode |
| 03 | PNOTE | 0-127 | Fixed mode note # |
| 04 | PPF | -64..+63 | Frequency Fine |
| 05 | PMLPMS | 0-7 | Pitch Mod Sensitivity |
| 06 | PPR1 | 0-63 | AWM Pitch EG Rate 1 |
| 07 | PPR2 | 0-63 | AWM Pitch EG Rate 2 |
| 08 | PPR3 | 0-63 | AWM Pitch EG Rate 3 |
| 09 | PPRR1 | 0-63 | AWM Pitch EG key off Rate 1 |
| 0A | PPL0 | -64..+63 | AWM Pitch EG Level 0 |
| 0B | PPL1 | -64..+63 | AWM Pitch EG Level 1 |
| 0C | PPL2 | -64..+63 | AWM Pitch EG Level 2 |
| 0D | PPL3 | -64..+63 | AWM Pitch EG Level 3 |
| 0E | PPRL1 | -64..+63 | AWM Pitch EG key off Level 1 |
| 0F | PPEGR | 1-3 | AWM Pitch EG Range |
| 10 | PPRS | -7..+7 | AWM Pitch EG Rate Scaling |
| 11 | PVPSW | off/on | AWM Velocity Switch |
| 12 | PLFSPD | 0-99 | AWM LFO Speed |
| 13 | PLFDLY | 0-99 | AWM LFO Delay |
| 14 | PLFPMD | 0-127 | AWM LFO Pitch Mod Depth |
| 15 | PLFAMD | 0-127 | AWM LFO Amp Mod Depth |
| 16 | PLFFMD | 0-127 | AWM LFO Filter Mod Depth |
| 17 | PLFWAV | 0-5 | AWM LFO Wave |
| 18 | PLINTP | 0-99 | AWM LFO Initial Phase |
| 4F | PAEGMD | normal/hold | Amplitude EG mode |
| 50 | PARI | 0-63 | Amp EG Attack Rate |
| 51 | PAR2 | 0-63 | Amp EG Decay Rate |
| 52 | PAR3 | 0-63 | Amp EG Rate 3 |
| 53 | PAR4 | 0-63 | Amp EG Rate 4 |
| 54 | PARR1 | 0-63 | Amp EG key off Rate 1 |
| 55 | PAL2 | 0-63 | Amp EG Level 2 |
| 56 | PAL3 | 0-63 | Amp EG Level 3 |
| 57 | PARS | -7..+7 | Amp EG Rate Scaling |
| 58-5B | PABP1-4 | 0-127 | Output Level Scaling Break Points |
| 5C-5F | PAOS21-4 | -128..+127 | Output Level Scaling Offsets |
| 60 | PAVSON | -7..+7 | Velocity Sensitivity |
| 61 | PARVSW | off/on | Attack Rate Velocity Switch |
| 62 | PAMS | -7..+7 | Amplitude Mod Sensitivity |

---

## Group 08 — Effect Data

`F0 43 1n 34 08 00 00 NN 00 VV F7`

| NN (hex) | Имя | Диапазон | Описание |
|----------|-----|----------|---------|
| 20 | EFMODE | 0-2 | Effect mode (off/serial/parallel) |
| 21 | EF1TYPE | 0-60 | Effect 1 type |
| 22-2B | EF1PRM1-10 | varies | Effect 1 parameters 1-10 |
| 2C | EF1OUTLV1 | 0-100 | Effect 1 output level 1 |
| 2D | EF1OUTLV2 | 0-100 | Effect 1 output level 2 |
| 2E | EF2TYPE | 0-60 | Effect 2 type |
| 2F-38 | EF2PRM1-10 | varies | Effect 2 parameters 1-10 |
| 39 | EF2EFBAL1 | 0-100 | Effect 2 mix level |
| 3A | EF2OUTLV1 | 0-100 | Effect 2 output level 1 |
| 3B | EF2OUTLV2 | 0-100 | Effect 2 output level 2 |
| 3C | OUT1EFBAL | 0-100 | Output 1 effect balance |
| 3D | OUT2EFBAL | 0-100 | Output 2 effect balance |
| 3E | CTRL1PRM | 0-32 | Controller 1 parameter |
| 3F | CTRL1ASN | 0-120 | Controller 1 device assign |
| 40 | CTRL1MIN | 0-99 | Controller 1 MIN |
| 41 | CTRL1MAX | 0-99 | Controller 1 MAX |
| 42 | CTRL2PRM | 0-32 | Controller 2 parameter |
| 43 | CTRL2ASN | 0-120 | Controller 2 device assign |
| 44 | CTRL2MIN | 0-99 | Controller 2 MIN |
| 45 | CTRL2MAX | 0-99 | Controller 2 MAX |
| 46 | EFLFOWV | tri/dwn/up/squ/sin/S-H | Effect LFO wave |
| 47 | EFLFOSP | 0-99 | Effect LFO speed |
| 48 | ER50DL | 0-99 | Effect LFO delay |
| 49 | EFLFOPH | 0-99 | Effect LFO initial phase |

---

## Group 09 — Filter Data

`F0 43 1n 34 09 EE 00 NN V1 VV F7`

T2 element selector same as groups above.
Filter number encoded in T2 bits (000=AFM filt1, 001=AFM filt2, 010=AFM common, 100=AWM filt1, 101=AWM filt2, 110=AWM common)

| NN (hex) | Имя | Диапазон | Описание |
|----------|-----|----------|---------|
| 00 | FTYPE | thru/LPF/HPF | Filter type |
| 01 | FCTOF | 0-127 | Cutoff frequency |
| 02 | FMODE | EG/LFO/EG-VA | Filter mode |
| 03 | FR1 | 0-63 | Filter EG Rate 1 |
| 04 | FR2 | 0-63 | Filter EG Rate 2 |
| 05 | FR3 | 0-63 | Filter EG Rate 3 |
| 06 | FR4 | 0-63 | Filter EG Rate 4 |
| 07 | FRR1 | 0-63 | Filter EG key off Rate 1 |
| 08 | FRR2 | 0-63 | Filter EG key off Rate 2 |
| 09 | FL0 | -64..+63 | Filter EG Level 0 |
| 0A | FL1 | -64..+63 | Filter EG Level 1 |
| 0B | FL2 | -64..+63 | Filter EG Level 2 |
| 0C | FL3 | -64..+63 | Filter EG Level 3 |
| 0D | FL4 | -64..+63 | Filter EG Level 4 |
| 0E | FRL1 | -64..+63 | Filter EG key off Level 1 |
| 0F | FRL2 | -64..+63 | Filter EG key off Level 2 |
| 10 | FRS | -7..+7 | Rate Scaling |
| 11-14 | FBP1-4 | 0-127 | Cutoff Level Scaling Break Points |
| 15-18 | FOS1-4 | -128..+127 | Cutoff Level Scaling Offsets |
| 20 | FRES | 0-99 | Resonance |
| 21 | FVSON | -7..+7 | Velocity Sensitivity |
| 22 | FCMS | -7..+7 | Cutoff Mod Sensitivity |

---

## Group 0A — PAN Data (Dynamic PAN EG)  ← ЭТО ТВОИ ДАННЫЕ ИЗ ЛОГОВ!

`F0 43 1n 34 0A MM 00 NN 00 VV F7`

MM = Memory number (PAN table number)

| NN (hex) | Имя | Диапазон | Описание |
|----------|-----|----------|---------|
| 00 | PNSCSEL | velocity/note#/LFO | PAN source select |
| 01 | PNSCDPT | 0-99 | PAN source depth |
| 02 | PNDT | 0-63 | EG key on/Hold Time |
| 03 | PNR1 | 0-63 | **PAN EG Rate 1** |
| 04 | PNR2 | 0-63 | **PAN EG Rate 2** |
| 05 | PNR3 | 0-63 | **PAN EG Rate 3** |
| 06 | PNR4 | 0-63 | **PAN EG Rate 4** |
| 07 | PNRR1 | 0-63 | PAN EG key off Rate 1 |
| 08 | PNRR2 | 0-63 | PAN EG key off Rate 2 |
| 09 | PNL0 | -32..+31 | **PAN EG Level 0** |
| 0A | PNL1 | -32..+31 | **PAN EG Level 1** |
| 0B | PNL2 | -32..+31 | **PAN EG Level 2** |
| 0C | PNL3 | -32..+31 | **PAN EG Level 3** |
| 0D | PNL4 | -32..+31 | **PAN EG Level 4** |
| 0E | PNRL1 | -32..+31 | PAN EG key off Level 1 |
| 0F | PNRL2 | -32..+31 | PAN EG key off Level 2 |
| 10 | PNSLP | 0-3 | repeat segment |
| 11-1A | PNNAM0-9 | ascii | Dynamic PAN Name (10 chars) |

### Соответствие данных из analiz-midi.txt:

Сообщения вида `F0 43 10 34 0A 00 00 NN 00 VV F7`:
- `0A 00 00 03` = PNR1 (PAN EG Rate 1), значения 0x00-0x3F
- `0A 00 00 04` = PNR2 (PAN EG Rate 2)
- `0A 00 00 05` = PNR3 (PAN EG Rate 3)  
- `0A 00 00 06` = PNR4 (PAN EG Rate 4)
- `0A 00 00 09` = PNL0 (PAN EG Level 0)
- `0A 00 00 0A` = PNL1 (PAN EG Level 1)
- `0A 00 00 0B` = PNL2 (PAN EG Level 2)
- `0A 00 00 0C` = PNL3 (PAN EG Level 3)
- `0A 00 00 0D` = PNL4 (PAN EG Level 4)

---

## Что означает байт 07 00 00 50 в логах

Из анализа лога `analiz-midi.txt`:
- `F0 43 10 34 07 00 00 50 00 3E F7` — группа **07 (AWM Element Data)**, Element 1 (T2=00), NN=50 hex = PAVSON (Velocity Sensitivity, range -7..+7)
- `F0 43 10 34 07 00 00 06 VV` — группа 07, NN=06 = **PPR1** (AWM Pitch EG Rate 1, 0-63)
- `F0 43 10 34 07 40 00 06 VV` — группа 07, Element 3 (T2=40), NN=06 = PPR1 Element 3
- `F0 43 10 34 07 40 00 17 VV` — группа 07, Element 3, NN=17 = **PLFWAV** (LFO Wave)

## Что означает байт 03 xx 00 07 в логах

- `F0 43 10 34 03 00 00 07 VV` — группа **03 (Normal Voice Element)**, Element 1, NN=07 = **PANNM** (PAN table select, 0-31)
- `F0 43 10 34 03 20 00 07 VV` — то же, Element 2
- `F0 43 10 34 03 40 00 07 VV` — то же, Element 3

## Что означает 56 00 00 1B в логах

- `F0 43 10 34 56 00 00 1B VV` — группа **56** (AFM Operator 6), Element 1, NN=1B

---

## Источник документа

Yamaha SY99 Service Manual, стр. 112-125, раздел "MIDI Data Format", таблицы 1-3 — 1-16.
Скачан с: https://archive.org/download/Yamaha_SY99_Service_Manual/Yamaha_SY99_Service_Manual_text.pdf

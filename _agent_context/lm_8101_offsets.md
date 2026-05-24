# LM 8101VC / 0040VC — карта смещений (SY99 single voice dump)

**Этап 0–1:** эталонные дампы в [`fixtures/`](fixtures/) · regression: [`fixtures/_validate_bulk_parse.py`](fixtures/_validate_bulk_parse.py) · parser 0040: [`fixtures/_parse_0040vc.py`](fixtures/_parse_0040vc.py).

**Связанные источники:**

| Документ | Содержание |
|----------|------------|
| [`sy99_sysex_complete.md`](sy99_sysex_complete.md) | Логические параметры (GG/NN) для parameter change `F0 43 1n 34 …` |
| [`04_observed_logs.md`](04_observed_logs.md) | Подтверждённые live-адреса (ELMODE, ELVL, OUTSEL…) |
| [`fixtures/lcd_reference.csv`](fixtures/lcd_reference.csv) | LCD-эталон fixtures 01–03 |

---

## Формат захвата (07:1 Voice)

Один голос = **ровно 2** SysEx-кадра подряд:

| # | Маркер | Размер `F0…F7` | Роль |
|---|--------|----------------|------|
| 1 | `LM` + `8101VC` | 475–586 B | Основной блок (common header + packed mixer/EG) |
| 2 | `LM` + `0040VC` | 113–115 B | Хвост Voice Common (group `02` tail) |

---

## Заголовок кадра (8101VC и 0040VC)

| Offset | Size | Hex / ASCII | Подтверждено | Примечание |
|--------|------|-------------|--------------|------------|
| 0 | 1 | `F0` | ✅ | SysEx start |
| 10 | 6 | `8101VC` / `0040VC` | ✅ | Тип блока |
| 32 | 1 | ELMODE raw | ✅ | 0…10 = parameter change `02 00 00 00` |
| 33 | 10 | VNAM ASCII | ✅ | Voice name |

---

## Блок 8101VC — подтверждённые поля

| Offset / паттерн | Параметр | Parameter change | Подтверждено | Метод |
|------------------|----------|------------------|--------------|-------|
| +32 | ELMODE | `02 00 00 00` | ✅ | fixtures 01–03 |
| +33 | VNAM | `02 00 00 01…0A` | ✅ | fixtures |
| `03`/`01` @ i−1, WOL @ i, ELVL E1 @ i+3 | WOL / ELVL E1 | `02…3F` / `03…00` | ✅ | WOL/EL1 pattern |
| ELVL E1 + 1 | OUTSEL E1 | `03 00 00 08` | ✅ | diff 04→05 @+98; `& 0x06` |
| outsel + 1 | ELNS E1 | `03 00 00 02` | ✅ | sysex VV=UI+64; fixtures 01–03 |
| outsel + 2 | ENLL E1 | `03 00 00 03` | ✅ | fixtures 01–03 (=0); AUTOSYNC ANONIM (=37 C#1) |
| outsel + 3 | ENLH E1 | `03 00 00 04` | ✅ | fixtures 01–03 (=127); AUTOSYNC ANONIM (=97 C#6) |
| outsel + 4 | EVLL E1 | `03 00 00 05` | ✅ | fixtures 01–03 (=1) |
| outsel + 5 | EVLH E1 | `03 00 00 06` | ✅ | fixtures 01–03 (=127); was misread @+3 before ENLL fix |
| outsel + 0 | ELDT E1 | `03 00 00 01` | ✅ elmode **1,4,6,8** | fixture 02 E1=−1 (sysex 9); fixture 03 (+2) |
| first `7F 01 7F` + 4 | OUTSEL E2 | `03 20 00 08` | ✅ | fixtures 01–03 (`& 0x06`) |
| anchor + 5 | ELVL E2 | `03 20 00 00` | ✅ | fixtures 01–03 |
| anchor + 6 | ELDT E2 | `03 20 00 01` | ✅ elmode **4,8** | fixture 03 E2=−2 (sysex 10); fixtures 01, 02 |
| anchor + 7 | ELNS E2 | `03 20 00 02` | ✅ | sysex VV=64 (=UI 0) |
| anchor + 8 | ENLL E2 | `03 20 00 03` | ✅ | fixtures 01–03 (=0) |
| anchor + 9 | ENLH E2 | `03 20 00 04` | ✅ | fixtures 01–03 (=127) |
| anchor + 10 | EVLL E2 | `03 20 00 05` | ✅ | fixtures 01–03 (=1) |
| anchor + 11 | EVLH E2 | `03 20 00 06` | ✅ | fixtures 01–03 (=127) |
| ELVL E1 + Δ | EFLN1EL El.1 | live `03 00 00 09` | ✅ bulk | Δ=36 (wol@94), 37 (wol@93), 35 (wol@95); EP=0x05 |
| efln + 12 | EFSDLV El.1 | live `03 00 00 0A` | ⚠️ bulk **8101 fallback only** | Classic: совпадает с 0040@100; **NiteHwk HW: 76 poison vs LCD 127** — authoritative **0040 @100** |
| efln + 1..3 | EFSDVSNS / EFSDSCL | live `0B` / `0C` | ❌ bulk | not in 8101VC; live param9 only (fixture 02 falsifies efln+2/+3) |
| ? | EFMODE | live `08 00 00 20` | ✅ **0040 @+33** | fixtures 06–08: 0/1/2 = Off/Serial/Parallel |

**anchor** = первый `7F 01 7F` после ELVL E1 (E2 strip). **outsel** = байт @ ELVL E1 + 1.

---

## Блок 0040VC — group 02 tail

Primary block @36–56 (дубликат @57–76 игнорировать). Tail @90–101.

| Offset | Параметр | NN | Подтверждено | Значение fixtures 01–03 |
|--------|----------|-----|--------------|-------------------------|
| **33** | **EFMODE** | live 0x20 | **✅** | fixtures 06–08: 0/1/2 |
| **41** | **WPBR** | 0x28 | **✅** | ANONIM=2; 02/03=0 |
| **55** | **WLLML** | 0x39 | **✅** | 100 (0x64) все |
| **98** | **SPTPNT** | 0x43 | **✅** | 60 (0x3C) все |
| 42 | ATPBR | 0x29 | **✅** | sysex raw @+42; fixtures 01–03 vary (63/99/8) |
| 40 | PMRNG | 0x2B | **✅** | fixtures 01–03 |
| 44 | PMASN | 0x2A | **✅** | fixtures 01–03 |
| 48 | FMRNG / AMASN | 0x2F / 0x2C | **✅** | shared byte @48 |
| 50 | PNLRNG / AMRNG | 0x31 / 0x2D | **✅** | shared byte @50 |
| 52 | CORNG | 0x33 | **✅** | fixtures 01–03 |
| 54 | PNBRNG / EGBRNG | 0x35 / 0x37 | **✅** | shared byte @54 |
| **100** | **EFSDLV El.1** | live `03 00 00 0A` | **✅ elmode 8** | diff step A/B′: direct 7-bit (127/126/100) |
| **104** | **EFSDLV El.2** | live `03 20 00 0A` | **✅ elmode 8** | diff step A/B′: direct 7-bit (127/100/14) |
| ~~100~~ | ~~AFTMD~~ | live `02 00 00 42` | ❌ **не bulk @100** | было ложное совпадение (127 на fixtures = EFSDLV E1) |

**Live parameter change (`F0 43 1n 34 …`) остаётся authoritative** для полей 0040 без ✅ при Stop sync, если synth шлёт param9.

---

## Эталонные файлы

| Файл | Кадры | Назначение |
|------|-------|------------|
| [`fixtures/01_init_anonim_07x1_voice.syx`](fixtures/01_init_anonim_07x1_voice.syx) | 586+115 | INIT / ANONIM |
| [`fixtures/02_ap_crsrock_07x1_voice.syx`](fixtures/02_ap_crsrock_07x1_voice.syx) | 585+115 | det E1/E2 −1/+2 |
| [`fixtures/03_ep_classic_07x1_voice.syx`](fixtures/03_ep_classic_07x1_voice.syx) | 475+115 | elmode 4; FX send |
| [`fixtures/04_anonim_outsel_both.syx`](fixtures/04_anonim_outsel_both.syx) | OUTSEL E1 @+98 | diff both |
| [`fixtures/05_anonim_outsel_g1.syx`](fixtures/05_anonim_outsel_g1.syx) | OUTSEL E1 @+98 | diff G1 |
| [`fixtures/06_efmode_off_ep_grndual.syx`](fixtures/06_efmode_off_ep_grndual.syx) | 0040 @+33=0 | EFMODE Off |
| [`fixtures/07_efmode_serial_ep_grndual.syx`](fixtures/07_efmode_serial_ep_grndual.syx) | 0040 @+33=1 | EFMODE Serial |
| [`fixtures/08_efmode_parallel_ep_grndual.syx`](fixtures/08_efmode_parallel_ep_grndual.syx) | 0040 @+33=2 | EFMODE Parallel |

---

*Последнее обновление: 2026-05-24. EFSDLV elmode 8 → 0040 @100/@104 (hardware diff). Regression: `_validate_bulk_parse.py` + `_parse_0040vc.py`.*

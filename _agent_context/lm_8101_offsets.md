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
| outsel + 3 | EVLH E1 | `03 00 00 06` | ✅ | fixtures 01–03 (=127) |
| outsel + 4 | EVLL E1 | `03 00 00 05` | ✅ | fixtures 01–03 (=1) |
| outsel + 0 | ELDT E1 | `03 00 00 01` | ✅ elmode **4** | fixture 03 (+2) |
| outsel + 0 | ELDT E1 | `03 00 00 01` | ❌ elmode **8** | fixture 02 E1=−1 — нужен diff |
| first `7F 01 7F` + 4 | OUTSEL E2 | `03 20 00 08` | ✅ | fixtures 01–03 (`& 0x06`) |
| anchor + 5 | ELVL E2 | `03 20 00 00` | ✅ | fixtures 01–03 |
| anchor + 6 | ELDT E2 | `03 20 00 01` | ✅ elmode **8** | fixtures 01, 02 |
| anchor + 6 | ELDT E2 | `03 20 00 01` | ❌ elmode **4** | fixture 03 — нужен diff |
| anchor + 7 | ELNS E2 | `03 20 00 02` | ✅ | sysex VV=64 (=UI 0) |
| ELVL E1 + ? | ENLL / ENLH E1 | `03 00 00 03/04` | ❌ | нет raw 36/103 в fixtures 01–03 |
| ELVL E1 + Δ | EFLN1EL El.1 | live `03 00 00 09` | ✅ bulk | Δ=36 (wol@94), 37 (wol@93), 35 (wol@95); EP=0x05 |
| efln + 1 | EFSDLV El.1 | live `03 00 00 0A` | ⚠ candidate | fixture 03 bulk=0 vs LCD lvl 127 — нужен diff |
| efln + 2 | EFSDVSNS El.1 | live `03 00 00 0B` | ⚠ candidate | signed7 ladder; bulk offset unconfirmed |
| efln + 3 | EFSDSCL El.1 | live `03 00 00 0C` | ⚠ candidate | signed7 ladder; bulk offset unconfirmed |
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
| 42 | ATPBR | 0x29 | ⚠ candidate | sysex raw; нужен diff |
| 40 | PMRNG | 0x2B | ⚠ candidate | |
| 44 | PMASN | 0x2A | ⚠ candidate | |
| 48 | FMRNG / AMASN | 0x2F / 0x2C | ⚠ candidate | |
| 50 | PNLRNG / AMRNG | 0x31 / 0x2D | ⚠ candidate | |
| 52 | CORNG | 0x33 | ⚠ candidate | |
| 54 | PNBRNG / EGBRNG | 0x35 / 0x37 | ⚠ candidate | |
| 100 | AFTMD | 0x42 | ⚠ candidate | raw 127 — нужен diff |

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

*Последнее обновление: 2026-05-21. Regression: `_validate_bulk_parse.py` + `_validate_efmode_bulk.py`.*

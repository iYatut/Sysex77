# SYM7 — зафиксированные TX-запросы (SY99, 2026-05-23)

Источник: MIDI-OX Monitor, SY Manager при connect + library sync + patch invoke.

**Формат SYM7 (verified):** 31 байт, zero-pad, **mm только в byte 29**, byte 28 = bank/preset (`00` internal).

```text
F0 43 20 7A 4C 4D 20 20 [TYPE6] 00 00 00 00
00 00 00 00 00 00 00 00 00 00 [B28] [MM] F7
```

| Offset | Поле |
|--------|------|
| 2 | `0x20` = bulk request, device 0 |
| 10–15 | LM type: `8101VC`, `0040VC`, `8101SY`, `0040MU`, … |
| 28 | byte28 (preset flag: `00` internal, `02` PRE1, `03` PRE2) |
| 29 | mm slot `00`…`3F` |

**Не использовать** для SYM7-parity: variant A/B (`mt/mm/checksum` tail) — SY99 не отвечает.

---

## 1. Connect — преамбула (опционально)

| # | TX | mm | Δ ms |
|---|-----|-----|------|
| 1 | `8101VC` | `00` | — |
| 2 | `0040MU` | `00` | ~172 |

Текущий голос A1 + Multi slot 0 (до полного handshake).

---

## 2. Connect — handshake

| # | TX | Примечание |
|---|-----|------------|
| 1 | `8101SY` | System setup |
| 2 | `F0 43 10 34 0F 00 00 34 00 00 F7` | param9 (utility) |
| 3 | `F0 43 10 34 0F 00 00 31 00 01 F7` | param9 |
| 4 | `0040MU` | Multi header |

Sysex77 **не шлёт** param9 (Bulk Protect — вручную на SY99).

---

## 3. Full library sync — internal voices

После handshake (и других фаз, не в коротком логе):

| Фаза | TX | Count | mm | TX→TX ms |
|------|-----|-------|-----|----------|
| A | `8101VC` b28=`00` | 64 | `00`…`3F` | ~258 |
| B | **`0040VC`** b28=`00` | **64** | **`00`…`3F`** | **~140–550** (тип. ~350) |

**Пара слота N:** `8101VC mm=N` + `0040VC mm=N` → полный голос (~700 B).  
EFSDLV elmode 4/8 — только из **0040VC** (@100/@104).

Дальше (extended sync) — см. §8 и `06_extended_0040mu_pn_mt_parsed.md`.

---

## 8. Extended sync — «другие банки» (verified 2026-05-24)

После Internal (8101+0040VC уже загружены), SYM7 шлёт:

| # | TX | ×N | mm | b28 | Δ ms |
|---|-----|-----|-----|-----|------|
| 1 | **`0040VC`** | 64 | `00`…`3F` | `00` | ~340 |
| 2 | **`0040MU`** | **16** | `00`…`0F` | `00` | ~325 |
| 3 | **`8101PN`** | 32 | `00`…`1F` | `00` | ~100 |
| 4 | **`8101MT`** | 2 | `00`, `01` | `00` | ~166 |

**Не в этом логе:** `8101VC` PRE1/PRE2 (`b28=02/03`) — пресеты, видимо, отдельная галочка или другой прогон.

**Важно:** Multi в library sync = **`0040MU` ×16**, не один `0040MU` и не только `8101MU`.

### Sysex77 parity

| Меню | Покрытие |
|------|----------|
| **#30** | Internal **`8101VC+0040VC` interleaved ×64** → `AUTOSYNC-VC-INT.syx` |
| **#31** fast | + PRE1/PRE2 `8101VC`, `8101MU` ×16 — **≠ SYM7 `0040MU`×16** |
| **#32** extended | + SA, WV, PN×32, MT×2 — **PN/MT ✅**, **`0040MU`×16 — gap** |

Генератор:
```bash
python _build_sym7_request.py 0040MU 0 3    # multi slot 3
python _build_sym7_request.py 8101PN 0 0    # pan 0
python _build_sym7_request.py 8101MT 0 1    # microtuning 1
```

---

## 4. Patch invoke (BankClick) — Sysex77 parity

8101 уже в `.syx` (AUTO SYNC). При клике слота:

| TX | mm | Ожидаемый RX |
|----|-----|--------------|
| **`0040VC`** | globalIdx `00`…`3F` | **115 B**, tag `0040VC` (**verified ANONIM 2026-05-24**) |

Код: `Librairie.h::beginBankClick0040Fetch` → `buildSym78101BulkRequest(..., "0040VC", b28, mm)`.

**RX invoke matrix (verified 2026-05-24):**

| Patch | mm | EFSDLV E1/E2 | elmode | Fixture |
|-------|-----|--------------|--------|---------|
| ANONIM | 00 | 127 / 127 | 6 | `05_rx_anonim_8101_0040vc.txt` |
| EP:Classic | 05 | 127 / 127 | 4 | `08_rx_classic_invoke.txt` |
| EP:NiteHwk | 06 | 127 / 100 | 4 | `07_rx_nitehwk_invoke.txt` |

После interleaved autosync (#30) paired 0040 в `.syx` — BankClick **без** lazy TX `0040VC`.

---

## 5. Manual DUMP OUT (07:1 Voice)

SY99 menu → один TX нет; SY99 **push** двух кадров:

```text
[8101VC ~585 B][0040VC ~113 B]
```

Единственный источник **edit buffer** `0040VC` без slot mm.

---

## Sysex77 mapping

| SYM7 | Sysex77 API |
|------|-------------|
| `8101VC` mm=N | `sendSym78101VcRequest(id, mm, b28)` |
| `0040VC` mm=N | `sendSym7VcRequest(id, mm, b28)` |
| `8101SY` | `sendSym78101BulkRequest(id, "8101SY", 0, 0)` |
| `0040MU` | `sendSym78101BulkRequest(id, "0040MU", 0, 0)` |
| Full sync phases | `sy99FullLibrarySyncPhases()` |

---

## Примеры hex

**0040VC A6 (mm=05):**

```text
F0 43 20 7A 4C 4D 20 20 30 30 34 30 56 43 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 05 F7
```

**8101VC A1 (mm=00):**

```text
F0 43 20 7A 4C 4D 20 20 38 31 30 31 56 43 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 F7
```

---

## 6. Реализация Sysex77 (2026-05-24)

| Было (ошибка) | Стало (SYM7 parity) |
|---------------|---------------------|
| BankClick → TX `8101VC` | BankClick → TX **`0040VC` mm=slot** |
| Full sync без paired ingest | Фаза **`0040VC×64`** (отдельный файл) + **#30 interleaved → VC-INT** |
| EFSDLV из 8101 only | Ingest **0040VC** (paired in .syx or lazy fetch) → @100/@104 |

**Internal library = 128 SysEx в двух файлах (full sync fast):**

| Файл | Содержимое | UI / BANK |
|------|------------|-----------|
| `AUTOSYNC-VC-INT.syx` | 64×8101VC (имена A1–D16) | Строка **SY-99** → VOICE |
| `AUTOSYNC-0040VC-INT.syx` | 64×0040VC (хвосты EFSDLV) | **Internal / audit only** — не voice bank, скрыт из BANK |

Код уже в дереве; **сборка + HW smoke** — см. [`HW_CAPTURE_0040VC_A6.md`](HW_CAPTURE_0040VC_A6.md).

---

## 7. RX verify — **PASS** (ANONIM, 2026-05-24)

| | Результат |
|--|-----------|
| **0040VC size** | **115 B** ✅ |
| Tag | `0040VC` ✅ |
| EFSDLV @100 / @104 | **127 / 127** ✅ |
| Paired 8101VC | ~338 B, `ANONIM`, +27 ms |

Capture: [`05_rx_anonim_8101_0040vc.txt`](05_rx_anonim_8101_0040vc.txt) · [`07_rx_nitehwk_invoke.txt`](07_rx_nitehwk_invoke.txt)

| Patch | 0040 B | EFSDLV @100/@104 | elmode |
|-------|--------|------------------|--------|
| ANONIM (A1) | 115 | **127 / 127** | 6 |
| **EP:NiteHwk (A7, mm=06)** | **115** | **127 / 100** ✅ | **4** |

**Осталось:** Sysex77 BankClick ingest → log `has0040=1` + UI 127/100 для NiteHwk.

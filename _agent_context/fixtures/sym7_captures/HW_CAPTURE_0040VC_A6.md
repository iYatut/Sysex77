# HW capture — RX на `0040VC` (SYM7-style request)

**Статус RX format:** **PASS** (ANONIM invoke, 115 B, 2026-05-24)  
**Статус BankClick smoke:** TODO (Sysex77 menu #13 / GrnDual mm=0A)

---

## Находка (SYM7 TX, подтверждено)

1. SYM7 после `8101VC×64` шлёт **`0040VC×64`** (mm `00`…`3F`), интервал ~350 ms.
2. При вызове патча нужен **`0040VC mm=slot`**, не `8101VC` (8101 уже в `.syx` после AUTO SYNC).
3. EFSDLV elmode 4/8 authoritative только в **0040 @100/@104**; `8101 @efln+12` — poison (NiteHwk: 76≠127).

## Реализовано в Sysex77 (2026-05-24)

| Изменение | Файл |
|-----------|------|
| BankClick TX **`0040VC`** (было `8101VC`) | `Librairie.h::beginBankClick0040Fetch` |
| Full sync фаза **`0040VC×64`** после internal `8101VC×64` | `Sy99BulkLibraryCapture.h` |
| Menu test **#13** A6 `0040VC mm=05` | `Librairie.h` |
| `sendSym7VcRequest()` | `Sy99DumpRequest.h` |

---

## RX verified — ANONIM (2026-05-24)

Источник: user MIDI-OX paste «anonim» — [`05_rx_anonim_8101_0040vc.txt`](05_rx_anonim_8101_0040vc.txt)

| Check | Result |
|-------|--------|
| **0040VC frame size** | **115 B** ✅ (ожидали ~113 B) |
| LM tag | `0040VC` @ byte 10 ✅ |
| Device | `F0 43 00 7A` (bulk data, dev 0) ✅ |
| EFSDLV @100 / @104 | **127 / 127** ✅ |
| Paired 8101VC | ~338 B, name `ANONIM`, Δ **~27 ms** |
| Follow-up param9 | `02 00 00 00` = **6** (1 AFM + 1 AWM) |

**Примечание:** захват при **invoke ANONIM** (вероятно mm=`00`, A1) — формат кадра совпадает с fixture `01_init_anonim_07x1_voice.syx` (115 B tail). Отдельный RX на **mm=05 (A6 Classic)** всё ещё желателен, но размер/парсинг подтверждены.

---

## Оставшийся smoke — Sysex77

### Способ A — menu test #13
2. MIDI In+Out на SY99, Bulk Protect **OFF**, Device ID **0** (как в SYM7 логе `43 20`).
3. Librairie → **REQUEST VOICE** → **Test #13: A6 0040 tail (0040VC mm=05)**.
4. Сохранить из MIDI-OX / `Sysex77_MIDI.log`:
   - **TX** 31 B (должен совпасть с hex ниже)
   - **RX** полный SysEx

**Ожидаемый TX:**

```text
F0 43 20 7A 4C 4D 20 20 30 30 34 30 56 43 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 05 F7
```

### Способ B — SYM7 / MIDI-OX вручную

Отправить TX выше на SY99 In, записать RX.

### Способ C — BankClick smoke

1. AUTO SYNC 64 (8101) уже выполнен.
2. Клик **A11 GrnDual** (mm=`0A`) или **A6 Classic** (mm=`05`).
3. В `Sysex77_OUT.log`:

```text
[BankClick] fetch0040 TX 0040VC b28=00 mm=05
[BankClick] fetch0040 elmode=… has0040=1 EFSDLV_E1=… EFSDLV_E2=…
```

---

## Критерии PASS

| Check | PASS |
|-------|------|
| RX length | **115 B** ✅ ANONIM |
| LM tag | `0040VC` ✅ |
| Ingest | `has0040=1` в логе — **TODO Sysex77** |
| EFSDLV GrnDual (mm=0A, elmode 8) | E1/E2 ≈ **127/100** в UI — **TODO** |
| EFSDLV Classic (mm=05, elmode 4) | E1/E2 ≈ **127/127** — ANONIM 127/127 ✅ |

## Куда положить результат

| Файл | Содержимое |
|------|------------|
| `05_rx_0040vc_a6.txt` | MIDI-OX paste TX+RX |
| или `05_rx_0040vc_a6.syx` | только RX кадр |

Обновить: `TEST_STATUS.md` (EFSDLV row), § SYM7 observed RX row в `SYM7_library_sync_progress.md`.

---

## Связанные документы

- [`sym7_request_sequence.md`](sym7_request_sequence.md) — полный каталог TX
- [`../README.md`](../README.md) — формат paired dump
- [`../../lm_8101_offsets.md`](../../lm_8101_offsets.md) — EFSDLV @100/@104

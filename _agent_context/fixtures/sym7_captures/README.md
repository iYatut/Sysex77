# SYM7 Hardware Capture Protocol

Запись исходящих/входящих MIDI-сообщений **SY Manager (SYM7)** при синхронизации библиотеки с SY99.

**Progress board:** [`../../SYM7_library_sync_progress.md`](../../SYM7_library_sync_progress.md)

---

## Prerequisites (SY99)

- Utility → MIDI → **Bulk Protect: OFF**
- **SysEx Device Number:** зафиксировать (рекомендуется `01`); записать в имя файла
- MIDI cable: computer ↔ SY99 In/Out (не только USB monitor без loopback)

## Prerequisites (SYM7)

- SYM7 7.4+ на той же Windows, где тестируется Sysex77
- Synth type: **SY99**
- Midi Engine → включить запись/просмотр TX и RX

---

## Capture scenarios (minimum)

| # | SYM7 action | Save as | Look for in TX |
|---|-------------|---------|----------------|
| 1 | Launch → select SY99 → open **Patch Viewer** | `01_startup_patchviewer.txt` | Device Inquiry `F0 7E 7F 06 01 F7`? |
| 2 | Download **edit buffer** / current voice | `02_single_edit.syx` or `.txt` | `mt=7F mm=00` tail bytes |
| 3 | Download **A1** internal | `03_single_a1.syx` | `mt=00 mm=00` |
| 4 | Download **64 internal voices** | `04_bank64_tx.txt` + `04_bank64_rx.syx` | 64× `8101VC` + **64× `0040VC`** |
| 4b | **RX single 0040VC A6** | `05_rx_0040vc_a6.txt` | TX mm=05 → RX ~113 B — **P0 before EFSDLV PASS** |
| 5 | Download **everything** | `05_everything_tx.txt` | VC×64 → MU → SY → MS order |
| 6 | Repeat #4 with Device ID **02** | `06_device02_bank64.txt` | `2n` byte = `0x22`? |

---

## Export formats

Предпочтительно (любой из):

1. **SYM7 Midi Engine** — copy/paste hex log → `.txt`
2. **Standard MIDI File** — `.mid` (если SYM7 экспортирует)
3. **SysEx dump** — `.syx` (только RX от SY99 или combined)

Именование: `{scenario}_{YYYYMMDD}_{deviceId}.{ext}`

---

## After capture

1. Положить файлы в **эту папку** (`_agent_context/fixtures/sym7_captures/`)
2. Обновить таблицы в [`SYM7_library_sync_progress.md`](../../SYM7_library_sync_progress.md):
   - § SYM7 capture log
   - § Command catalog → SYM7 observed
   - § Decision log (tail variant, timing)
3. Сравнить RX с baselines:

   ```bash
   python _agent_context/fixtures/_validate_dump_request.py
   ```

4. Обновить [`sy99_bulk_dump_request.md`](../../sy99_bulk_dump_request.md) §15–16

---

## Sysex77 cross-check (without SYM7)

После Auto-sync 64 в Sysex77 (Librairie → REQUEST VOICE → Auto-sync 64 internal):

- Ожидается файл `%AppData%/Application Support/Sysex77/captures/AUTOSYNC-VOICE64-*.syx`
- Сравнить с manual FROM SY99 ← 64 Voice baseline

---

## Notes

- Parameter-change SysEx (`F0 43 1n 34 …`) при drag слайдеров — **не** включать в library sync capture
- ALL DATA file cannot be sent as one bulk over MIDI (SY99 manual) — SYM7 uses sequential requests

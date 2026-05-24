# SYM7 startup capture — parsed (2026-05-23)

Source: MIDI-OX Monitor during SY Manager startup sync (user paste).

## Frame template (31 bytes) — VERIFIED

```text
F0 43 20 7A 4C 4D 20 20 38 31 30 31 [TT TT] 00 00 00 00
00 00 00 00 00 00 00 00 00 00 [MM] F7
```

| Offset | Value | Meaning |
|--------|-------|---------|
| 0 | F0 | SysEx start |
| 1 | 43 | Yamaha |
| 2 | 20 | Bulk **request**, device ID **0** (`0x20 \| 0`) |
| 3 | 7A | Bulk subcommand |
| 4–13 | `4C 4D 20 20` + 6 chars | `"LM  "` + type string |
| 14–15 | 00 00 | Fixed |
| 16–28 | 00… | Zero padding |
| **29** | **MM** | **Memory number / slot index** (increments 0x00..0x3F) |
| 30 | F7 | End |

**Not** `0040VC` + mt/mm/checksum (Sysex77 current guess).  
**Is** `8101VC` / `8101MU` / `8101PN` / `8101MT` + slot byte at offset 29.

### Example — voice A1 (mm=0)

```text
F0 43 20 7A 4C 4D 20 20 38 31 30 31 56 43 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 F7
```

### Example — voice B1 (mm=0x10)

```text
... 38 31 30 31 56 43 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 10 F7
```

## Startup sequence (SYM7 auto-sync) — verified 2026-05-23

| Phase | LM type | Count | mm range | TX→TX ms | Notes |
|-------|---------|-------|----------|----------|-------|
| 0 | **8101SY** | 1 | — | — | Connect handshake |
| 0 | **param9** ×2 | — | — | ~134 / ~28 | `F0 43 10 34 0F…` — Sysex77 не шлёт |
| 0 | **0040MU** | 1 | `00` | — | Multi header |
| 1 | **8101VC** | 64 | `0x00..0x3F` | ~258 | Internal voices banks A–D |
| 2 | **0040VC** | 64 | `0x00..0x3F` | ~140–550 | Voice common tail (EFSDLV @100/@104) |
| 3 | **8101VC** | 64 | b28=`02` | ~260 | Preset 1 |
| 4 | **8101VC** | 64 | b28=`03` | ~245 | Preset 2 |
| … | **8101MU**, **0040SA**, **0040WV**, **8101PN**, **8101MT** | — | — | — | Extended sync |

Inter-request gap ≈ **250–350 ms** (8101VC), **~350 ms** (0040VC sweep).

## Pairing rule

| Request | Paired request | Use |
|---------|----------------|-----|
| `8101VC` mm=N | `0040VC` mm=N | Full voice; BankClick lazy-fetch = **0040VC only** if 8101 in `.syx` |
| Manual 07:1 | RX push both | Edit buffer |

## Sysex77 gap (why no response)

| SYM7 | Sysex77 (before fix) |
|------|----------------------|
| `8101VC` | `0040VC` |
| 31 B, zero pad, mm @ byte 29 | variant A/B, mt/mm/checksum |
| Device `43 20` (ID 0) | `DeviceModel` may differ |

## Next: implement `buildSym7Style8101Request()` in Sy99DumpRequest.h

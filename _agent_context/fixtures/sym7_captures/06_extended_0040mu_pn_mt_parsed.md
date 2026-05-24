# SYM7 extended sync — «другие банки» (2026-05-24)
# User MIDI-OX paste after Internal already loaded

## Sequence summary

| Phase | LM type | Count | mm range | b28 | ~TX→TX ms |
|-------|---------|-------|----------|-----|-----------|
| 1 | **0040VC** | 64 | 0x00..0x3F | 0x00 | ~340 |
| 2 | **0040MU** | 16 | 0x00..0x0F | 0x00 | ~325 |
| 3 | **8101PN** | 32 | 0x00..0x1F | 0x00 | ~100 |
| 4 | **8101MT** | 2 | 0x00..0x01 | 0x00 | ~166 |

**Not in this paste:** `8101VC` PRE1/PRE2 (b28=02/03), `8101MU`, `0040SA`, `0040WV`, `8101SY`.

## Notes

- Connect handshake uses **one** `0040MU mm=00`; full library sync uses **`0040MU` ×16** (one per multi slot).
- All requests: 31 B SYM7 format, mm @ byte 29, b28=0x00 in this capture.
- **0040VC×64** repeats internal voice tails (same as first internal-only sync).

## Sysex77 gap

| SYM7 (this log) | Sysex77 extended (#32) |
|-----------------|-------------------------|
| `0040MU` ×16 | `8101MU` ×16 (+ `0040MU` ×1 at start only) |
| `8101PN` ×32 | `8101PN` ×32 ✅ |
| `8101MT` ×2 | `8101MT` ×2 ✅ |

Consider adding phase **`0040MU` mm=0..15** after internal 0040VC (or replace MU block — needs RX verify).

## Example hex

0040MU slot 3:
F0 43 20 7A 4C 4D 20 20 30 30 34 30 4D 55 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 03 F7

8101PN slot 0:
F0 43 20 7A 4C 4D 20 20 38 31 30 31 50 4E 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 F7

8101MT slot 1:
F0 43 20 7A 4C 4D 20 20 38 31 30 31 4D 54 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 01 F7

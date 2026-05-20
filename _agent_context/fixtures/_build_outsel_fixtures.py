#!/usr/bin/env python3
"""Build OUTSEL diff fixtures 04 (both) / 05 (G1) from ANONIM 07:1."""
from pathlib import Path

FIX = Path(__file__).resolve().parent
SRC = FIX / "01_init_anonim_07x1_voice.syx"
OUT_BOTH = FIX / "04_anonim_outsel_both.syx"
OUT_G1 = FIX / "05_anonim_outsel_g1.syx"


def load8101(data: bytes) -> tuple[bytearray, int, int]:
    i = 0
    while i < len(data):
        if data[i] != 0xF0:
            i += 1
            continue
        j = i + 1
        while j < len(data) and data[j] != 0xF7:
            j += 1
        frame = bytearray(data[i : j + 1])
        if b"8101VC" in frame:
            return frame, i, j + 1
        i = j + 1
    raise SystemExit("no 8101VC")


def find_elvl_e1(frame: bytes) -> int:
    for wol in range(44, min(len(frame) - 4, 130)):
        if frame[wol - 1] not in (0x03, 0x01):
            continue
        if frame[wol + 1] != 0 or frame[wol + 2] != 0:
            continue
        if 93 <= wol <= 96:
            return wol + 3
    raise SystemExit("no ELVL E1")


def main() -> None:
    raw = SRC.read_bytes()
    frame, start, end = load8101(raw)
    outsel_off = find_elvl_e1(frame) + 1
    print(f"ELVL E1 @+{outsel_off - 1}, OUTSEL El.1 @+{outsel_off} (was 0x{frame[outsel_off]:02X})")

    both = bytearray(raw)
    both_frame, _, _ = load8101(both)
    both_frame[outsel_off] = 0x06
    both[start:end] = both_frame

    g1 = bytearray(both)
    g1_frame, _, _ = load8101(g1)
    g1_frame[outsel_off] = 0x02
    g1[start:end] = g1_frame

    OUT_BOTH.write_bytes(both)
    OUT_G1.write_bytes(g1)
    print(f"wrote {OUT_BOTH.name} OUTSEL=0x06, {OUT_G1.name} OUTSEL=0x02 @{outsel_off}")


if __name__ == "__main__":
    main()

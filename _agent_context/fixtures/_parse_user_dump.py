#!/usr/bin/env python3
"""Parse user-pasted SY99 bulk dumps for EFSDLV analysis."""
from __future__ import annotations

import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent
sys.path.insert(0, str(FIX))
from _validate_bulk_parse import find_lm8101_frame, find_wol_elvl_e1_pair

DUMP8101 = """
F0 43 0 7A 4 43 4C 4D 20 20 38 31 30 31 56 43 0 0 0 0 0 0 0 0 0 0 0 0 0 0 7F A 8
45 50 7C 47 72 6E 44 75 61 6C 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
2 1C 0 F 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 7F 0 0 7C 3 21 0 7F 1 7F 2C 0 7F B 40 0 7F 1 7F 26 0
3F 12 A 3F 1C 3F 3F 3F 3C 0 0 0 0 3 3F 0 3 0 6 0 0 6 11 33 0 0 C 0 0 3 6A 0 1F 54 60 1 0 1 0 1 0 0 78 0 1 0
3F D B 3F 1F 3F 3F 3C 0 0 0 0 3 3F 0 4 0 1 0 0 1 4 3F 3 0 C 0 0 3 7F 0 1F 3E 5D 1 0 1 0 1 0 1 0 0 1 0
3F 12 A 3F 1B 3F 3F 3C 0 0 0 0 3 3F 0 3 0 6 0 0 7 12 3B 0 0 C 0 0 13 6C 0 1F 54 60 1 0 1 0 1 0 0 78 0 1 0
3F D B 3F 1F 3F 3F 3C 0 0 0 0 3 3F 0 4 0 3 0 0 1 14 3F 3 0 C 0 0 13 7F 0 1F 3E 5D 1 0 1 0 1 0 1 0 0 1 0
3F 1C 15 3F 1A 3F 3F 37 0 0 0 0 3 3F 0 3 0 6 0 0 0 13 3F 0 0 0 0 0 0 67 0 43 4F 67 1 0 1 0 0 6C 0 6F 0 C 0
3F 15 D 3F 1F 3F 3F 39 0 0 0 0 3 3F 0 4 0 4 0 0 1 14 3F 3 0 8 0 0 17 76 0 1F 3E 5D 1 0 1 0 1 0 1 0 0 1 0
29 3F 3F 3F 3F 40 40 40 40 0 0 0 0 3A 0 8 0 0 0 0 0 0 0 0 0 0 1 7F 0 0 0 0 0 0 0 40 40 40 40 40 40 40 0 0 30 54 7F
1 0 1 0 1 0 1 0 1 7F 0 0 0 0 0 0 0 40 40 40 40 40 40 40 0 0 30 54 7F 1 0 1 0 1 0 1 0 0 3 0 0 0 0 0
3C 40 3 3E 3F 3F 3F 40 40 40 40 40 1 0 0 36 13 0 0 0 0 0 0 0 18 0 16 0 0 0 0 0 40 40 40 40 40 40 40 1 0 48 54 7F
1 0 1 0 1 0 1 0 1 66 0 3F 3 0 0 0 0 40 40 22 22 22 22 22 7 0 3C 48 7F 1 0 1 0 1 9 1 D 0 6 0 0
3F C D C 1D 3F 3A 2 4B 4C 50 60 1 0 1 0 1 0 1 0 3 1 0 2 F7
"""


def token_bytes(text: str) -> list[int]:
    out: list[int] = []
    for raw in text.split():
        x = raw.strip()
        if x in ("F0", "F7"):
            continue
        if len(x) <= 2 and any(c in "abcdefABCDEF" for c in x):
            out.append(int(x, 16) & 0xFF)
        else:
            out.append(int(x) & 0xFF)
    return out


def main() -> None:
    f1 = token_bytes(DUMP8101)
    frame = bytes([0xF0, *f1, 0xF7])
    name = bytes(f1[33:43]).decode("ascii", "replace").rstrip("\x00 ")
    print(f"8101VC len={len(f1)} name={name!r} elmode@32={f1[32]}")

    pair = find_wol_elvl_e1_pair(frame)
    if pair is None:
        print("no wol/elvl pair")
        return
    wol_off, elvl_off, wol, elvl = pair
    efln = elvl_off + {94: 36, 93: 37, 95: 35}.get(wol_off, 36)
    print(f"wol_off={wol_off} elvl_off={elvl_off} wol={wol} elvl_e1={elvl}")
    print(f"efln@{efln}={f1[efln]}  parser EFSDLV@+12={f1[efln + 12]}")

    print("\n--- efln .. +16 (127 = LCD effect send?) ---")
    for d in range(-2, 18):
        off = efln + d
        if not (0 <= off < len(f1)):
            continue
        b = f1[off]
        tags: list[str] = []
        if d == 0:
            tags.append("EFLN1EL")
        if d == 12:
            tags.append("parser-EFSDLV")
        if b == 127:
            tags.append("**127**")
        print(f"  +{d:2d} @{off:3d}: {b:3d}  {' '.join(tags)}")

    print("\n--- anchor E2 strip ---")
    for a in range(90, min(125, len(f1) - 7)):
        if f1[a] == 0x7F and f1[a + 1] == 0x01 and f1[a + 2] == 0x7F:
            print(f"anchor@{a}: elvl2={f1[a+5]} eldt2={f1[a+6]} outsel={f1[a+4]&6}")
            for d in range(0, 14):
                b = f1[a + d]
                mark = "127!" if b == 127 else ""
                print(f"    +{d:2d}: {b:3d} {mark}")

    baseline = find_lm8101_frame(
        (FIX / "baseline_ep_grndual.syx").read_bytes()
    )
    if baseline:
        same = sum(1 for i in range(min(len(f1), len(baseline))) if f1[i] == baseline[i])
        print(f"\nbaseline match: {same}/{min(len(f1), len(baseline))} bytes identical")
        print(f"efln+12 live={f1[efln+12]} baseline={baseline[efln+12]}")


if __name__ == "__main__":
    main()

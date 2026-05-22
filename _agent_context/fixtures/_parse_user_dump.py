#!/usr/bin/env python3
"""Parse user-provided ANONIM dump from chat."""
from __future__ import annotations

D1_HEX = """
F0 43 0 7A 2 4E 4C 4D 20 20 38 31 30 31 56 43 0 0 0 0 0 0 0 0 0 0 0 0 0 0 7F 0 6
41 4E 4F 4E 49 4D 20 20 20 20 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
1 0 1 40 C 23 1 0 D 0 C 0 A 0 2 0 E 0 0 1 0 0 3 7F 0 0 34 0 44 25 61 1 7F 20 2 7F 0 40 0 7F 1 7F 20 6 0 0 40 0 3C 40 3 3F 3F 3F 3F 40 40 40 40 40 1 0 0 41 0 0 0 0 0 0 0 0 7F 1 0 0 0 0 0 0 40 40 40 40 40 40 40 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 7F 1 0 0 0 0 0 0 40 40 40 40 40 40 40 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 0 0 0 3F 3F 3F 0 3F 3F 3F 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 0 0 0 0 40 0 3C 40 3 3F 3F 3F 3F 40 40 40 40 40 1 0 0 41 0 0 0 0 0 0 0 0 7F 1 0 0 0 0 0 0 40 40 40 40 40 40 40 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 7F 1 0 0 0 0 0 0 40 40 40 40 40 40 40 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 0 0 0 3F 3F 3F 0 3F 3F 3F 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 0 0 0 21 F7
"""

D2_HEX = """
F0 43 0 7A 0 6B 4C 4D 20 20 30 30 34 30 56 43 0 0 0 0 0 0 0 0 0 0 0 0 0 0 7F 0 6
2 0 0 19 0 8 0 A 2 3F 1 47 0 4 0 37 0 7 0 6 0 14 64 64 0 0 19 0 8 0 A 2 3F 1 47 0 4 0 37 0 7 0 6 0 14 2B 64 64 32 32 0 0 0 63 0 0 0 63 0 41 0 64 0 0 0 40 F 7F 9 0 F 7F 0 0 4 0 3F 4 0 3F 2E F7
"""


def hb(s: str) -> bytes:
    return bytes(int(x, 16) for x in s.split())


def eldt_ui(raw: int) -> int:
    raw &= 0x7F
    if raw <= 63:
        return raw
    return raw - 128


def elns_ui(raw: int) -> int:
    return raw - 64


def outsel_ui(raw: int) -> str:
    m = raw & 0x06
    return {0: "Off", 2: "G1", 4: "G2", 6: "Both"}.get(m, f"0x{m:02X}")


def main() -> None:
    d1, d2 = hb(D1_HEX), hb(D2_HEX)
    print(f"8101VC len={len(d1)} (full voice ~475-586 B)")
    print(f"0040VC len={len(d2)}")
    print(f"ELMODE@32 = {d1[32]}")
    print(f"VNAM = {d1[33:43]!r}")
    print()

    # WOL/ELVL E1 search
    print("=== WOL / ELVL E1 ===")
    for i in range(31, len(d1) - 4):
        if d1[i - 1] in (0x03, 0x01) and d1[i + 1] == 0 and d1[i + 2] == 0:
            elvl = d1[i + 3]
            if d1[i] <= 127 and elvl <= 127:
                print(f"  prefix 0x{d1[i-1]:02X} @ {i-1}: WOL={d1[i]} ELVL={elvl} (outsel@+1={d1[i+4] if i+4<len(d1) else '?'})")

    # Typical offsets
    print("\n=== bytes @ typical offsets ===")
    for off in range(90, 105):
        if off < len(d1):
            print(f"  @{off:3d} = 0x{d1[off]:02X} ({d1[off]:3d})")

    print("\n=== anchors 7F 01 7F ===")
    for i in range(len(d1) - 8):
        if d1[i : i + 3] == bytes([0x7F, 0x01, 0x7F]):
            os_raw = d1[i + 4]
            elvl = d1[i + 5]
            eldt = d1[i + 6]
            elns = d1[i + 7]
            print(
                f"  @{i}: OUTSEL={outsel_ui(os_raw)} raw=0x{os_raw:02X} "
                f"ELVL={elvl} ELDT={eldt}({eldt_ui(eldt)}) ELNS={elns}({elns_ui(elns)})"
            )

    print("\n=== 0040VC confirmed ===")
    print(f"  SPTPNT@98 = {d2[98]} (UI shows 60 → expect 0x3C)")
    print(f"  WPBR@41 = {d2[41]}")
    print(f"  WLLML@55 = {d2[55]}")
    print(f"  MCTUN@90 = {d2[90]}")
    print(f"  AFTMD@100 = {d2[100]}")

    print("\n=== UI screenshot (from user) ===")
    ui = {
        "ELVL_E1": 124,
        "ELVL_E2": 127,
        "ELDT_E1": 0,
        "ELDT_E2": 0,
        "ELNS_E1": 0,
        "ELNS_E2": 0,
        "OUTSEL_E1": "G1",
        "OUTSEL_E2": "G2",
        "SPTPNT": 60,
        "WPBR": 0,
        "MCTUN": 0,
    }
    for k, v in ui.items():
        print(f"  {k} = {v}")


if __name__ == "__main__":
    main()

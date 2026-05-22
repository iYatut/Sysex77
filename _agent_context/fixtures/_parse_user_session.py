#!/usr/bin/env python3
"""Parse user chat SysEx dumps."""
from __future__ import annotations

import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent
sys.path.insert(0, str(FIX))

from _parse_0040vc import parse_lm0040_vc
from _validate_bulk_parse import parse_lm8101_vc

D1 = """
F0 43 0 7A 4 43 4C 4D 20 20 38 31 30 31 56 43 0 0 0 0 0 0 0 0 0 0 0 0 0 0 7F A 8 45 50 7C 47 72 6E 44 75 61 6C 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 2 1C 0 F 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 7F 0 0 7C 3 21 0 7F 1 7F 2C 0 7F B 40 0 7F 1 7F 26 0 3F 12 A 3F 1C 3F 3F 3C 0 0 0 0 3 3F 0 3 0 6 0 0 6 11 33 0 0 C 0 0 3 6A 0 1F 54 60 1 0 1 0 1 0 0 78 0 1 0 3F D B 3F 1F 3F 3F 3C 0 0 0 0 3 3F 0 4 0 1 0 0 1 4 3F 3 0 C 0 0 3 7F 0 1F 3E 5D 1 0 1 0 1 0 1 0 0 1 0 3F 12 A 3F 1B 3F 3F 3C 0 0 0 0 3 3F 0 3 0 6 0 0 7 12 3B 0 0 C 0 0 13 6C 0 1F 54 60 1 0 1 0 1 0 0 78 0 1 0 3F D B 3F 1F 3F 3F 3C 0 0 0 0 3 3F 0 4 0 3 0 0 1 14 3F 3 0 C 0 0 13 7F 0 1F 3E 5D 1 0 1 0 1 0 1 0 0 1 0 3F 1C 15 3F 1A 3F 3F 37 0 0 0 0 3 3F 0 3 0 6 0 0 0 13 3F 0 0 0 0 0 0 67 0 43 4F 67 1 0 1 0 0 6C 0 6F 0 C 0 3F 15 D 3F 1F 3F 3F 39 0 0 0 0 3 3F 0 4 0 4 0 0 1 14 3F 3 0 8 0 0 17 76 0 1F 3E 5D 1 0 1 0 1 0 1 0 0 1 0 29 3F 3F 3F 3F 40 40 40 40 0 0 0 0 3A 0 8 0 0 0 0 0 0 0 0 0 0 1 7F 0 0 0 0 0 0 0 40 40 40 40 40 40 40 0 0 30 54 7F 1 0 1 0 1 0 1 0 1 7F 0 0 0 0 0 0 0 40 40 40 40 40 40 40 0 0 30 54 7F 1 0 1 0 1 0 1 0 0 3 0 0 0 0 0 3C 40 3 3E 3F 3F 3F 40 40 40 40 40 1 0 0 36 13 0 0 0 0 0 0 0 18 0 16 0 0 0 0 0 40 40 40 40 40 40 40 1 0 48 54 7F 1 0 1 0 1 0 1 0 1 66 0 3F 3 0 0 0 0 40 40 22 22 22 22 22 7 0 3C 48 7F 1 0 1 0 1 9 1 D 0 6 0 0 3F C D C 1D 3F 3A 2 4B 4C 50 60 1 0 1 0 1 0 1 0 3 1 0 2 F7
"""

D2 = """
F0 43 0 7A 0 6B 4C 4D 20 20 30 30 34 30 56 43 0 0 0 0 0 0 0 0 0 0 0 0 0 0 7F A 8 0 2C 0 A 0 C 0 5 0 7 0 8 0 8 0 8 0 32 0 28 0 64 64 64 0 0 13 0 6 0 A 2 2B 0 0 0 4 0 32 0 6 0 6 0 12 27 64 7 64 26 9 D 0 63 0 7C 1 63 0 0 63 64 0 0 0 3C F 7F 0 0 F 7F 0 0 0 0 0 0 0 0 15 F7
"""


def hb(s: str) -> bytes:
    return bytes(int(x, 16) for x in s.split())


def eldt_ui(raw: int) -> int:
    raw &= 0x7F
    delta = 9
    if raw > delta - 2:
        raw -= delta
        raw = ~raw
    return max(-7, min(7, raw))


def main() -> None:
    d1, d2 = hb(D1), hb(D2)
    p = parse_lm8101_vc(d1)
    p40 = parse_lm0040_vc(d2)
    print(f"8101 len={len(d1)} name={p.get('name')!r} elmode={p.get('elmodeRaw')}")
    print(f"ELVL={p.get('elvlRaw')}")
    print(f"ELDT={p.get('eldtRaw')} -> UI E1={eldt_ui(p['eldtRaw'][0]) if p['eldtRaw'][0]>=0 else '?'} E2={eldt_ui(p['eldtRaw'][1]) if p['eldtRaw'][1]>=0 else '?'}")
    print(f"ELNS={p.get('elnsRaw')}")
    print(f"OUTSEL E1={p.get('outselE1Raw')} (@99=0x{d1[99]:02X} &6=0x{d1[99]&6:02X}) outselRaw={p.get('outselRaw')}")
    print("0040:")
    for k in ["MCTUN", "ATPBR", "WPBR", "WLLML", "SPTPNT", "PMASN", "PMRNG", "EFMODE"]:
        info = p40.get("fields", {}).get(k, {})
        print(f"  {k}: raw={info.get('raw')} off={info.get('offset')}")
    print(f"0040 @90 direct={d2[90]}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Regression: EFSDLV elmode 4 — authoritative 0040 @100/@104 (HW EP:NiteHwk 2026-05-24).

8101 @ efln+12 is NOT EFSDLV on NiteHwk (76 poison vs LCD 127/100).
Classic fixture 03 still matches 0040 @100/@104 = 127/127.
"""
from __future__ import annotations

import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent
K_EFLN_DELTA = 12
K_0040_E1 = 100
K_0040_E2 = 104


def iter_frames(data: bytes):
    i = 0
    while i < len(data):
        if data[i] != 0xF0:
            i += 1
            continue
        j = data.find(0xF7, i)
        if j < 0:
            break
        yield data[i : j + 1]
        i = j + 1


def find_tag(data: bytes, tag: bytes) -> bytes | None:
    for fr in iter_frames(data):
        if tag in fr:
            return fr
    return None


def efln_off(frame: bytes) -> int:
    elvl = 98
    wol = 95
    if wol == 93:
        return elvl + 37
    if wol == 95:
        return elvl + 35
    return elvl + 36


def check(path: Path, expect_e1: int, expect_e2: int, label: str) -> bool:
    data = path.read_bytes()
    f8101 = find_tag(data, b"8101VC")
    f0040 = find_tag(data, b"0040VC")
    ok = True

    if f8101 is None or f0040 is None:
        print(f"{label}: FAIL missing 8101VC or 0040VC")
        return False

    elmode = f8101[32]
    poison = f8101[efln_off(f8101) + K_EFLN_DELTA] if efln_off(f8101) + K_EFLN_DELTA < len(f8101) else -1
    e1 = f0040[K_0040_E1]
    e2 = f0040[K_0040_E2]

    print(f"{label}: elmode={elmode} 8101@efln+12={poison} 0040@100={e1} 0040@104={e2}")

    if e1 != expect_e1:
        print(f"  FAIL E1 expected {expect_e1} from 0040@100")
        ok = False
    if e2 != expect_e2:
        print(f"  FAIL E2 expected {expect_e2} from 0040@104")
        ok = False

    if label.startswith("nitehwk") and poison == expect_e1:
        print(f"  NOTE 8101 poison accidentally equals E1 ({poison})")
    elif label.startswith("nitehwk") and poison != expect_e1:
        print(f"  OK 8101@efln+12={poison} != LCD E1={expect_e1} (must not use 8101 alone)")

    return ok


def main() -> int:
    cases = [
        (FIX / "03_ep_classic_07x1_voice.syx", 127, 127, "classic"),
        (FIX / "baseline_ep_nitehwk_voice.syx", 127, 100, "nitehwk_hw"),
    ]
    ok = all(check(p, e1, e2, lbl) for p, e1, e2, lbl in cases)
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

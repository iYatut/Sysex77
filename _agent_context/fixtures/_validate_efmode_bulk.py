#!/usr/bin/env python3
"""Regression: EFMODE bulk @ 0040VC offset +33 (fixtures 06–08)."""
from __future__ import annotations

import sys
from pathlib import Path

from _parse_0040vc import find_frame, parse_lm0040_vc

FIX = Path(__file__).resolve().parent

CASES = (
    ("06", "06_efmode_off_ep_grndual.syx", 0),
    ("07", "07_efmode_serial_ep_grndual.syx", 1),
    ("08", "08_efmode_parallel_ep_grndual.syx", 2),
)


def main() -> int:
    all_pass = True

    for fid, fn, expected in CASES:
        path = FIX / fn
        if not path.is_file():
            print(f"{fid}: FAIL missing {fn}")
            all_pass = False
            continue

        frame = find_frame(path.read_bytes(), "0040VC")
        if frame is None:
            print(f"{fid}: FAIL no 0040VC")
            all_pass = False
            continue

        parsed = parse_lm0040_vc(frame)
        raw = parsed.get("fields", {}).get("EFMODE", {}).get("raw", -1)

        if raw != expected:
            print(f"{fid}: FAIL EFMODE raw={raw} expected={expected}")
            all_pass = False
        else:
            print(f"{fid}: PASS EFMODE={raw}")

    return 0 if all_pass else 1


if __name__ == "__main__":
    sys.exit(main())

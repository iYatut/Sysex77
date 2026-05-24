#!/usr/bin/env python3
"""Regression: 0040VC bulk offsets (fixtures 01-03 + EFMODE 06-08)."""
from __future__ import annotations

import sys
from pathlib import Path

from _parse_0040vc import find_frame, parse_lm0040_vc

FIX = Path(__file__).resolve().parent

# Stable across fixtures 01-03 (lcd_reference / lm_8101_offsets)
STABLE_010203 = {
    "WLLML": 100,
    "SPTPNT": 60,
}

# Per-fixture expectations where LCD or capture notes exist
FIXTURE_EXPECT = {
    "01": {"WPBR": 2, "ATPBR": 63},
    "02": {"WPBR": 0, "ATPBR": 99},
    "03": {"WPBR": 0, "ATPBR": 8},
}

EFMODE_CASES = (
    ("06", "06_efmode_off_ep_grndual.syx", 0),
    ("07", "07_efmode_serial_ep_grndual.syx", 1),
    ("08", "08_efmode_parallel_ep_grndual.syx", 2),
)


def main() -> int:
    all_pass = True

    for fid, fn in (
        ("01", "01_init_anonim_07x1_voice.syx"),
        ("02", "02_ap_crsrock_07x1_voice.syx"),
        ("03", "03_ep_classic_07x1_voice.syx"),
    ):
        path = FIX / fn
        frame = find_frame(path.read_bytes(), "0040VC")
        if frame is None:
            print(f"{fid}: FAIL no 0040VC")
            all_pass = False
            continue

        parsed = parse_lm0040_vc(frame)
        fields = parsed.get("fields", {})
        checks: list[tuple[str, bool, str]] = []

        for tag, exp in STABLE_010203.items():
            raw = fields.get(tag, {}).get("raw", -1)
            checks.append((tag, raw == exp, f"{raw} vs {exp}"))

        for tag, exp in FIXTURE_EXPECT[fid].items():
            raw = fields.get(tag, {}).get("raw", -1)
            checks.append((tag, raw == exp, f"{raw} vs {exp}"))

        failed = [f"{k}({d})" for k, ok, d in checks if not ok]
        if failed:
            print(f"{fid}: FAIL — " + ", ".join(failed))
            all_pass = False
        else:
            print(f"{fid}: PASS ({len(checks)} checks)")

    for fid, fn, expected in EFMODE_CASES:
        path = FIX / fn
        frame = find_frame(path.read_bytes(), "0040VC")
        if frame is None:
            print(f"{fid}: FAIL no 0040VC")
            all_pass = False
            continue
        raw = parse_lm0040_vc(frame).get("fields", {}).get("EFMODE", {}).get("raw", -1)
        if raw != expected:
            print(f"{fid}: FAIL EFMODE raw={raw} expected={expected}")
            all_pass = False
        else:
            print(f"{fid}: PASS EFMODE={raw}")

    return 0 if all_pass else 1


if __name__ == "__main__":
    sys.exit(main())

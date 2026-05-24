#!/usr/bin/env python3
"""Scan efln neighborhood for EFSD* bulk bytes (phase 2)."""
from __future__ import annotations

from pathlib import Path

from _validate_bulk_parse import find_lm8101_frame, find_wol_elvl_e1_pair

FIX = Path(__file__).resolve().parent

# Expected from lcd_reference / notes
EXPECTED = {
    "01_init_anonim_07x1_voice.syx": {"efln": None, "efsdlv": 0, "vsns": 0, "scl": 0},
    "02_ap_crsrock_07x1_voice.syx": {"efln": None, "efsdlv": 0, "vsns": 0, "scl": 0},
    "03_ep_classic_07x1_voice.syx": {"efln": 0x05, "efsdlv": 127, "vsns": 0, "scl": 0},
}


def efln_off(wol_off: int, elvl_off: int) -> int:
    delta = {94: 36, 93: 37, 95: 35}.get(wol_off, 36)
    return elvl_off + delta


def main() -> None:
    for fn, exp in EXPECTED.items():
        data = (FIX / fn).read_bytes()
        frame = find_lm8101_frame(data)
        if frame is None:
            print(f"{fn}: no 8101")
            continue
        pair = find_wol_elvl_e1_pair(frame)
        if pair is None:
            print(f"{fn}: no wol/elvl")
            continue
        wol_off, elvl_off, _wol, _elvl = pair
        base = efln_off(wol_off, elvl_off)
        print(f"\n=== {fn} efln@{base} ===")
        print(f"  expected EFLN={exp['efln']} EFSDLV={exp['efsdlv']} VSNS={exp['vsns']} SCL={exp['scl']}")
        hits = []
        for d in range(0, 20):
            off = base + d
            if off >= len(frame):
                break
            b = frame[off]
            parts = []
            if exp["efln"] is not None and b == exp["efln"] and d == 0:
                parts.append("EFLN")
            if b == exp["efsdlv"]:
                parts.append("EFSDLV?")
            if b == exp["vsns"] and d > 0:
                parts.append("VSNS?")
            if b == exp["scl"] and d > 0:
                parts.append("SCL?")
            mark = f"  <-- {' '.join(parts)}" if parts else ""
            print(f"  +{d:2d} @{off:3d}: {b:3d} 0x{b:02X}{mark}")
            if "EFSDLV?" in mark:
                hits.append(("EFSDLV", d))
        print(f"  EFSDLV hits at delta: {[d for _, d in hits]}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Parse LM 0040VC group-02 tail (fixtures 01–03).

Encoding (confirmed fixtures 01–03, see lm_8101_offsets.md):
  Primary packed block @36–56 (duplicate @57–76 ignored).
  Tail block @90–101 (EFSDLV E1/E2 / SPTPNT region).

Absolute offsets are from start of F0…F7 frame.
"""
from __future__ import annotations

from pathlib import Path
from typing import Any

FIX = Path(__file__).resolve().parent

# Absolute offsets — only entries marked confirmed in lm_8101_offsets.md
K_0040 = {
    "EFMODE": 33,    # ✅ live 08 00 00 20; bulk 0040 @+33 (fixtures 06–08)
    "WPBR": 41,      # ✅ NN 0x28
    "ATPBR": 42,     # confirmed fixtures 01-03 (sysex raw @+42)
    "PMRNG": 40,     # confirmed fixtures 01-03
    "PMASN": 44,     # confirmed fixtures 01-03
    "AMASN": 48,     # confirmed (shares byte with FMRNG in map)
    "AMRNG": 50,     # confirmed fixtures 01-03
    "FMASN": 47,     # confirmed fixtures 01-03
    "FMRNG": 48,     # shares byte with AMASN — display only
    "PNLASN": 49,    # confirmed fixtures 01-03
    "PNLRNG": 50,    # shares byte with AMRNG
    "COASN": 51,     # confirmed fixtures 01-03
    "CORNG": 52,     # confirmed fixtures 01-03
    "PNBASN": 53,     # confirmed fixtures 01-03
    "PNBRNG": 54,     # confirmed fixtures 01-03
    "EGBASN": 46,     # confirmed fixtures 01-03
    "EGBRNG": 54,     # shares byte with PNBRNG
    "WLASN": 56,     # confirmed fixtures 01-03
    "WLLML": 55,     # ✅ NN 0x39
    "MCTUN": 90,     # confirmed fixtures 01-03
    "RNDP": 52,      # alias candidate — shares CORNG byte
    "EFSDLV_E1": 100,  # ✅ elmode 8 hardware diff (was mis-tagged AFTMD)
    "EFSDLV_E2": 104,  # ✅ elmode 8 hardware diff
    "SPTPNT": 98,    # ✅ NN 0x43
}

CONFIRMED = {
    "EFMODE", "WPBR", "ATPBR", "PMRNG", "PMASN", "AMASN", "AMRNG", "FMASN",
    "PNLASN", "PNLRNG", "COASN", "CORNG", "PNBASN", "PNBRNG", "EGBASN", "EGBRNG",
    "WLASN", "WLLML", "MCTUN", "SPTPNT", "EFSDLV_E1", "EFSDLV_E2",
}


def find_frame(data: bytes, tag: str) -> bytes | None:
    i = 0
    while i < len(data):
        if data[i] != 0xF0:
            i += 1
            continue
        j = i + 1
        while j < len(data) and data[j] != 0xF7:
            j += 1
        if j >= len(data):
            break
        frame = data[i : j + 1]
        if tag.encode() in frame:
            return frame
        i = j + 1
    return None


def ui_from_atpbr_sysex(raw: int) -> int:
    raw &= 0x7F
    if raw == 0:
        return 0
    if 1 <= raw <= 0x0C:
        return raw
    if 0x11 <= raw <= 0x1C:
        return 0x10 - raw
    return raw


def parse_lm0040_vc(frame: bytes) -> dict[str, Any]:
    out: dict[str, Any] = {"found0040": False, "fields": {}}
    if len(frame) < 102 or frame[10:16] != b"0040VC":
        return out
    out["found0040"] = True

    for name, off in K_0040.items():
        if off >= len(frame):
            continue
        raw = frame[off]
        entry: dict[str, Any] = {"offset": off, "raw": raw}
        if name == "ATPBR":
            entry["ui"] = ui_from_atpbr_sysex(raw)
        if name in CONFIRMED:
            entry["confirmed"] = True
        out["fields"][name] = entry

    return out


def format_log(parsed: dict[str, Any]) -> str:
    if not parsed.get("found0040"):
        return "LM0040=missing"
    parts = []
    for name in sorted(parsed["fields"], key=lambda n: K_0040.get(n, 0)):
        f = parsed["fields"][name]
        tag = "✅" if f.get("confirmed") else "⚠"
        s = f"{tag}{name}={f['raw']}@{f['offset']}"
        if "ui" in f:
            s += f"(ui={f['ui']})"
        parts.append(s)
    return " ".join(parts)


def main() -> None:
    for fn in sorted(FIX.glob("*_07x1_voice.syx")):
        f40 = find_frame(fn.read_bytes(), "0040VC")
        if f40 is None:
            print(f"{fn.name}: no 0040VC")
            continue
        p = parse_lm0040_vc(f40)
        print(f"{fn.name}: {format_log(p)}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Parse LM 0040VC group-02 tail (fixtures 01–03).

Encoding (confirmed fixtures 01–03, see lm_8101_offsets.md):
  Primary packed block @36–56 (duplicate @57–76 ignored).
  Tail block @90–101 (AFTMD / SPTPNT region).

Absolute offsets are from start of F0…F7 frame.
"""
from __future__ import annotations

from pathlib import Path
from typing import Any

FIX = Path(__file__).resolve().parent

# Absolute offsets — only entries marked confirmed in lm_8101_offsets.md
K_0040 = {
    "WPBR": 41,      # ✅ NN 0x28
    "ATPBR": 42,     # ⚠ candidate — sysex raw byte
    "PMRNG": 40,     # ⚠ candidate NN 0x2B
    "PMASN": 44,     # ⚠ candidate NN 0x2A
    "AMASN": 48,     # ⚠ candidate NN 0x2C
    "AMRNG": 50,     # ⚠ candidate NN 0x2D
    "FMASN": 47,     # ⚠ candidate NN 0x2E
    "FMRNG": 48,     # ⚠ shares byte with AMASN in capture — use 48 for FMRNG display only when distinct
    "PNLASN": 49,    # ⚠
    "PNLRNG": 50,    # ⚠
    "COASN": 51,     # ⚠
    "CORNG": 52,     # ⚠
    "PNBASN": 53,    # ⚠
    "PNBRNG": 54,    # ⚠
    "EGBASN": 46,    # ⚠
    "EGBRNG": 54,    # ⚠
    "WLASN": 56,     # ⚠ tail of primary
    "WLLML": 55,     # ✅ NN 0x39
    "MCTUN": 90,     # ⚠ tail — 0x63 on ANONIM/EP
    "RNDP": 52,      # ⚠ alias candidate only
    "AFTMD": 100,    # ⚠ tail — 0x7F on fixtures (needs diff)
    "SPTPNT": 98,    # ✅ NN 0x43
}

CONFIRMED = {"WPBR", "WLLML", "SPTPNT"}


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

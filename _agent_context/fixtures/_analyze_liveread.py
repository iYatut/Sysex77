#!/usr/bin/env python3
"""Analyze a LIVEREAD capture .syx file."""
from __future__ import annotations

import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent
sys.path.insert(0, str(FIX))

from _parse_0040vc import parse_lm0040_vc
from _validate_bulk_parse import find_lm8101_frame, parse_lm8101_vc


def iter_frames(data: bytes):
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
        fr = data[i : j + 1]
        tag = fr[10:16].decode("ascii", errors="replace") if len(fr) >= 16 else ""
        yield i, fr, tag
        i = j + 1


def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else None
    if path is None or not path.is_file():
        print("Usage: _analyze_liveread.py <path.syx>")
        return 1

    data = path.read_bytes()
    print(f"File: {path.name}")
    print(f"Size: {len(data)} bytes\n")

    frames = list(iter_frames(data))
    print(f"SysEx frames: {len(frames)}")
    for idx, (off, fr, tag) in enumerate(frames):
        hdr = " ".join(f"{b:02X}" for b in fr[: min(20, len(fr))])
        print(f"  [{idx}] off={off} len={len(fr)} tag={tag!r}")
        print(f"       {hdr}...")

    f8101 = find_lm8101_frame(data)
    if not f8101:
        print("\nNo 8101VC frame found.")
        return 1

    p = parse_lm8101_vc(f8101)
    print("\n=== 8101VC parsed ===")
    for k, v in p.items():
        print(f"  {k}: {v}")

    # WOL/ELVL selection (mirror C++ scoring)
    best = None
    best_score = 9999
    search_end = min(len(f8101) - 4, 130)
    for i in range(44, search_end + 1):
        prefix = f8101[i - 1]
        if prefix not in (0x03, 0x01):
            continue
        if f8101[i + 1] != 0x00 or f8101[i + 2] != 0x00:
            continue
        wol, elvl = f8101[i], f8101[i + 3]
        if wol > 127 or elvl > 127:
            continue
        if 93 <= i <= 96:
            score = (0 if prefix == 0x03 else 10) + (96 - i)
            if score < best_score:
                best_score = score
                best = (i, i + 3, wol, elvl)

    if best:
        wol_off, elvl_off, wol, elvl = best
        efln_delta = {94: 36, 93: 37, 95: 35}.get(wol_off, 36)
        efln_off = elvl_off + efln_delta
        outsel_off = elvl_off + 1
        print(f"\nAnchor: wol_off={wol_off} elvl_off={elvl_off} wol={wol} elvl={elvl}")
        print(f"outsel_off={outsel_off} outsel&0x06=0x{f8101[outsel_off] & 0x06:02X}")
        print(f"efln_off={efln_off} EFLN1EL=0x{f8101[efln_off]:02X}")
        end = min(efln_off + 4, len(f8101))
        chunk = f8101[elvl_off:end]
        names = ["ELVL", "OUTSEL pack", "+2", "+3", "+4"]
        for j in range(len(chunk)):
            nm = names[j] if j < len(names) else f"+{j}"
            if j == efln_off - elvl_off:
                nm = "EFLN1EL"
            elif j == efln_off - elvl_off + 1:
                nm = "EFSDLV?"
            print(f"  [{elvl_off + j}] 0x{chunk[j]:02X}  ({nm})")

    f40 = None
    for _, fr, tag in frames:
        if tag == "0040VC":
            f40 = fr
    print("\n=== 0040VC ===")
    if f40:
        p40 = parse_lm0040_vc(f40)
        print(f"  found: {p40.get('found0040')}")
        for name, info in sorted(p40.get("fields", {}).items()):
            print(f"  {name}: raw={info.get('raw')} off={info.get('offset')}")
    else:
        print("  (no frame)")

    print("\n=== Live 03 NN ... frames ===")
    count = 0
    for idx, (_, fr, _) in enumerate(frames):
        if len(fr) < 11:
            continue
        if fr[1:4] == bytes([0x43, 0x10, 0x34]) and fr[4] == 0x03:
            b4, b5, b6, b8 = fr[5], fr[6], fr[7], fr[9]
            addr = f"{b4:02X} {b5:02X} {b6:02X}"
            param = {0x00: "ELVL?", 0x08: "OUTSEL", 0x09: "EFLN1EL", 0x0A: "EFSDLV",
                     0x0B: "EFSDVSNS", 0x0C: "EFSDSCL"}.get(b6, f"0x{b6:02X}")
            print(f"  [{idx}] El.off={b4:02X} addr {addr} -> {param} VV=0x{b8:02X} ({b8})")
            count += 1
    if count == 0:
        print("  (none — bulk-only capture)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

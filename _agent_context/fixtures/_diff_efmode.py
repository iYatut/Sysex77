#!/usr/bin/env python3
"""Diff 0040VC frames to locate EFMODE bulk byte (expects VV 0/1/2 @+33)."""
from __future__ import annotations

import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent


def find_frame(data: bytes, tag: str = "0040VC") -> bytes | None:
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


def main() -> int:
    if len(sys.argv) < 4:
        print("Usage: _diff_efmode.py off.syx serial.syx parallel.syx")
        print("  Expect three 07:1 voice dumps with EFMODE Off / Serial / Parallel.")
        return 1

    frames: list[tuple[str, bytes]] = []
    for path_str in sys.argv[1:4]:
        path = Path(path_str)
        data = path.read_bytes()
        frame = find_frame(data)
        if frame is None:
            print(f"FAIL: no 0040VC in {path}")
            return 1
        frames.append((path.name, frame))

    a, b, c = (f[1] for f in frames)
    n = min(len(a), len(b), len(c))
    diffs: list[tuple[int, int, int, int]] = []
    for i in range(n):
        va, vb, vc = a[i], b[i], c[i]
        if va != vb or vb != vc:
            diffs.append((i, va, vb, vc))

    mode_triples = [(i, va, vb, vc) for i, va, vb, vc in diffs if {va, vb, vc} == {0, 1, 2}]
    print(f"0040VC frame sizes: {len(a)} {len(b)} {len(c)}")
    print(f"Total differing bytes: {len(diffs)}")
    print(f"Candidates with values {{0,1,2}} only: {len(mode_triples)}")
    for i, va, vb, vc in mode_triples[:20]:
        labels = ", ".join(f"{name}={v}" for name, v in zip((f[0] for f in frames), (va, vb, vc)))
        print(f"  @{i}: {labels}")
    if len(mode_triples) > 20:
        print(f"  ... and {len(mode_triples) - 20} more")

    if mode_triples and mode_triples[0][0] == 33:
        print("CONFIRMED: EFMODE bulk @ 0040VC +33")

    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Simulate dedup manifest repair: last 8101VC frame per mm must be valid F0..F7 slices."""

from __future__ import annotations

import json
import sys
from pathlib import Path

CAPTURE = Path(
    r"C:\Users\user\AppData\Roaming\Application Support\Sysex77\captures\AUTOSYNC-VC-INT.syx"
)


def iter_sysex_frames(data: bytes):
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
        yield i, j - i + 1, data[i : j + 1]
        i = j + 1


def read_mm(frame: bytes) -> int | None:
    p = frame.find(b"LM  8101VC")
    if p < 0 or p + 25 >= len(frame):
        return None
    return frame[p + 25]


def build_dedup_manifest(data: bytes, expected_mm: int = 64) -> list[dict]:
    best: dict[int, dict] = {}
    for off, ln, frame in iter_sysex_frames(data):
        if b"8101VC" not in frame:
            continue
        mm = read_mm(frame)
        if mm is None or mm < 0 or mm >= expected_mm:
            continue
        slot = {"mm": mm, "offset": off, "length": ln}
        if mm not in best or off > best[mm]["offset"]:
            best[mm] = slot
    return [best[i] for i in sorted(best)]


def slice_valid(data: bytes, off: int, ln: int) -> bool:
    if off < 0 or ln <= 0 or off + ln > len(data):
        return False
    return data[off] == 0xF0 and data[off + ln - 1] == 0xF7


def main() -> int:
    if not CAPTURE.exists():
        print(f"SKIP missing capture: {CAPTURE}")
        return 0

    data = CAPTURE.read_bytes()
    slots = build_dedup_manifest(data)
    print(f"dedup slots={len(slots)} fileBytes={len(data)}")

    failed = False
    for s in slots:
        ok = slice_valid(data, s["offset"], s["length"])
        if not ok:
            print(f"FAIL mm={s['mm']} off={s['offset']} len={s['length']}")
            failed = True

    s10 = next((x for x in slots if x["mm"] == 10), None)
    if s10 is None:
        print("FAIL mm=10 missing")
        failed = True
    else:
        frame = data[s10["offset"] : s10["offset"] + s10["length"]]
        elmode = frame[32] if len(frame) > 32 else -1
        name = frame[33:43].decode("latin1", errors="replace").rstrip("\x00 ")
        ok = elmode == 8 and "GrnDual" in name
        print(
            f"{'OK' if ok else 'FAIL'} mm=10 off={s10['offset']} len={s10['length']} "
            f"elmode={elmode} name={name!r}"
        )
        failed = failed or not ok

    idx = CAPTURE.with_suffix(".syx.idx")
    if idx.exists():
        manifest = json.loads(idx.read_text())
        bad = 0
        for slot in manifest.get("slots", []):
            if not slice_valid(data, slot["offset"], slot["length"]):
                bad += 1
        print(f"current idx invalid={bad}/{len(manifest.get('slots', []))}")

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

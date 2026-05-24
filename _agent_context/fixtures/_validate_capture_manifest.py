#!/usr/bin/env python3
"""Validate 8101VC manifest fields (name + elmode @ byte 32) against baseline .syx fixtures."""

from __future__ import annotations

import sys
from pathlib import Path

FIXTURES = Path(__file__).resolve().parent
VOICE_FIXTURES = [
    FIXTURES / "baseline_a1_voice.syx",
    FIXTURES / "baseline_b1_voice.syx",
    FIXTURES / "baseline_edit_voice.syx",
    FIXTURES / "baseline_ep_grndual.syx",
    FIXTURES / "baseline_sp_alaska.syx",
    FIXTURES / "baseline_anonim.syx",
]


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
        yield i, data[i : j + 1]
        i = j + 1


def frame_has_tag(frame: bytes, tag: str) -> bool:
    needle = tag.encode("ascii")
    return needle in frame


def read_name(frame: bytes) -> str:
    if len(frame) < 43:
        return ""
    chars = []
    for b in frame[33:43]:
        chars.append(chr(b) if 0x20 <= b < 0x7F else " ")
    return "".join(chars).rstrip()


def read_elmode(frame: bytes) -> int | None:
    if len(frame) <= 32:
        return None
    em = frame[32]
    return em if em <= 10 else None


def validate_file(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    voices = 0
    with_elmode = 0
    for _, frame in iter_sysex_frames(data):
        if not frame_has_tag(frame, "8101VC"):
            continue
        voices += 1
        name = read_name(frame)
        elmode = read_elmode(frame)
        if name and elmode is not None:
            with_elmode += 1
    return voices, with_elmode


def main() -> int:
    failed = False
    for path in VOICE_FIXTURES:
        if not path.exists():
            print(f"SKIP missing fixture: {path.name}")
            continue
        voices, with_elmode = validate_file(path)
        ok = voices >= 1 and with_elmode == voices
        status = "OK" if ok else "FAIL"
        print(f"{status} {path.name}: voices={voices} elmode={with_elmode}")
        failed = failed or not ok

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

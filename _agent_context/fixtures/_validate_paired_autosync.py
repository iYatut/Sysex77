#!/usr/bin/env python3
"""Validate interleaved AUTOSYNC .syx: 8101VC followed by 0040VC per slot + EFSDLV offsets."""

from __future__ import annotations

import re
import sys
from pathlib import Path

EFSDLV_E1_OFF = 100
EFSDLV_E2_OFF = 104

# 0040VC tail bytes from HW captures (Classic mm=05, NiteHwk mm=06)
FIXTURE_0040 = {
    "classic": {
        "mm": 0x00,
        "e1": 0x7F,
        "e2": 0x7F,
        "hex": (
            "F0 43 00 7A 00 6B 4C 4D 20 20 30 30 34 30 56 43 00 00 "
            "00 00 00 00 00 00 00 00 00 00 00 00 7F 00 06 02 00 00 "
            "19 00 08 00 0A 02 3F 01 47 00 04 00 37 00 07 00 06 00 "
            "14 64 64 00 00 19 00 08 00 0A 02 3F 01 47 00 04 00 37 "
            "00 07 00 06 00 14 2B 64 64 32 32 00 00 00 63 00 00 00 "
            "63 00 41 00 64 00 00 00 40 0F 7F 09 00 0F 7F 00 00 04 "
            "00 3F 04 00 3F 2E F7"
        ),
    },
    "nitehwk": {
        "mm": 0x06,
        "e1": 0x7F,
        "e2": 0x64,
        "hex": (
            "F0 43 00 7A 00 6B 4C 4D 20 20 30 30 34 30 56 43 00 00 "
            "00 00 00 00 00 00 00 00 00 00 00 00 7F 06 04 02 1F 00 "
            "12 00 48 02 09 00 12 00 18 00 08 03 73 00 05 00 00 00 "
            "10 64 64 2C 00 0E 00 07 00 0A 00 08 00 08 00 0A 00 0F "
            "00 57 00 28 00 64 27 64 07 64 64 07 06 05 14 00 00 00 "
            "63 00 00 63 64 00 00 00 3C 0F 7F 00 00 0F 64 00 00 00 "
            "00 00 00 00 00 78 F7"
        ),
    },
}


def parse_hex(text: str) -> bytes:
    return bytes(int(b, 16) for b in re.findall(r"[0-9A-Fa-f]{2}", text))


def split_syx(data: bytes) -> list[bytes]:
    frames: list[bytes] = []
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
        frames.append(data[i : j + 1])
        i = j + 1
    return frames


def frame_has_tag(frame: bytes, tag: str) -> bool:
    return tag.encode("ascii") in frame


def find_paired_0040_after(frames: list[bytes], after_index: int) -> bytes | None:
    """Mirror C++ findPaired0040FrameAfter: next 0040VC before next 8101VC."""
    for idx in range(after_index + 1, len(frames)):
        fr = frames[idx]
        if frame_has_tag(fr, "0040VC"):
            return fr
        if frame_has_tag(fr, "8101VC"):
            break
    return None


def build_synthetic_interleaved() -> bytes:
    stub8101 = bytes([0xF0, 0x43, 0x00, 0x7A]) + b"\x00" * 20 + b"8101VC" + b"\x00" * 50 + bytes([0xF7])
    out = bytearray()
    for key in ("classic", "nitehwk"):
        f0040 = parse_hex(FIXTURE_0040[key]["hex"])
        out.extend(stub8101)
        out.extend(f0040)
    return bytes(out)


def validate_interleaved(data: bytes) -> tuple[bool, list[str]]:
    frames = split_syx(data)
    errors: list[str] = []
    slot = 0
    i = 0
    while i < len(frames):
        fr = frames[i]
        if not frame_has_tag(fr, "8101VC"):
            i += 1
            continue
        paired = find_paired_0040_after(frames, i)
        if paired is None:
            errors.append(f"slot {slot}: 8101 at frame {i} has no paired 0040VC")
        elif len(paired) < EFSDLV_E2_OFF + 1:
            errors.append(f"slot {slot}: 0040VC too short ({len(paired)} B)")
        i += 1
        if paired is not None:
            i += 1
        slot += 1
    ok = len(errors) == 0 and slot > 0
    return ok, errors


def validate_fixture_efsdlv(name: str) -> tuple[bool, str]:
    spec = FIXTURE_0040[name]
    frame = parse_hex(spec["hex"])
    if len(frame) < EFSDLV_E2_OFF + 1:
        return False, f"{name}: frame too short"
    e1 = frame[EFSDLV_E1_OFF]
    e2 = frame[EFSDLV_E2_OFF]
    if e1 != spec["e1"] or e2 != spec["e2"]:
        return False, f"{name}: EFSDLV {e1}/{e2} != expected {spec['e1']}/{spec['e2']}"
    return True, f"{name}: EFSDLV {e1}/{e2} OK"


def main() -> int:
    failed = 0
    for name in FIXTURE_0040:
        ok, msg = validate_fixture_efsdlv(name)
        print(("PASS" if ok else "FAIL") + " " + msg)
        if not ok:
            failed += 1

    synth = build_synthetic_interleaved()
    ok, errors = validate_interleaved(synth)
    if ok:
        print(f"PASS interleaved synthetic syx: {len(split_syx(synth))} frames")
    else:
        failed += 1
        print("FAIL interleaved synthetic syx:")
        for e in errors:
            print("  " + e)

    if len(sys.argv) > 1:
        path = Path(sys.argv[1])
        if path.is_file():
            data = path.read_bytes()
            ok, errors = validate_interleaved(data)
            if ok:
                n8101 = sum(1 for f in split_syx(data) if frame_has_tag(f, "8101VC"))
                n0040 = sum(1 for f in split_syx(data) if frame_has_tag(f, "0040VC"))
                print(f"PASS {path.name}: {n8101}x8101 {n0040}x0040")
            else:
                failed += 1
                print(f"FAIL {path.name}:")
                for e in errors:
                    print("  " + e)

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())

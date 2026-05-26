#!/usr/bin/env python3
"""
Regression: SYM7 slot address (byte28, mm) must use LM+24 scan, not F0+24.

Mirrors readLmSym7SlotAddressFromFrame / read8101AddressFromFrame / sy99ReadSym78101AddressFromSysexBody.
Exit 0 if all PASS, 1 otherwise.
"""
from __future__ import annotations

import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent
K_LM_TAG_OFFSET = 4


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
        yield data[i : j + 1]
        i = j + 1


def read_sym7_slot_address(frame: bytes, tag6: bytes) -> tuple[int, int] | None:
    if len(frame) < 26:
        return None
    for i in range(0, len(frame) - 25):
        if frame[i : i + 2] != b"LM":
            continue
        if frame[i + K_LM_TAG_OFFSET : i + K_LM_TAG_OFFSET + 6] != tag6:
            continue
        addr = i + 24
        if addr + 1 >= len(frame):
            return None
        return frame[addr], frame[addr + 1]
    return None


def read_wrong_f0_plus_24(frame: bytes, tag6: bytes) -> tuple[int, int] | None:
    if b"LM" not in frame or tag6 not in frame:
        return None
    if len(frame) < 26:
        return None
    return frame[24], frame[25]


def dedupe_by_mm(frames: list[bytes], tag6: bytes, byte28: int) -> list[bytes]:
    best_by_mm: dict[int, tuple[int, bytes]] = {}
    for frame in frames:
        addr = read_sym7_slot_address(frame, tag6)
        if addr is None:
            continue
        b28, mm = addr
        if b28 != byte28:
            continue
        score = len(frame)
        prev = best_by_mm.get(mm)
        if prev is None or score > prev[0]:
            best_by_mm[mm] = (score, frame)
    return [best_by_mm[k][1] for k in sorted(best_by_mm)]


def validate_single_frame_baseline(path: Path, tag6: bytes) -> bool:
    data = path.read_bytes()
    frames = [f for f in iter_sysex_frames(data) if tag6 in f]
    if not frames:
        print(f"FAIL {path.name}: no {tag6.decode()} frames")
        return False

    frame = frames[0]
    correct = read_sym7_slot_address(frame, tag6)
    wrong = read_wrong_f0_plus_24(frame, tag6)

    if correct is None:
        print(f"FAIL {path.name}: LM+24 address not found")
        return False

    if wrong is not None and wrong == correct:
        print(f"FAIL {path.name}: old F0+24 matches LM+24 (fix not testable)")
        return False

    print(
        f"PASS {path.name}: LM+24 b28={correct[0]} mm={correct[1]}"
        f" (old F0+24={wrong})"
    )
    return True


def validate_file(path: Path, tag6: bytes, byte28: int, min_distinct_mm: int) -> bool:
    data = path.read_bytes()
    frames = [f for f in iter_sysex_frames(data) if tag6 in f]
    if not frames:
        print(f"FAIL {path.name}: no {tag6.decode()} frames")
        return False

    mm_set: set[int] = set()
    wrong_collapses = 0
    for frame in frames:
        correct = read_sym7_slot_address(frame, tag6)
        wrong = read_wrong_f0_plus_24(frame, tag6)
        if correct is None:
            continue
        mm_set.add(correct[1])
        if wrong is not None and wrong[1] == 0 and correct[1] != 0:
            wrong_collapses += 1

    deduped = dedupe_by_mm(frames, tag6, byte28)
    ok = True

    if len(mm_set) < min_distinct_mm:
        print(
            f"FAIL {path.name}: distinct mm={len(mm_set)} expected>={min_distinct_mm} tag={tag6.decode()}"
        )
        ok = False

    if len(deduped) < min_distinct_mm:
        print(
            f"FAIL {path.name}: dedupe kept {len(deduped)} frames expected>={min_distinct_mm}"
        )
        ok = False

    if wrong_collapses > 0 and len(mm_set) > 1:
        print(
            f"INFO {path.name}: old F0+24 would collapse {wrong_collapses} non-zero mm slots to mm=0"
        )

    if ok:
        print(
            f"PASS {path.name}: frames={len(frames)} distinct_mm={len(mm_set)} deduped={len(deduped)}"
        )
    return ok


def main() -> int:
    all_ok = True

    for path in (FIX / "baseline_ep_grndual.syx", FIX / "baseline_a1_voice.syx"):
        if not path.is_file():
            print(f"SKIP {path.name}: not found")
            continue
        if not validate_single_frame_baseline(path, b"8101VC"):
            all_ok = False

    captures = Path.home() / "AppData/Roaming/Application Support/Sysex77/captures"
    vc_p2 = captures / "AUTOSYNC-VC-P2.syx"
    if vc_p2.is_file():
        if not validate_file(vc_p2, b"8101VC", 0x02, 16):
            all_ok = False
    else:
        print("SKIP AUTOSYNC-VC-P2.syx: not found (optional multi-frame gate)")

    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())

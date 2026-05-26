#!/usr/bin/env python3
"""T0: EFLN El.1 parser + El.2 anchor+12 scan on AUTOSYNC 8101VC frames."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent
sys.path.insert(0, str(FIX))

from _validate_bulk_parse import (
    efln1_el_raw_from_mixer_strip,
    find_first_anchor,
    find_wol_elvl_e1_pair,
    parse_lm8101_vc,
)

DEFAULT_CAPTURE = Path.home() / "AppData/Roaming/Application Support/Sysex77/captures/AUTOSYNC-VC-INT.syx"

VALID_EFLN = {0, 0x01, 0x04, 0x05}


def efln_wire_masked(raw: int) -> int:
    b = raw & 0xFF
    ui = b & 0x05
    if b & 0x02:
        ui |= 0x01
    return ui & 0x05


def iter_8101_frames_by_slot(data: bytes) -> list[bytes]:
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
        frame = data[i : j + 1]
        if b"8101VC" in frame:
            frames.append(frame)
        i = j + 1
    return frames


def sweep_el1(frame: bytes, expected: int | None) -> dict:
    pair = find_wol_elvl_e1_pair(frame)
    if pair is None:
        return {"error": "no wol/elvl pair"}

    wol_off, elvl_off, wol, elvl = pair
    parser_val = efln1_el_raw_from_mixer_strip(frame, elvl_off, elvl)

    hits: list[tuple[int, int, int]] = []
    for delta in range(28, 49):
        off = elvl_off + delta
        if off >= len(frame):
            continue
        raw = frame[off]
        masked = efln_wire_masked(raw)
        if masked in VALID_EFLN:
            hits.append((delta, off, masked))

    best = None
    if expected is not None:
        for delta, off, masked in hits:
            if masked == (expected & 0x05):
                best = (delta, off, masked)
                break

    anchor = find_first_anchor(frame)
    el2 = None
    if anchor is not None and anchor + 12 < len(frame):
        el2 = {
            "anchor": anchor,
            "off": anchor + 12,
            "raw": frame[anchor + 12],
            "masked": efln_wire_masked(frame[anchor + 12]),
        }

    return {
        "wol_off": wol_off,
        "elvl_off": elvl_off,
        "wol": wol,
        "elvl": elvl,
        "parser_val": parser_val,
        "expected": expected,
        "best": best,
        "hits": hits,
        "anchor_el2": el2,
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--capture", type=Path, default=DEFAULT_CAPTURE)
    ap.add_argument("--slots", default="5,7,28,40")
    ap.add_argument("--expected-el1", type=int, default=0)
    ap.add_argument("--expected-el2", type=int, default=0)
    args = ap.parse_args()

    if not args.capture.is_file():
        print(f"FAIL: capture not found: {args.capture}")
        return 1

    data = args.capture.read_bytes()
    by_slot = iter_8101_frames_by_slot(data)
    slots = [int(s.strip()) for s in args.slots.split(",") if s.strip()]

    print(f"Capture: {args.capture} ({len(data)} bytes, {len(by_slot)} x 8101VC)")
    print(f"Expected El.1 EFLN wire: {args.expected_el1 & 0x05}, El.2: {args.expected_el2 & 0x05}\n")

    any_fail = False
    for slot in slots:
        frame = by_slot[slot] if 0 <= slot < len(by_slot) else None
        if frame is None:
            print(f"=== slot {slot} (mm={slot:02X}) === MISSING FRAME")
            any_fail = True
            continue

        parsed = parse_lm8101_vc(frame)
        sw = sweep_el1(frame, args.expected_el1)
        name = parsed.get("name", "?")
        print(f"=== slot {slot} mm={slot:02X} name={name!r} elmode={parsed.get('elmodeRaw')} ===")
        if "error" in sw:
            print(f"  ERROR: {sw['error']}")
            any_fail = True
            continue

        print(
            f"  WOL={sw['wol']}@{sw['wol_off']} ELVL={sw['elvl']}@{sw['elvl_off']} "
            f"parser_val={sw['parser_val']}"
        )
        exp = sw["expected"] if sw["expected"] is not None else -1
        el1_ok = sw["parser_val"] == (exp & 0x05)
        print(f"  El.1 parser vs expected: {'PASS' if el1_ok else 'FAIL'}")
        if not el1_ok:
            any_fail = True

        if sw["best"]:
            bd, bo, bv = sw["best"]
            print(f"  El.1 best single-byte match: delta={bd} off={bo} val={bv}")
        else:
            print(f"  El.1 best single-byte match: NONE among {len(sw['hits'])} hits")

        if sw["anchor_el2"]:
            e2 = sw["anchor_el2"]
            e2_ok = e2["masked"] == (args.expected_el2 & 0x05)
            print(
                f"  El.2 anchor+12: off={e2['off']} raw=0x{e2['raw']:02X} masked={e2['masked']} "
                f"{'PASS' if e2_ok else 'FAIL (use reconcile)'}"
            )
        else:
            print("  El.2 anchor+12: n/a")
        print()

    return 1 if any_fail else 0


if __name__ == "__main__":
    raise SystemExit(main())

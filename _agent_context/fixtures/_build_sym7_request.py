#!/usr/bin/env python3
"""Build SYM7-style SY99 bulk REQUEST (31 B). No hardware needed.

Usage:
  python _build_sym7_request.py 0040VC 0 5      # A6 0040 tail
  python _build_sym7_request.py 8101VC 0 0      # A1 8101
  python _build_sym7_request.py 8101VC 2 10     # PRE1 B1 (b28=0x02)
  python _build_sym7_request.py --slot A11 0040VC
"""
from __future__ import annotations

import argparse
import sys

DEVICE_ID = 0  # SY99 Utility SysEx device number (SYM7 capture used 0 → 43 20)


def sym7_lm_request(lm_type6: str, byte28: int, mm: int, device_id: int = DEVICE_ID) -> bytes:
    if len(lm_type6) != 6:
        raise ValueError(f"lm_type6 must be 6 chars, got {lm_type6!r}")
    if not (0 <= byte28 <= 0xFF):
        raise ValueError("byte28 out of range")
    if not (0 <= mm <= 0x7F):
        raise ValueError("mm out of range")
    if not (0 <= device_id <= 0x0F):
        raise ValueError("device_id out of range")

    body = bytearray()
    body.append(0xF0)
    body.append(0x43)
    body.append(0x20 | (device_id & 0x0F))
    body.append(0x7A)
    body.extend(b"LM  ")
    body.extend(lm_type6.encode("ascii"))
    body.extend((0x00, 0x00))
    body.extend(bytes(12))
    body.append(byte28 & 0xFF)
    body.append(mm & 0x7F)
    body.append(0xF7)
    if len(body) != 31:
        raise AssertionError(f"expected 31 bytes, got {len(body)}")
    return bytes(body)


def slot_label_to_mm(label: str) -> int:
    label = label.strip().upper()
    if len(label) < 2:
        raise ValueError(f"bad slot label {label!r}")
    bank = "ABCD".index(label[0])
    num = int(label[1:], 10)
    if not (1 <= num <= 16):
        raise ValueError(f"voice number 1..16, got {num}")
    return bank * 16 + (num - 1)


def fmt_hex(data: bytes, wrap: int = 18) -> str:
    parts = [f"{b:02X}" for b in data]
    lines = []
    for i in range(0, len(parts), wrap):
        lines.append(" ".join(parts[i : i + wrap]))
    return "\n".join(lines)


# Catalog: (type, b28, mm range, notes) — enough to form all library sync TX without per-patch capture
SYM7_REQUEST_CATALOG = """
| Use case              | lm_type6 | b28  | mm        |
|-----------------------|----------|------|-----------|
| Internal voice 8101   | 8101VC   | 0x00 | 0..63     |
| Internal voice 0040   | 0040VC   | 0x00 | 0..63     |
| Preset1 voice         | 8101VC   | 0x02 | 0..63     |
| Preset2 voice         | 8101VC   | 0x03 | 0..63     |
| System setup          | 8101SY   | 0x00 | 0x00      |
| Multi header          | 0040MU   | 0x00 | 0x00      |
| Multi program         | 8101MU   | 0x00 | 0..15     | Sysex77 fast sync |
| **Multi (0040 tail)** | **0040MU** | **0x00** | **0..15** | **SYM7 extended ✅** |
| Pan                   | 8101PN   | 0x00 | 0..31     | SYM7 ×32 |
| Microtuning           | 8101MT   | 0x00 | 0..1      | SYM7 ×2 |
| Sample/pan/etc.       | 0040SA…  | 0x00 | see sync  |
"""


def main() -> int:
    p = argparse.ArgumentParser(description="SYM7-style SY99 bulk request builder")
    p.add_argument("lm_type6", nargs="?", help="e.g. 8101VC, 0040VC, 8101SY")
    p.add_argument("byte28", nargs="?", type=lambda x: int(x, 0), help="bank/preset byte (0 internal)")
    p.add_argument("mm", nargs="?", type=lambda x: int(x, 0), help="slot mm 0..63")
    p.add_argument("--slot", help="A1..D16 (sets mm, b28=0)")
    p.add_argument("--device", type=lambda x: int(x, 0), default=DEVICE_ID)
    p.add_argument("--catalog", action="store_true", help="print request catalog")
    args = p.parse_args()

    if args.catalog:
        print(SYM7_REQUEST_CATALOG.strip())
        return 0

    if args.lm_type6 is None:
        p.print_help()
        return 1

    b28 = args.byte28 if args.byte28 is not None else 0
    if args.slot:
        mm = slot_label_to_mm(args.slot)
        b28 = 0
    elif args.mm is not None:
        mm = args.mm
    else:
        mm = 0

    req = sym7_lm_request(args.lm_type6.upper(), b28, mm, args.device)
    print(fmt_hex(req))
    print(f"# len={len(req)} type={args.lm_type6.upper()} b28=0x{b28:02X} mm=0x{mm:02X} ({mm})")
    return 0


if __name__ == "__main__":
    sys.exit(main())

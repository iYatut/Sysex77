#!/usr/bin/env python3
from pathlib import Path

FIX = Path(__file__).resolve().parent


def load8101(p):
    d = p.read_bytes()
    i = 0
    while i < len(d):
        if d[i] != 0xF0:
            i += 1
            continue
        j = i + 1
        while d[j] != 0xF7:
            j += 1
        fr = d[i : j + 1]
        if b"8101VC" in fr:
            return fr
        i = j + 1


def find_wol(frame):
    for i in range(44, min(len(frame) - 4, 130)):
        if frame[i - 1] in (0x03, 0x01) and frame[i + 1] == 0 and frame[i + 2] == 0:
            if 93 <= i <= 96:
                return i
    return None


for fn in sorted(FIX.glob("0*.syx")):
    fr = load8101(fn)
    wol = find_wol(fr)
    print(f"\n=== {fn.name} wol@{wol} ===")
    if wol is None:
        continue
    for rel in range(-12, 0):
        off = wol + rel
        if off < 0:
            continue
        b = fr[off]
        print(f"  wol{rel:+d} @{off}: 0x{b:02X} outsel=0x{b & 0x06:02X}")

    # first anchor
    for a in range(98, min(len(fr) - 6, 125)):
        if fr[a] == 0x7F and fr[a + 1] == 0x01 and fr[a + 2] == 0x7F:
            print(f"  anchor@{a} +3=0x{fr[a+3]:02X} +4=0x{fr[a+4]:02X} (outsel=0x{fr[a+4]&0x06:02X})")
            break

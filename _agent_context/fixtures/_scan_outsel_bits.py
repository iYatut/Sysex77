#!/usr/bin/env python3
from pathlib import Path

def load8101(p):
    d = Path(p).read_bytes()
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
    return None

FIX = Path(__file__).resolve().parent
for fn in sorted(FIX.glob("0*.syx")):
    fr = load8101(fn)
    print(f"=== {fn.name} elmode={fr[32]} ===")
    for off in range(80, 102):
        b = fr[off]
        outsel = b & 0x06
        mark = ""
        if outsel in (0, 2, 4, 6):
            mark = f" OUTSEL_bits=0x{outsel:02X}"
        print(f"  @{off}: 0x{b:02X}{mark}")

# synthesize diff: patch fixture01 @99 from 0x40 to test - user needs real dumps
# Check if @99 has OUTSEL: both=0x06 would need full byte; @99=0x40 has bits 0x00 for outsel
print("\nNote: @99=0x40 -> OUTSEL bits =", hex(0x40 & 0x06))

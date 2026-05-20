#!/usr/bin/env python3
"""Parse 05:64 ANONIM bank dump from agent transcript hex."""
from pathlib import Path

# From user monitor capture (1 AFM mono, G1 OUTSEL per roadmap)
HEX = """
F0 43 0 7A 3 4A 4C 4D 20 20 38 31 30 31 56 43 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
41 4E 4F 4E 49 4D 20 20 20 20 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
2 0 1 40 C 40 1 0 D 40 C 0 A 0 2 0 E 0 2 3 1 0 3 7F 0 0 6E 0 40 0 7F 1 7F 20 6 37 3F 3F 3F A A 3F 3F 3F 3F 0 0 3 3F 0 0 0 0 0 0 0 6 3F 0 0 E 1 0 0
""".replace("\n", " ")


def hx(s: str) -> bytes:
    return bytes(int(x, 16) for x in s.split())


data = hx(HEX)
print("len", len(data))
elmode = data[32] if len(data) > 32 else -1
print("elmode@32", elmode)

for a in range(50, min(len(data) - 6, 120)):
    if data[a] == 0x7F and data[a + 1] == 0x01 and data[a + 2] == 0x7F:
        chunk = data[a : a + 8]
        print(f"anchor@{a}: {' '.join(f'{b:02X}' for b in chunk)} +4={data[a+4]:02X}")

for off in range(80, min(110, len(data))):
    b = data[off]
    if (b & 0x06) in (0, 2, 4, 6):
        print(f"@{off}: 0x{b:02X} outsel_bits=0x{b & 0x06:02X}")

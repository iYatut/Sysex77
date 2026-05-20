#!/usr/bin/env python3
from pathlib import Path

FIX = Path(__file__).resolve().parent

def find8101(data):
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
            return frame
        i = j + 1
    return None

for f in sorted(FIX.glob("0*.syx")):
    frame = find8101(f.read_bytes())
    print(f"\n=== {f.name} ===")
    print(" ".join(f"{i:3d}:{frame[i]:02X}" for i in range(80, 120)))
    anchors = []
    for a in range(80, min(len(frame)-6, 130)):
        if frame[a]==0x7F and frame[a+1]==0x01 and frame[a+2]==0x7F:
            anchors.append(a)
    print("anchors:", anchors)
    for a in anchors:
        print(f"  @{a}: {' '.join(f'{frame[a+k]:02X}' for k in range(8))}")

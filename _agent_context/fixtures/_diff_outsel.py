#!/usr/bin/env python3
"""Find OUTSEL El.1 offset: diff both(0x06) vs G1(0x02) in 8101VC."""
from pathlib import Path

FIX = Path(__file__).resolve().parent


def find8101(data: bytes) -> bytes | None:
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


def analyze_frame(name: str, frame: bytes) -> None:
    print(f"=== {name} len={len(frame)} ===")
    for a in range(98, min(len(frame) - 6, 125)):
        if frame[a] == 0x7F and frame[a + 1] == 0x01 and frame[a + 2] == 0x7F:
            chunk = frame[a : a + 8]
            hx = " ".join(f"{b:02X}" for b in chunk)
            print(f"  anchor@{a}: {hx}  +4={frame[a+4]:02X} +5={frame[a+5]:02X}")
            break
    for v, label in ((0x06, "both"), (0x02, "G1"), (0x04, "G2"), (0x00, "off")):
        hits = [i for i in range(80, 120) if frame[i] == v]
        if hits:
            print(f"  {label} 0x{v:02X} in 80-119: {hits}")


def main() -> None:
    for f in sorted(FIX.glob("*.syx")):
        frame = find8101(f.read_bytes())
        if frame:
            analyze_frame(f.name, frame)

    both = FIX / "04_anonim_outsel_both.syx"
    g1 = FIX / "05_anonim_outsel_g1.syx"
    if both.is_file() and g1.is_file():
        fa, fb = find8101(both.read_bytes()), find8101(g1.read_bytes())
        if fa and fb:
            n = min(len(fa), len(fb))
            diffs = [(i, fa[i], fb[i]) for i in range(n) if fa[i] != fb[i]]
            print(f"\n=== DIFF both vs G1 ({len(diffs)} bytes) ===")
            for i, a, b in diffs:
                print(f"  @{i}: 0x{a:02X} -> 0x{b:02X}")


if __name__ == "__main__":
    main()

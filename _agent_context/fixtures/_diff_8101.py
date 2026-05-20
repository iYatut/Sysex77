#!/usr/bin/env python3
"""Diff 8101VC frames; find OUTSEL both(0x06)->G1(0x02) candidates."""
from pathlib import Path

FIX = Path(__file__).resolve().parent


def load8101(path: Path) -> bytes | None:
    data = path.read_bytes()
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


def diff_frames(a: bytes, b: bytes) -> list[tuple[int, int, int]]:
    n = min(len(a), len(b))
    return [(i, a[i], b[i]) for i in range(n) if a[i] != b[i]]


def outsel_candidates(diffs: list[tuple[int, int, int]]) -> list[tuple[int, int, int]]:
    out = []
    for i, va, vb in diffs:
        if (va & 0x06) == 0x06 and (vb & 0x06) == 0x02 and (va & ~0x06) == (vb & ~0x06):
            out.append((i, va, vb))
    return out


def main() -> None:
    both = FIX / "04_anonim_outsel_both.syx"
    g1 = FIX / "05_anonim_outsel_g1.syx"

    if not both.is_file() or not g1.is_file():
        print("Missing 04/05 fixtures — synthesize from 01 for dry-run")
        base = load8101(FIX / "01_init_anonim_07x1_voice.syx")
        if base is None:
            return
        # brute: try each offset 80-120 where (b&0x06)==0x06 -> patch to G1
        for off in range(80, min(len(base), 121)):
            b = base[off]
            if (b & 0x06) != 0x06:
                continue
            patched = bytearray(base)
            patched[off] = (b & ~0x06) | 0x02
            diffs = diff_frames(base, bytes(patched))
            if len(diffs) == 1:
                print(f"  single-byte patch @{off}: 0x{b:02X} -> 0x{patched[off]:02X}")
        return

    fa, fb = load8101(both), load8101(g1)
    diffs = diff_frames(fa, fb)
    print(f"Total diffs: {len(diffs)}")
    for row in diffs:
        print(f"  @{row[0]}: 0x{row[1]:02X} -> 0x{row[2]:02X}")
    cand = outsel_candidates(diffs)
    print("OUTSEL both->G1 candidates:", cand)


if __name__ == "__main__":
    main()

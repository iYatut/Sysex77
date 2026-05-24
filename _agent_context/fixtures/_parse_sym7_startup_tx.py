#!/usr/bin/env python3
"""Parse SYM7 startup TX log (02_startup_full_20260523.txt)."""
from __future__ import annotations

from collections import defaultdict
from pathlib import Path


def read_frames(path: Path) -> list[tuple[int, bytes]]:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    frames: list[tuple[int, bytes]] = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if "System Exclusive" in line and "31 Bytes" in line:
            ts = int(line.split()[0], 16)
            if (
                i + 2 < len(lines)
                and lines[i + 1].startswith(" SYSX:")
                and lines[i + 2].startswith(" SYSX:")
            ):
                h1 = lines[i + 1].split(":", 1)[1].split()
                h2 = lines[i + 2].split(":", 1)[1].split()
                fr = bytes(int(x, 16) for x in (h1 + h2))
                if len(fr) == 31:
                    frames.append((ts, fr))
                i += 3
                continue
        i += 1
    return frames


def lm6(fr: bytes) -> str:
    return fr[10:16].decode("ascii", "replace")


def main() -> None:
    path = Path(__file__).resolve().parent / "sym7_captures" / "02_startup_full_20260523.txt"
    frames = read_frames(path)
    print(f"frames: {len(frames)}")

    groups: list[dict] = []
    cur: dict | None = None
    for ts, fr in frames:
        key = (lm6(fr), fr[27], fr[28])
        if cur is None or cur["key"] != key:
            cur = {"key": key, "count": 0, "mms": [], "first_ts": ts, "last_ts": ts}
            groups.append(cur)
        cur["count"] += 1
        cur["mms"].append(fr[29])
        cur["last_ts"] = ts

    print("\n=== Phase groups (SYM7 TX order) ===")
    for gi, g in enumerate(groups):
        lt, b27, b28 = g["key"]
        mms = g["mms"]
        dt = g["last_ts"] - g["first_ts"]
        avg = dt / max(1, g["count"] - 1)
        print(
            f"{gi + 1:2d}. {lt:8s} b27={b27:02X} b28={b28:02X} "
            f"n={g['count']:3d} mm={mms[0]:02X}..{mms[-1]:02X} avg_gap={avg:.0f}"
        )

    vc = [(ts, fr) for ts, fr in frames if lm6(fr) == "8101VC" and fr[28] == 0]
    if len(vc) > 5:
        d = [vc[i + 1][0] - vc[i][0] for i in range(min(64, len(vc) - 1))]
        ds = sorted(d)
        print(
            f"\n8101VC b28=0 gaps (ticks): min={min(d)} med={ds[len(ds) // 2]} "
            f"max={max(d)} mean={sum(d)/len(d):.0f} n={len(d)}"
        )

    print("\n=== First 35 requests ===")
    for i, (ts, fr) in enumerate(frames[:35]):
        print(f"  {i:2d} {lm6(fr):8s} b28={fr[28]:02X} mm={fr[29]:02X}")


if __name__ == "__main__":
    main()

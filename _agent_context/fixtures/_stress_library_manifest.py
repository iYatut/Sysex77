#!/usr/bin/env python3
"""Simulate 100-bank library listing cost: manifest read is O(1) per active bank."""

from __future__ import annotations

import json
import shutil
import sys
import tempfile
import time
from pathlib import Path

FIXTURE = Path(__file__).resolve().parent / "baseline_a1_voice.syx"


def write_manifest(syx: Path) -> None:
    data = syx.read_bytes()
    slots = []
    mm = 0
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
        if b"8101VC" in frame and len(frame) > 43:
            name = "".join(
                chr(b) if 0x20 <= b < 0x7F else " "
                for b in frame[33:43]
            ).rstrip()
            elmode = frame[32] if frame[32] <= 10 else -1
            slots.append(
                {
                    "mm": mm,
                    "name": name,
                    "elmodeRaw": elmode,
                    "offset": i,
                    "length": len(frame),
                    "tag": "8101VC",
                }
            )
            mm += 1
        i = j + 1

    sidecar = syx.with_suffix(".syx.idx")
    payload = {
        "version": 1,
        "sourceModTimeMs": int(syx.stat().st_mtime * 1000),
        "captureFile": syx.name,
        "slots": slots,
    }
    sidecar.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def main() -> int:
    if not FIXTURE.exists():
        print(f"FAIL missing fixture {FIXTURE}")
        return 1

    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        banks: list[Path] = []
        t0 = time.perf_counter()
        for n in range(100):
            dest = root / f"bank_{n:03d}.syx"
            shutil.copy2(FIXTURE, dest)
            write_manifest(dest)
            banks.append(dest)
        list_ms = (time.perf_counter() - t0) * 1000

        t1 = time.perf_counter()
        for bank in banks[:20]:
            sidecar = bank.with_suffix(".syx.idx")
            json.loads(sidecar.read_text(encoding="utf-8"))
        read_ms = (time.perf_counter() - t1) * 1000

    print(f"OK created 100 banks + manifests in {list_ms:.1f} ms")
    print(f"OK read 20 manifests in {read_ms:.1f} ms")
    return 0


if __name__ == "__main__":
    sys.exit(main())

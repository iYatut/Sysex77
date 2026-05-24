#!/usr/bin/env python3
"""Analyze Sysex77_MIDI.log for library sync session."""
from __future__ import annotations

import re
from collections import Counter
from datetime import datetime
from pathlib import Path

LOG = Path(r"c:\projects\sy_99\Sysex77-master\Builds\VisualStudio2022\x64\Debug\App\Sysex77_MIDI.log")


def parse_sysex_line(line: str):
    m = re.search(
        r"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}) \[(TX|RX)\].*?(F0(?: [0-9A-F]{2})+ F7)",
        line,
        re.I,
    )
    if not m:
        return None
    ts = datetime.strptime(m.group(1), "%Y-%m-%d %H:%M:%S.%f")
    kind = m.group(2)
    data = bytes(int(x, 16) for x in m.group(3).split())
    if len(data) < 31 or data[0] != 0xF0:
        return None
    lm = data[8:14].decode("ascii", "replace")
    b28 = data[28]
    mm = data[29]
    return ts, kind, lm, b28, mm, len(data)


def phase_key(lm: str, b28: int) -> str:
    return f"{lm} b28={b28:02X}"


def main() -> None:
    rows = []
    session_starts: list[tuple[str, datetime | None]] = []
    for line in LOG.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith("--- sysex session"):
            session_starts.append((line.strip(), None))
        p = parse_sysex_line(line)
        if p:
            rows.append(p)
            if session_starts and session_starts[-1][1] is None:
                session_starts[-1] = (session_starts[-1][0], p[0])

    tx = [r for r in rows if r[1] == "TX"]
    rx = [r for r in rows if r[1] == "RX"]

    print("=== Sessions ===")
    for label, ts in session_starts:
        print(f"  {label}" + (f"  first MIDI @ {ts}" if ts else ""))

    # Focus: last session (17:10 extended sync)
    last_sess_ts = session_starts[-1][1] if session_starts else None
    if last_sess_ts:
        tx_s = [r for r in tx if r[0] >= last_sess_ts]
        rx_s = [r for r in rx if r[0] >= last_sess_ts]
    else:
        tx_s, rx_s = tx, rx

    print(f"\n=== Last session (from {last_sess_ts}) ===")
    print(f"TX={len(tx_s)}  RX={len(rx_s)}")
    if tx_s:
        dur = (tx_s[-1][0] - tx_s[0][0]).total_seconds()
        print(f"Duration: {dur/60:.1f} min  ({tx_s[0][0].time()} .. {tx_s[-1][0].time()})")

    # Phase groups in TX order
    groups: list[dict] = []
    cur = None
    for r in tx_s:
        k = phase_key(r[2], r[3])
        if cur is None or cur["k"] != k:
            cur = {"k": k, "n": 0, "mms": [], "gaps": [], "last_ts": r[0]}
            groups.append(cur)
        else:
            cur["gaps"].append((r[0] - cur["last_ts"]).total_seconds() * 1000)
        cur["n"] += 1
        cur["mms"].append(r[4])
        cur["last_ts"] = r[0]

    print("\n=== TX phases (last session) ===")
    for i, g in enumerate(groups):
        mms = g["mms"]
        avg = sum(g["gaps"]) / len(g["gaps"]) if g["gaps"] else 0
        print(
            f"  {i+1:2d}. {g['k']:16s} n={g['n']:3d} "
            f"mm={mms[0]:02X}..{mms[-1]:02X}  avg_gap={avg:.0f}ms"
        )

    # TX->RX latency sample (match lm+b28+mm within 5s after TX)
    matched = 0
    latencies: list[float] = []
    for t in tx_s:
        t_ts, _, lm, b28, mm, _ = t
        best = None
        for r in rx_s:
            if r[2] == lm and r[3] == b28 and r[4] == mm:
                dt = (r[0] - t_ts).total_seconds()
                if 0 <= dt < 5 and (best is None or dt < best):
                    best = dt
        if best is not None:
            matched += 1
            latencies.append(best * 1000)

    print(f"\n=== TX/RX pairing (last session) ===")
    print(f"Matched: {matched}/{len(tx_s)} ({100*matched/max(1,len(tx_s)):.0f}%)")
    if latencies:
        latencies.sort()
        print(
            f"RX latency ms: min={min(latencies):.0f}  med={latencies[len(latencies)//2]:.0f}  "
            f"max={max(latencies):.0f}"
        )

    gaps = [(tx_s[i][0] - tx_s[i - 1][0]).total_seconds() * 1000 for i in range(1, len(tx_s))]
    if gaps:
        gs = sorted(gaps)
        print(
            f"Inter-TX ms: min={min(gaps):.0f}  med={gs[len(gs)//2]:.0f}  "
            f"max={max(gaps):.0f}  mean={sum(gaps)/len(gaps):.0f}"
        )

    # Check SYM7 types
    for t in ["8101SY", "0040MU", "0040SA", "0040WV", "8101MT"]:
        n = sum(1 for r in tx_s if r[2] == t)
        if n:
            print(f"  {t}: {n} TX")

    # Log end state
    if tx_s:
        last = tx_s[-1]
        print(f"\nLast TX: {last[0]}  {phase_key(last[2], last[3])} mm={last[4]:02X}")


if __name__ == "__main__":
    main()

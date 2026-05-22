#!/usr/bin/env python3
"""Export params_meta.json observationLog fields to readable plain text."""

import json
import os
import re
from datetime import datetime

path = os.path.join(os.environ["APPDATA"], "Application Support", "Sysex77", "params_meta.json")
out_path = os.path.join(os.environ["APPDATA"], "Application Support", "Sysex77", "observation_logs.txt")

with open(path, encoding="utf-8") as f:
    data = json.load(f)

pat = re.compile(r"\[#(\d+)\]\s*SysEx:\s*(.+)", re.I)

lines_out: list[str] = []
lines_out.append("SY99 Observation Logs (exported from params_meta.json)")
lines_out.append(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
lines_out.append("=" * 72)
lines_out.append("")

count_params = 0
count_sysex = 0


def fmt_addr(addr: dict) -> str:
    parts = [addr.get("b3"), addr.get("b4"), addr.get("b5"), addr.get("b6")]
    if all(isinstance(x, int) for x in parts):
        return " ".join(f"{x:02X}" for x in parts)
    return " ".join(str(x) for x in parts)


for entry in data:
    pid = entry.get("id") or entry.get("tag") or "?"
    label = entry.get("uiLabel") or ""
    group = entry.get("group") or ""
    ver = entry.get("sy99Verification") or {}
    if not isinstance(ver, dict):
        continue

    def el_sort_key(item):
        k = item[0]
        return int(k) if str(k).lstrip("-").isdigit() else str(k)

    for el_key, block in sorted(ver.items(), key=el_sort_key):
        if not isinstance(block, dict):
            continue
        log = (block.get("observationLog") or "").strip()
        if not log:
            continue

        count_params += 1
        addr = entry.get("addr") or {}

        lines_out.append(f"PARAMETER: {pid}  ({label})")
        lines_out.append(f"GROUP:     {group}")
        lines_out.append(f"ELEMENT:   {el_key}")
        lines_out.append(f"ADDR:      {fmt_addr(addr)}")
        lines_out.append(f"RAW RANGE: {entry.get('rawMin', '?')} .. {entry.get('rawMax', '?')}")
        if block.get("confirmed"):
            lines_out.append(f"CONFIRMED: yes  at {block.get('confirmedAt', '')}")
        lines_out.append("-" * 72)
        lines_out.append("SysEx list:")
        lines_out.append("")

        for ln in log.split("\n"):
            ln = ln.strip()
            if not ln:
                continue
            m = pat.match(ln)
            if m:
                idx = int(m.group(1))
                sysex = m.group(2).strip()
                parts = sysex.split()
                raw = None
                if len(parts) >= 2:
                    try:
                        raw = int(parts[-2], 16)
                    except ValueError:
                        raw = None
                raw_s = f"raw={raw}" if raw is not None else ""
                lines_out.append(f"  [{idx:4d}]  {sysex}  ({raw_s})" if raw_s else f"  [{idx:4d}]  {sysex}")
                count_sysex += 1
            else:
                lines_out.append(f"  {ln}")

        lines_out.append("")
        lines_out.append("=" * 72)
        lines_out.append("")

with open(out_path, "w", encoding="utf-8", newline="\n") as f:
    f.write("\n".join(lines_out))

print(f"out: {out_path}")
print(f"params with logs: {count_params}")
print(f"sysex lines: {count_sysex}")
print(f"size: {os.path.getsize(out_path)}")

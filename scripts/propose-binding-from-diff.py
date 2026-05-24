#!/usr/bin/env python3
"""
Propose bulk dump offset from before/after .syx pair and param id.

Usage:
  python scripts/propose-binding-from-diff.py before.syx after.syx ELDT
  python scripts/propose-binding-from-diff.py --param-id OUTSEL fixtures/04_anonim_outsel_both.syx fixtures/05_anonim_outsel_g1.syx

Reads sy99_master_catalog.json for expected SysEx group (GG) and NN.
Diffs 8101VC and 0040VC frames; prints candidate byte offsets and bindStatus suggestion.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CATALOG_PATHS = [
    ROOT / "_agent_context" / "fixtures" / "sy99_master_catalog.json",
    ROOT / "ui" / "fixtures" / "sy99_master_catalog.json",
]


def load_catalog() -> list[dict]:
    for path in CATALOG_PATHS:
        if path.is_file():
            return json.loads(path.read_text(encoding="utf-8"))
    raise FileNotFoundError("sy99_master_catalog.json not found")


def find_param(catalog: list[dict], param_id: str) -> dict | None:
    needle = param_id.strip().upper()
    for row in catalog:
        if row.get("id", "").upper() == needle or row.get("tag", "").upper() == needle:
            return row
        reg = row.get("registryId")
        if reg and str(reg).upper() == needle:
            return row
    return None


def extract_frame(data: bytes, tag: bytes) -> bytes | None:
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
        if tag in frame:
            return frame
        i = j + 1
    return None


def diff_frames(a: bytes, b: bytes) -> list[tuple[int, int, int]]:
    n = min(len(a), len(b))
    return [(i, a[i], b[i]) for i in range(n) if a[i] != b[i]]


def format_gg_nn(entry: dict) -> str:
    gg = entry.get("sysexGroup", 0)
    nn = entry.get("nn", 0)
    return f"GG=0x{gg:02X} NN=0x{nn:02X}"


def propose_for_frame(name: str, before: bytes | None, after: bytes | None, entry: dict) -> None:
    print(f"\n=== {name} ===")
    if before is None or after is None:
        print("  frame not found in one or both files")
        return

    diffs = diff_frames(before, after)
    print(f"  frame size: {len(before)} bytes, diffs: {len(diffs)}")
    if not diffs:
        print("  no byte changes — check that only one parameter was edited")
        return

    per_element = bool(entry.get("perElement"))
    element_count = int(entry.get("elementCount") or (4 if per_element else 1))

    print("  changed offsets:")
    for off, va, vb in diffs[:40]:
        hint = ""
        if per_element and element_count == 4:
            for el in range(4):
                base = el * 0x20
                if (off - base) >= 0 and (off - base) < 0x20:
                    hint = f" (element {el + 1}, +0x{off - base:02X} in EE block)"
                    break
        print(f"    @{off:4d} (0x{off:04X}): 0x{va:02X} -> 0x{vb:02X}{hint}")

    if len(diffs) > 40:
        print(f"    … and {len(diffs) - 40} more")

    if len(diffs) == 1:
        off, va, vb = diffs[0]
        print(f"\n  candidate_binding:")
        print(f"    bulkSource: {'8101' if name == '8101VC' else '0040'}")
        print(f"    frameOffset: {off}")
        print(f"    rawBefore: {va}")
        print(f"    rawAfter: {vb}")
        print('    bindStatus: "candidate_bulk8101"' if name == "8101VC" else '    bindStatus: "candidate_bulk0040"')
    elif len(diffs) <= 4 and per_element:
        print("\n  likely per-element strip — verify each offset maps to element index × 0x20")


def main() -> int:
    parser = argparse.ArgumentParser(description="Propose dump binding from syx diff")
    parser.add_argument("before", type=Path, help="voice dump before edit")
    parser.add_argument("after", type=Path, help="voice dump after single-param edit")
    parser.add_argument("param_id", nargs="?", help="catalog param id, e.g. ELDT")
    parser.add_argument("--param-id", dest="param_id_flag", help="alias for param_id")
    args = parser.parse_args()

    param_id = args.param_id_flag or args.param_id
    if not param_id:
        parser.error("param_id required")

    catalog = load_catalog()
    entry = find_param(catalog, param_id)
    if entry is None:
        print(f"Unknown param_id: {param_id}", file=sys.stderr)
        return 1

    before_data = args.before.read_bytes()
    after_data = args.after.read_bytes()

    print(f"Param: {entry.get('id')} ({entry.get('uiLabel')})")
    print(f"  {format_gg_nn(entry)}  perElement={entry.get('perElement')}  address={entry.get('addressHex')}")
    print(f"  current bindStatus: {entry.get('bindStatus', 'manual_only')}")

    propose_for_frame("8101VC", extract_frame(before_data, b"8101VC"), extract_frame(after_data, b"8101VC"), entry)
    propose_for_frame("0040VC", extract_frame(before_data, b"0040VC"), extract_frame(after_data, b"0040VC"), entry)

    print("\nNext steps:")
    print("  1. Update _agent_context/fixtures/sy99_param_bindings.json")
    print("  2. python _agent_context/fixtures/_validate_bulk_parse.py")
    print("  3. Library review (?review=) — see _agent_context/library_binding_workflow.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

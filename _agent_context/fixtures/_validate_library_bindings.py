#!/usr/bin/env python3
"""Validate sy99_param_bindings.json bulkRead specs vs parser field map."""
from __future__ import annotations

import json
import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent
BINDINGS = FIX / "sy99_param_bindings.json"

FIELDS_8101 = {
    "elmodeRaw",
    "wolRaw",
    "elvlRaw",
    "lmEldtRaw",
    "lmElnsRaw",
    "lmEnllRaw",
    "lmEnlhRaw",
    "lmEvllRaw",
    "lmEvlhRaw",
    "lmOutselRaw",
    "lmEfln1ElRaw",
    "lmEfsdlvRaw",
}

FIELDS_0040 = {
    "wpbrRaw",
    "atpbrRaw",
    "pmasnRaw",
    "pmrngRaw",
    "amasnRaw",
    "amrngRaw",
    "fmasnRaw",
    "fmrngRaw",
    "pnlasnRaw",
    "pnlrngRaw",
    "coasnRaw",
    "corngRaw",
    "pnbasnRaw",
    "pnbrngRaw",
    "egbasnRaw",
    "egbrngRaw",
    "wlasnRaw",
    "wllmlRaw",
    "mctunRaw",
    "rndpRaw",
    "aftmdRaw",
    "sptpntRaw",
    "efmodeRaw",
}


def main() -> int:
    data = json.loads(BINDINGS.read_text(encoding="utf-8"))
    ok = True

    for item in data:
        status = item.get("bindStatus", "")
        bulk = item.get("bulkSource")
        read = item.get("bulkRead")
        pid = item.get("id", "?")

        if status.startswith("confirmed_bulk") or status.startswith("candidate_bulk"):
            if read is None:
                print(f"FAIL {pid}: missing bulkRead for {status}")
                ok = False
                continue

            frame = read.get("frame")
            field = read.get("field")
            if frame == "8101" and field not in FIELDS_8101:
                print(f"FAIL {pid}: unknown 8101 field {field!r}")
                ok = False
            elif frame == "0040" and field not in FIELDS_0040:
                print(f"FAIL {pid}: unknown 0040 field {field!r}")
                ok = False
            elif bulk and frame != bulk:
                print(f"FAIL {pid}: bulkSource={bulk} vs bulkRead.frame={frame}")
                ok = False

        if status == "manual_only" and read is not None:
            print(f"FAIL {pid}: manual_only must not have bulkRead")
            ok = False

    if ok:
        confirmed = sum(
            1
            for i in data
            if str(i.get("bindStatus", "")).startswith("confirmed_bulk")
            and i.get("bulkRead")
        )
        print(f"PASS {len(data)} bindings, {confirmed} with bulkRead")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

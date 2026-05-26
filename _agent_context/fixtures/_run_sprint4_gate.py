#!/usr/bin/env python3
"""Sprint 4 offline gate: library navigation regression + HW checklist pointer."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent

HW_CHECKLIST = """
Sprint 4 HW verify (SY99 + MIDI-OX + Sysex77_MIDI.log) - see TEST_STATUS.md Sprint 4

Preflight: python _test_library_navigation.py  (offline, this gate)

Scenario 3 - Inbound PC (LOW risk):
  SY99 panel: A1->A2->...->A16->B1; Librairie highlight follows; no PC ping-pong.
  Log: [NAV audit] RX ... suppressed=0; after outbound recall echo window suppressed=1.

Scenario 2 - Outbound recall (MED risk):
  MIDI-OX ch1: D1->D2 PC-only; A16->B1 PC 15->16 no CC0/CC32; Internal->PRE1 full triple;
  tab PRE1 -> CC0+CC32 then slot -> PC-only.
  Log: [NAV audit] TX fullTriple=0/1.

Scenario 4 - Synth panel nav abort 0040 (MED risk):
  Menu #14 Invoke A6 OR pending fetch0040; press SY99 slot before RX completes.
  Log: [BankClick] fetch0040 aborted: synth navigation mm=...
  R-KEEP-8: nav not blocked by requestSysex.

Scenario 1 - Librairie click ingest (HIGH param risk, nav OK):
  Click A1/A6/A7; [BankClick] parsed8101; companion parsed0040 if AUTOSYNC split on disk.
  Param vs LCD - separate Sprint 5 gate; nav/ingest path only here.
"""


def run_script(name: str) -> bool:
    path = FIX / name
    print(f"\n=== {name} ===")
    rc = subprocess.call([sys.executable, str(path)], cwd=str(FIX))
    if rc != 0:
        print(f"FAIL {name} (exit {rc})")
        return False
    return True


def main() -> int:
    failed = 0

    if not run_script("_test_library_navigation.py"):
        failed += 1

    # Sprint 2 regression still required before Sprint 5; quick nav-adjacent sanity.
    for optional in ("_validate_library_bindings.py",):
        if not run_script(optional):
            failed += 1

    print(HW_CHECKLIST)

    if failed:
        print(f"\nSprint 4 offline gate: FAIL ({failed} script(s))")
        return 1

    print("\nSprint 4 offline gate: PASS (HW scenarios 1-4 manual, see checklist above)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

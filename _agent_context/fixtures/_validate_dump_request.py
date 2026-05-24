#!/usr/bin/env python3
"""
Compare manual DUMP OUT baselines vs programmatic Dump Request captures.

Usage (from repo root):
  python _agent_context/fixtures/_validate_dump_request.py

Expects pairs in _agent_context/fixtures/:
  baseline_<case>_voice.syx  — manual DUMP OUT (FROM SY99 <-)
  REQTEST-<case>-*.syx         — auto request test capture

Cases: edit, a1, b1 (override BASELINE_FILES below if your manual names differ).
Exit 0 if all PASS, 1 otherwise.
"""
from __future__ import annotations

import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent

K_VNAM_OFF = 33
K_VNAM_LEN = 10
K_ELMODE_OFF = 32
K_MIN_8101 = 100

# Manual baselines from hardware (rename/copy your VOICE1-*.syx here if needed)
BASELINE_FILES: dict[str, str] = {
    "edit": "baseline_edit_voice.syx",
    "a1": "baseline_a1_voice.syx",
    "b1": "baseline_b1_voice.syx",
}

# Fallbacks from 2026-05-23 manual session (EP|GrnDual / ANONIM / SP:Alaska)
BASELINE_FALLBACKS: dict[str, list[str]] = {
    "edit": ["baseline_ep_grndual.syx", "VOICE1-20260523-103322.syx"],
    "a1": ["baseline_anonim.syx", "VOICE1-20260523-103851.syx"],
    "b1": ["baseline_sp_alaska.syx", "VOICE1-20260523-104543.syx"],
}


def load(path: Path) -> bytes:
    return path.read_bytes()


def iter_sysex_frames(data: bytes):
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
        yield data[i : j + 1]
        i = j + 1


def find_tag_frame(data: bytes, tag: bytes) -> bytes | None:
    for frame in iter_sysex_frames(data):
        if tag in frame:
            return frame
    return None


def vnam(frame: bytes) -> str:
    if frame is None or len(frame) < K_VNAM_OFF + K_VNAM_LEN:
        return ""
    raw = frame[K_VNAM_OFF : K_VNAM_OFF + K_VNAM_LEN]
    return raw.decode("ascii", errors="replace").rstrip()


def elmode(frame: bytes) -> int | None:
    if frame is None or len(frame) <= K_ELMODE_OFF:
        return None
    return frame[K_ELMODE_OFF]


def resolve_baseline(case: str) -> Path | None:
    primary = FIX / BASELINE_FILES.get(case, "")
    if primary.is_file():
        return primary
    for name in BASELINE_FALLBACKS.get(case, []):
        p = FIX / name
        if p.is_file():
            return p
    cap_dir = Path.home() / "AppData/Roaming/Application Support/Sysex77/captures"
    for name in BASELINE_FALLBACKS.get(case, []):
        if not name.startswith("VOICE"):
            continue
        p = cap_dir / name
        if p.is_file():
            return p
    return None


def newest_reqtest(case: str) -> Path | None:
    matches = sorted(FIX.glob(f"REQTEST-{case}-*.syx"), key=lambda p: p.stat().st_mtime, reverse=True)
    if matches:
        return matches[0]
    cap_dir = Path.home() / "AppData/Roaming/Application Support/Sysex77/captures"
    matches = sorted(cap_dir.glob(f"REQTEST-{case}-*.syx"), key=lambda p: p.stat().st_mtime, reverse=True)
    return matches[0] if matches else None


def first_diff(a: bytes, b: bytes, limit: int = 8) -> str:
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            return f"offset {i}: baseline=0x{a[i]:02X} request=0x{b[i]:02X}"
    if len(a) != len(b):
        return f"length baseline={len(a)} request={len(b)}"
    return "identical"


def compare_pair(case: str) -> bool:
    baseline_path = resolve_baseline(case)
    request_path = newest_reqtest(case)

    print(f"\n=== case {case} ===")

    if baseline_path is None:
        print(f"  SKIP - no baseline (expected {BASELINE_FILES.get(case)})")
        return False

    if request_path is None:
        print(f"  FAIL - no REQTEST-{case}-*.syx (run Librairie REQUEST VOICE -> Test request)")
        return False

    print(f"  baseline: {baseline_path.name}")
    print(f"  request:  {request_path.name}")

    b_data = load(baseline_path)
    r_data = load(request_path)

    for tag, label in ((b"8101VC", "8101VC"), (b"0040VC", "0040VC")):
        bf = find_tag_frame(b_data, tag)
        rf = find_tag_frame(r_data, tag)

        if bf is None:
            print(f"  FAIL - baseline missing {label.decode()}")
            return False

        if rf is None:
            print(f"  FAIL - request missing {label.decode()}")
            return False

        if tag == b"8101VC":
            bn, rn = vnam(bf), vnam(rf)
            be, re = elmode(bf), elmode(rf)
            print(f"  VNAM baseline={bn!r} request={rn!r}")
            print(f"  ELMODE baseline={be} request={re}")

            if bn != rn:
                print(f"  FAIL - VNAM mismatch")
                return False

            if be != re:
                print(f"  FAIL - ELMODE mismatch")
                return False

        if len(bf) != len(rf):
            print(f"  FAIL - {label.decode()} length {len(bf)} vs {len(rf)} ({first_diff(bf, rf)})")
            return False

        if bf != rf:
            print(f"  FAIL - {label.decode()} bytes differ ({first_diff(bf, rf)})")
            return False

        print(f"  OK — {label.decode()} exact match ({len(bf)} B)")

    print("  PASS")
    return True


def check_autosync_voice64() -> bool:
    cap_dir = Path.home() / "AppData/Roaming/Application Support/Sysex77/captures"
    matches = sorted(cap_dir.glob("AUTOSYNC-VOICE64-*.syx"),
                     key=lambda p: p.stat().st_mtime,
                     reverse=True)

    print("\n=== AUTOSYNC-VOICE64 ===")

    if not matches:
        print("  SKIP - no AUTOSYNC-VOICE64-*.syx (run Librairie REQUEST VOICE -> Auto-sync 64)")
        return True

    path = matches[0]
    data = load(path)
    frames = list(iter_sysex_frames(data))
    n8101 = sum(1 for f in frames if b"8101VC" in f)
    n0040 = sum(1 for f in frames if b"0040VC" in f)

    print(f"  file: {path.name}")
    print(f"  frames: {len(frames)}  8101VC: {n8101}  0040VC: {n0040}")

    if n8101 < 48:
        print(f"  FAIL - expected >= 48 voice frames (got {n8101})")
        return False

    print("  PASS - voice frame count OK")
    return True


def main() -> int:
    ok = True
    for case in ("a1", "b1", "edit"):
        if not compare_pair(case):
            ok = False
    if not check_autosync_voice64():
        ok = False
    print("\n" + ("ALL PASS" if ok else "SOME FAIL - see above"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

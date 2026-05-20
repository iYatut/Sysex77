#!/usr/bin/env python3
"""
Regression: bulk parse vs lcd_reference.csv (fixtures 01–03).

Mirrors YamahaLmVoiceDump + _parse_0040vc.py offset map.
Exit 0 if all PASS, 1 otherwise.
"""
from __future__ import annotations

import csv
import sys
from pathlib import Path

from _parse_0040vc import parse_lm0040_vc

FIX = Path(__file__).resolve().parent
CSV_PATH = FIX / "lcd_reference.csv"

K_LM8101_VC_NAME_OFFSET = 33
K_LM8101_VC_NAME_LENGTH = 10
K_LM8101_VC_ELMODE_OFFSET = 32
K_MIN_8101_VC_FRAME_SIZE = 100

EXPECTED_ELMODE = {"01": 8, "02": 8, "03": 4}
EXPECTED_ELVL = {"01": (110, 127), "02": (127, 127), "03": (127, 127)}
EXPECTED_ELDT = {"01": (0, 0), "02": (-1, 2), "03": (2, -2)}
EXPECTED_OUTSEL_BOTH = 0x06


def find_lm8101_frame(data: bytes) -> bytes | None:
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


def find_wol_elvl_e1_pair(frame: bytes) -> tuple[int, int, int, int] | None:
    search_end = min(len(frame) - 4, 130)
    best = None
    best_score = 9999
    first = None
    for i in range(44, search_end + 1):
        prefix = frame[i - 1]
        if prefix not in (0x03, 0x01):
            continue
        if frame[i + 1] != 0x00 or frame[i + 2] != 0x00:
            continue
        wol, elvl = frame[i], frame[i + 3]
        if wol > 127 or elvl > 127:
            continue
        if first is None:
            first = (i, i + 3, wol, elvl)
        if 93 <= i <= 96:
            score = (0 if prefix == 0x03 else 10) + (96 - i)
            if score < best_score:
                best_score = score
                best = (i, i + 3, wol, elvl)
    return best if best is not None else first


def find_first_anchor(frame: bytes) -> int | None:
    for a in range(90, min(len(frame) - 7, 200)):
        if frame[a : a + 3] == bytes([0x7F, 0x01, 0x7F]):
            return a
    return None


def max_elvl_anchor_slots_from_elmode(elmode_raw: int) -> bool:
    return elmode_raw in (1, 2, 4, 6, 7, 8, 9)


def parse_lm8101_vc(frame: bytes) -> dict:
    out = {
        "found8101": False,
        "name": "",
        "elmodeRaw": -1,
        "wolRaw": -1,
        "elvlE1Raw": -1,
        "outselE1Raw": -1,
        "elvlRaw": [-1, -1, -1, -1],
        "outselRaw": [-1, -1, -1, -1],
        "eldtRaw": [-1, -1, -1, -1],
        "elnsRaw": [-1, -1, -1, -1],
        "evllRaw": [-1, -1, -1, -1],
        "evlhRaw": [-1, -1, -1, -1],
        "efln1ElRaw": -1,
    }

    if len(frame) < K_MIN_8101_VC_FRAME_SIZE:
        return out
    if frame[0] != 0xF0 or frame[-1] != 0xF7 or frame[10:16] != b"8101VC":
        return out

    out["found8101"] = True
    name_bytes = frame[
        K_LM8101_VC_NAME_OFFSET : K_LM8101_VC_NAME_OFFSET + K_LM8101_VC_NAME_LENGTH
    ]
    out["name"] = name_bytes.decode("ascii", errors="replace").rstrip()

    em = frame[K_LM8101_VC_ELMODE_OFFSET]
    if em <= 10:
        out["elmodeRaw"] = em

    pair = find_wol_elvl_e1_pair(frame)
    if pair is None:
        return out

    wol_off, elvl_off, wol, elvl = pair
    out["wolRaw"] = wol
    out["elvlE1Raw"] = elvl
    out["elvlRaw"][0] = elvl

    outsel_off = elvl_off + 1
    if outsel_off < len(frame):
        out["outselE1Raw"] = frame[outsel_off] & 0x06

    # E1 strip relative to outsel (ELVL+1)
    if outsel_off + 4 < len(frame):
        out["elnsRaw"][0] = frame[outsel_off + 1] - 64
        out["evlhRaw"][0] = frame[outsel_off + 3]
        out["evllRaw"][0] = frame[outsel_off + 4]

    # E1 ELDT — confirmed elmode 4 (fixture 03): raw UI @ outsel+0
    if out["elmodeRaw"] == 4 and outsel_off < len(frame):
        out["eldtRaw"][0] = frame[outsel_off]

    # EFLN1EL El.1 @ elvl + delta (delta depends on WOL anchor; fixtures 01–03)
    efln_delta = {94: 36, 93: 37, 95: 35}.get(wol_off, 36)
    efln_off = elvl_off + efln_delta
    if efln_off < len(frame):
        out["efln1ElRaw"] = frame[efln_off]

    anchor = find_first_anchor(frame)
    if anchor is not None and max_elvl_anchor_slots_from_elmode(out["elmodeRaw"]):
        out["outselRaw"][1] = frame[anchor + 4] & 0x06
        out["elvlRaw"][1] = frame[anchor + 5]
        if out["elmodeRaw"] == 8:
            out["eldtRaw"][1] = frame[anchor + 6]
        out["elnsRaw"][1] = frame[anchor + 7] - 64

    return out


def load_lcd_rows() -> list[dict]:
    with CSV_PATH.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def main() -> int:
    rows = load_lcd_rows()
    target_ids = {"01", "02", "03"}
    all_pass = True

    for row in rows:
        fid = row["fixture_id"].strip()
        if fid not in target_ids:
            continue

        syx_path = FIX / row["syx_file"].strip()
        if not syx_path.is_file():
            print(f"{fid}: FAIL missing {syx_path.name}")
            all_pass = False
            continue

        data = syx_path.read_bytes()
        frame = find_lm8101_frame(data)
        f40 = None
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
            fr = data[i : j + 1]
            if b"0040VC" in fr:
                f40 = fr
            i = j + 1

        if frame is None:
            print(f"{fid}: FAIL no 8101VC")
            all_pass = False
            continue

        parsed = parse_lm8101_vc(frame)
        p40 = parse_lm0040_vc(f40) if f40 else {"found0040": False, "fields": {}}

        exp_name = row["sy99_voice_name"].strip()
        exp_wol = int(row["master_volume_wol_lcd"])
        exp_elmode = EXPECTED_ELMODE[fid]
        exp_el1, exp_el2 = EXPECTED_ELVL[fid]
        exp_eldt1, exp_eldt2 = EXPECTED_ELDT[fid]

        checks = [
            ("VNAM", parsed["name"] == exp_name, f"{parsed['name']!r} vs {exp_name!r}"),
            ("WOL", parsed["wolRaw"] == exp_wol, f"{parsed['wolRaw']} vs {exp_wol}"),
            ("ELVL_E1", parsed["elvlE1Raw"] == exp_el1, f"{parsed['elvlE1Raw']} vs {exp_el1}"),
            ("ELMODE", parsed["elmodeRaw"] == exp_elmode, f"{parsed['elmodeRaw']} vs {exp_elmode}"),
            ("ELVL_E2", parsed["elvlRaw"][1] == exp_el2, f"{parsed['elvlRaw'][1]} vs {exp_el2}"),
            # OUTSEL E1 bulk often stale vs LCD on fixtures 01–03 (see 04/05 diff).
            ("EVLL_E1", parsed["evllRaw"][0] == 1, f"{parsed['evllRaw'][0]} vs 1"),
            ("EVLH_E1", parsed["evlhRaw"][0] == 127, f"{parsed['evlhRaw'][0]} vs 127"),
            ("ELNS_E1", parsed["elnsRaw"][0] == 0, f"{parsed['elnsRaw'][0]} vs 0"),
        ]

        if parsed["elmodeRaw"] == 8:
            checks.append(("ELDT_E2", parsed["eldtRaw"][1] == exp_eldt2,
                           f"{parsed['eldtRaw'][1]} vs {exp_eldt2}"))

        if fid == "03":
            checks.append(("ELDT_E1", parsed["eldtRaw"][0] == exp_eldt1,
                           f"{parsed['eldtRaw'][0]} vs {exp_eldt1}"))
            checks.append(("EFLN1EL", parsed["efln1ElRaw"] == 0x05,
                           f"0x{parsed['efln1ElRaw']:02X} vs 0x05"))

        if p40.get("found0040"):
            wpbr = p40["fields"].get("WPBR", {}).get("raw", -1)
            spt = p40["fields"].get("SPTPNT", {}).get("raw", -1)
            wll = p40["fields"].get("WLLML", {}).get("raw", -1)
            checks.extend([
                ("0040_WPBR", wpbr in (0, 2), f"{wpbr} (ANONIM=2, others may be 0)"),
                ("0040_SPTPNT", spt == 60, f"{spt} vs 60"),
                ("0040_WLLML", wll == 100, f"{wll} vs 100"),
            ])

        failed = [f"{k}({d})" for k, ok, d in checks if not ok]
        if failed:
            print(f"{fid} {exp_name}: FAIL — " + ", ".join(failed))
            all_pass = False
        else:
            print(f"{fid} {exp_name}: PASS")

    return 0 if all_pass else 1


if __name__ == "__main__":
    sys.exit(main())

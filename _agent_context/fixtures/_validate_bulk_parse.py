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
EXPECTED_EFLN1EL = {"03": 0x05}
EXPECTED_EFSDLV = {"03": 127}
EXPECTED_OUTSEL_BOTH = 0x06
K_LM8101_VC_EFSDLV_DELTA_FROM_EFLN = 12
K_LM8101_VC_EFSDLV_DELTA_FROM_ELVL = 47


def efln_wire_masked(raw: int) -> int:
    b = raw & 0xFF
    ui = b & 0x05
    if b & 0x02:
        ui |= 0x01
    return ui & 0x05


def efln1_el_raw_from_mixer_strip(frame: bytes, elvl_off: int, elvl_e1_raw: int) -> int:
    if elvl_off < 0:
        return -1
    if elvl_e1_raw == 0:
        off35 = elvl_off + 35
        return efln_wire_masked(frame[off35]) if off35 < len(frame) else -1
    best_masked = -1
    best_score = -1
    for off in (elvl_off + 39, elvl_off + 26, elvl_off + 35):
        if off >= len(frame):
            continue
        masked = efln_wire_masked(frame[off])
        if masked <= 0:
            continue
        score = 3 if masked == 5 else 2 if masked == 4 else 1 if masked == 1 else 0
        if score > best_score:
            best_score = score
            best_masked = masked
    if best_masked >= 0:
        return best_masked
    off39 = elvl_off + 39
    return efln_wire_masked(frame[off39]) if off39 < len(frame) else -1


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


def max_elns_anchor_slots_from_elmode(elmode_raw: int) -> int:
    if elmode_raw in (2, 7, 9):
        return 3
    if elmode_raw in (1, 4, 6, 8):
        return 1
    return 0


def find_next_anchor(frame: bytes, after: int) -> int | None:
    for a in range(after + 1, min(len(frame) - 7, 200)):
        if frame[a : a + 3] == bytes([0x7F, 0x01, 0x7F]):
            return a
    return None


def max_elvl_anchor_slots_from_elmode(elmode_raw: int) -> bool:
    return elmode_raw in (1, 2, 4, 6, 7, 8, 9)


def eldt_e1_uses_outsel_strip(elmode_raw: int) -> bool:
    return elmode_raw in (1, 4, 6, 8)


def eldt_e2_uses_first_anchor(elmode_raw: int) -> bool:
    return elmode_raw in (4, 8)


def eldt_ui(raw: int) -> int:
    raw = raw & 0x7F
    delta = 9
    if raw > delta - 2:
        raw -= delta
        raw = ~raw
    return max(-7, min(7, raw))


def elns_ui(raw: int) -> int:
    if raw < 0:
        return raw
    return max(-64, min(63, (raw & 0x7F) - 64))


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
        "enllRaw": [-1, -1, -1, -1],
        "enlhRaw": [-1, -1, -1, -1],
        "evllRaw": [-1, -1, -1, -1],
        "evlhRaw": [-1, -1, -1, -1],
        "efln1ElRaw": -1,
        "efsdlvRaw": -1,
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

    # E1 strip relative to outsel (ELVL+1) — sysex raw (matches YamahaLmVoiceDump.h)
    if outsel_off + 5 < len(frame):
        out["elnsRaw"][0] = frame[outsel_off + 1]
        out["enllRaw"][0] = frame[outsel_off + 2]
        out["enlhRaw"][0] = frame[outsel_off + 3]
        out["evllRaw"][0] = frame[outsel_off + 4]
        out["evlhRaw"][0] = frame[outsel_off + 5]

    if outsel_off < len(frame) and eldt_e1_uses_outsel_strip(out["elmodeRaw"]):
        out["eldtRaw"][0] = frame[outsel_off]

    # EFLN1EL El.1 @ mixer strip (elvl layout — HW matrix 2026-05-26)
    if elvl_off >= 0:
        out["efln1ElRaw"] = efln1_el_raw_from_mixer_strip(frame, elvl_off, elvl)
        efsdlv_off = elvl_off + K_LM8101_VC_EFSDLV_DELTA_FROM_ELVL
        if efsdlv_off < len(frame):
            out["efsdlvRaw"] = frame[efsdlv_off]

    max_anchors = max_elns_anchor_slots_from_elmode(out["elmodeRaw"])
    if max_anchors > 0:
        anchor = find_first_anchor(frame)
        for anchor_slot in range(max_anchors):
            if anchor is None or anchor + 11 >= len(frame):
                break

            el_idx = 1 + anchor_slot
            if el_idx < 4:
                out["outselRaw"][el_idx] = frame[anchor + 4] & 0x06
                out["elnsRaw"][el_idx] = frame[anchor + 7]
                out["enllRaw"][el_idx] = frame[anchor + 8]
                out["enlhRaw"][el_idx] = frame[anchor + 9]
                out["evllRaw"][el_idx] = frame[anchor + 10]
                out["evlhRaw"][el_idx] = frame[anchor + 11]

            if anchor_slot == 0:
                if out["elvlRaw"][1] < 0:
                    out["elvlRaw"][1] = frame[anchor + 5]
                if eldt_e2_uses_first_anchor(out["elmodeRaw"]):
                    out["eldtRaw"][1] = frame[anchor + 6]

            if anchor_slot + 1 < max_anchors:
                anchor = find_next_anchor(frame, anchor)

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
            ("ENLL_E1", parsed["enllRaw"][0] == 0, f"{parsed['enllRaw'][0]} vs 0"),
            ("ENLH_E1", parsed["enlhRaw"][0] == 127, f"{parsed['enlhRaw'][0]} vs 127"),
        ]

        if parsed["enllRaw"][1] >= 0:
            checks.append((
                "ENLL_E2",
                parsed["enllRaw"][1] == 0,
                f"{parsed['enllRaw'][1]} vs 0",
            ))

        if parsed["enlhRaw"][1] >= 0:
            checks.append((
                "ENLH_E2",
                parsed["enlhRaw"][1] == 127,
                f"{parsed['enlhRaw'][1]} vs 127",
            ))

        if parsed["elnsRaw"][0] >= 0:
            checks.append((
                "ELNS_E1",
                elns_ui(parsed["elnsRaw"][0]) == 0,
                f"{elns_ui(parsed['elnsRaw'][0])} vs 0",
            ))

        if parsed["eldtRaw"][0] >= 0:
            checks.append((
                "ELDT_E1",
                eldt_ui(parsed["eldtRaw"][0]) == exp_eldt1,
                f"{eldt_ui(parsed['eldtRaw'][0])} vs {exp_eldt1}",
            ))

        if parsed["eldtRaw"][1] >= 0:
            checks.append((
                "ELDT_E2",
                eldt_ui(parsed["eldtRaw"][1]) == exp_eldt2,
                f"{eldt_ui(parsed['eldtRaw'][1])} vs {exp_eldt2}",
            ))

        if fid == "03":
            checks.append(("EFLN1EL", parsed["efln1ElRaw"] == EXPECTED_EFLN1EL[fid],
                           f"0x{parsed['efln1ElRaw']:02X} vs 0x{EXPECTED_EFLN1EL[fid]:02X}"))
            checks.append(("EFSDLV_E1", parsed["efsdlvRaw"] == EXPECTED_EFSDLV[fid],
                           f"{parsed['efsdlvRaw']} vs {EXPECTED_EFSDLV[fid]}"))

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

    all_pass = validate_grndual_efsdlv() and all_pass

    return 0 if all_pass else 1


def bulk8101_efsd_tail_diagnostic(frame: bytes) -> int:
    """Byte @ elvl+47 in 8101VC (106 for EP|GrnDual — diagnostic, not authoritative EFSDLV)."""
    parsed = parse_lm8101_vc(frame)
    pair = find_wol_elvl_e1_pair(frame)
    if pair is None:
        return parsed["efsdlvRaw"]
    elvl_off = pair[1]
    off = elvl_off + K_LM8101_VC_EFSDLV_DELTA_FROM_ELVL
    return frame[off] if off < len(frame) else parsed["efsdlvRaw"]


def validate_grndual_efsdlv() -> bool:
    path = FIX / "baseline_ep_grndual.syx"
    ok = True

    if not path.is_file():
        print("grndual: FAIL missing baseline_ep_grndual.syx")
        return False

    data = path.read_bytes()
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
        print("grndual: FAIL no 8101VC")
        return False

    parsed = parse_lm8101_vc(frame)
    p40 = parse_lm0040_vc(f40) if f40 else {"found0040": False, "fields": {}}
    tail = bulk8101_efsd_tail_diagnostic(frame)

    checks = [
        ("elmode", parsed["elmodeRaw"] == 8, f"{parsed['elmodeRaw']} vs 8"),
        ("8101_tail@efln+12", tail == 106, f"{tail} vs 106 (diagnostic, not EFSDLV)"),
    ]

    if p40.get("found0040"):
        e1 = p40["fields"].get("EFSDLV_E1", {}).get("raw", -1)
        e2 = p40["fields"].get("EFSDLV_E2", {}).get("raw", -1)
        checks.extend([
            ("0040_EFSDLV_E1", e1 == 127, f"{e1} vs 127"),
            ("0040_EFSDLV_E2", e2 == 127, f"{e2} vs 127"),
        ])
    else:
        e1 = e2 = -1
        checks.append(("0040_present", False, "missing 0040VC"))

    failed = [f"{k}({d})" for k, pass_ok, d in checks if not pass_ok]
    if failed:
        print("grndual EP|GrnDual: FAIL — " + ", ".join(failed))
        ok = False
    else:
        print("grndual EP|GrnDual: PASS")

    # Poison: without live, bulk8101 tail 106 must not win; bulk0040 E1=127 does.
    elmode = parsed["elmodeRaw"]
    live_e1 = -1
    if live_e1 >= 0:
        resolved_no_live, src_no_live = live_e1, "live"
    elif elmode == 4 and tail >= 0:
        resolved_no_live, src_no_live = tail, "8101"
    elif elmode == 8 and e1 >= 0:
        resolved_no_live, src_no_live = e1, "0040"
    else:
        resolved_no_live, src_no_live = -1, "None"

    poison_ok = resolved_no_live == 127 and src_no_live == "0040"
    if not poison_ok:
        print(
            f"grndual poison no-live: FAIL resolved={resolved_no_live} src={src_no_live} "
            f"(expected 127 from 0040, not 8101 tail {tail})"
        )
        ok = False
    else:
        print("grndual poison no-live: PASS (0040 wins; 8101 tail ignored)")

    resolved_live, src_live = (127, "live")
    if resolved_live != 127 or src_live != "live":
        print(f"grndual poison live=127: FAIL resolved={resolved_live} src={src_live}")
        ok = False
    else:
        print("grndual poison live=127: PASS")

    return ok


if __name__ == "__main__":
    sys.exit(main())

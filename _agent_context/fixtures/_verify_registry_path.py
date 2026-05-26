#!/usr/bin/env python3
"""Mirror Sy99ParamRegistry resolve/apply path against fixtures 01-03."""
from __future__ import annotations

import csv
import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent
sys.path.insert(0, str(FIX))
from _parse_0040vc import find_frame, parse_lm0040_vc
from _validate_bulk_parse import find_lm8101_frame, parse_lm8101_vc

META = {
    "ELMODE": dict(live="elmodeRaw", b8101="lmElmodeRaw", b0040=None, cf8101=True, cf0040=False, cand=False, packed=False),
    "WOL": dict(live="commonVolumeRaw", b8101="lmWolRaw", b0040=None, cf8101=True, cf0040=False, cand=False, packed=False),
    "WPBR": dict(live="liveWpbrRaw", b8101=None, b0040="lm0040WpbrRaw", cf8101=False, cf0040=True, cand=False, packed=False),
    "WLLML": dict(live="liveWllmlRaw", b8101=None, b0040="lm0040WllmlRaw", cf8101=False, cf0040=True, cand=False, packed=False),
    "SPTPNT": dict(live="liveSptpntRaw", b8101=None, b0040="lm0040SptpntRaw", cf8101=False, cf0040=True, cand=False, packed=False),
    "EFMODE": dict(live="efmodeRaw", b8101=None, b0040="lm0040EfmodeRaw", cf8101=False, cf0040=True, cand=False, packed=False),
    "ELVL": dict(live="elementLevelRaw", b8101="lmElvlE1Raw", b0040=None, cf8101=True, cf0040=False, cand=False, packed=False, per_el=True),
    "ATPBR": dict(live="liveAtpbrRaw", b8101=None, b0040="lm0040AtpbrRaw", cf8101=False, cf0040=True, cand=False, packed=False),
    "PMASN": dict(live="livePmasnRaw", b8101=None, b0040="lm0040PmasnRaw", cf8101=False, cf0040=True, cand=False, packed=False),
    "PMRNG": dict(live="livePmrngRaw", b8101=None, b0040="lm0040PmrngRaw", cf8101=False, cf0040=True, cand=False, packed=False),
    "AMASN": dict(live="liveAmasnRaw", b8101=None, b0040="lm0040AmasnRaw", cf8101=False, cf0040=True, cand=False, packed=True),
    "AMRNG": dict(live="liveAmrngRaw", b8101=None, b0040="lm0040AmrngRaw", cf8101=False, cf0040=True, cand=False, packed=True),
    "MCTUN": dict(live="liveMctunRaw", b8101=None, b0040="lm0040MctunRaw", cf8101=False, cf0040=True, cand=False, packed=False),
    "RNDP": dict(live="liveRndpRaw", b8101=None, b0040="lm0040RndpRaw", cf8101=False, cf0040=False, cand=True, packed=True),
    "AFTMD": dict(live="liveAftmdRaw", b8101=None, b0040=None, cf8101=False, cf0040=False, cand=True, packed=False),
}


class State:
    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        self.hasParsedBulk8101 = False
        self.hasParsedBulk0040 = False
        self.elmodeRaw = -1
        self.efmodeRaw = -1
        self.commonVolumeRaw = -1
        self.lmElmodeRaw = -1
        self.lmWolRaw = -1
        self.lmElvlE1Raw = -1
        self.elementLevelRaw = [-1, -1, -1, -1]
        self.liveWpbrRaw = -1
        self.liveWllmlRaw = -1
        self.liveSptpntRaw = -1
        self.liveAtpbrRaw = -1
        self.livePmasnRaw = -1
        self.liveAmasnRaw = -1
        self.liveAmrngRaw = -1
        self.liveMctunRaw = -1
        self.liveRndpRaw = -1
        self.liveAftmdRaw = -1
        self.lmEfsdlvRaw = -1
        self.lm0040EfsdlvE1Raw = -1
        self.lm0040EfsdlvE2Raw = -1
        self.elementEfsendlvlRaw = [-1, -1, -1, -1]
        self.lm0040WpbrRaw = -1
        self.lm0040WllmlRaw = -1
        self.lm0040SptpntRaw = -1
        self.lm0040AtpbrRaw = -1
        self.lm0040PmasnRaw = -1
        self.lm0040AmasnRaw = -1
        self.lm0040AmrngRaw = -1
        self.lm0040MctunRaw = -1
        self.lm0040RndpRaw = -1
        self.lm0040EfmodeRaw = -1
        self.lmVoiceName = ""
        self.voiceName = ""
        self.voiceNameCharCount = 0
        self.parameterFrameCount = 0


def apply_bulk8101(s: State, p: dict) -> None:
    s.hasParsedBulk8101 = p["found8101"]
    if p["elmodeRaw"] >= 0:
        s.lmElmodeRaw = p["elmodeRaw"]
    if p["wolRaw"] >= 0:
        s.lmWolRaw = p["wolRaw"]
    if p["elvlE1Raw"] >= 0:
        s.lmElvlE1Raw = p["elvlE1Raw"]
    s.lmVoiceName = p["name"]
    if p["elmodeRaw"] == 4 and p.get("efsdlvRaw", -1) >= 0:
        s.lmEfsdlvRaw = p["efsdlvRaw"]


def apply_bulk0040(s: State, p40: dict) -> None:
    s.hasParsedBulk0040 = p40.get("found0040", False)
    if not s.hasParsedBulk0040:
        return
    mapping = {
        "WPBR": "lm0040WpbrRaw",
        "WLLML": "lm0040WllmlRaw",
        "SPTPNT": "lm0040SptpntRaw",
        "ATPBR": "lm0040AtpbrRaw",
        "PMASN": "lm0040PmasnRaw",
        "AMASN": "lm0040AmasnRaw",
        "AMRNG": "lm0040AmrngRaw",
        "MCTUN": "lm0040MctunRaw",
        "RNDP": "lm0040RndpRaw",
        "EFSDLV_E1": "lm0040EfsdlvE1Raw",
        "EFSDLV_E2": "lm0040EfsdlvE2Raw",
        "EFMODE": "lm0040EfmodeRaw",
    }
    for k, attr in mapping.items():
        if k in p40["fields"]:
            setattr(s, attr, p40["fields"][k]["raw"])


def bulk0040_for_resolve(m: dict, raw: int) -> int:
    if raw < 0 or not m["cf0040"] or m["cand"] or m["packed"]:
        return -1
    return raw


def bulk8101_for_resolve(m: dict, raw: int) -> int:
    if raw < 0 or not m["cf8101"]:
        return -1
    return raw


def catalog_efsdlv_raw0040(slot_raw: int, p40: dict, el: int) -> int:
    """Mirror populateRegistryDumpFields: authoritative parsed 0040 overrides bulk slot (incl. poison 0)."""
    field = "EFSDLV_E1" if el == 0 else "EFSDLV_E2"
    raw = slot_raw
    fields = p40.get("fields") or {}
    if field in fields and fields[field]["raw"] >= 0:
        raw = fields[field]["raw"]
    return raw


def catalog_efsdlv_ui_value(raw8101: int, raw0040: int) -> int:
    """Mirror populateRegistryDumpFields uiValue: raw0040 before raw8101 for EFSDLV."""
    if raw0040 >= 0:
        return raw0040
    if raw8101 >= 0:
        return raw8101
    return -1


def resolve_efsdlv(s: State, el: int = 0) -> tuple[int, str]:
    live = s.elementEfsendlvlRaw[el] if 0 <= el < 4 else -1
    if live >= 0:
        return live, "live"
    elmode = s.lmElmodeRaw if s.lmElmodeRaw >= 0 else s.elmodeRaw
    if el == 0 and elmode == 4 and s.lmEfsdlvRaw >= 0:
        return s.lmEfsdlvRaw, "8101"
    if el == 0 and elmode == 8 and s.lm0040EfsdlvE1Raw >= 0:
        return s.lm0040EfsdlvE1Raw, "0040"
    if el == 1 and elmode == 8 and s.lm0040EfsdlvE2Raw >= 0:
        return s.lm0040EfsdlvE2Raw, "0040"
    return -1, "None"


def resolve(s: State, tag: str, el: int = 0) -> tuple[int, str]:
    m = META[tag]
    live = s.elementLevelRaw[el] if m.get("per_el") else getattr(s, m["live"])
    b8101 = getattr(s, m["b8101"]) if m["b8101"] else -1
    b0040 = getattr(s, m["b0040"]) if m["b0040"] else -1
    if live >= 0:
        return live, "live"
    b81 = bulk8101_for_resolve(m, b8101)
    if b81 >= 0:
        return b81, "8101"
    b40 = bulk0040_for_resolve(m, b0040)
    if b40 >= 0:
        return b40, "0040"
    return -1, "None"


def ingest_live(s: State, tag: str, value: int) -> None:
    m = META[tag]
    s.parameterFrameCount += 1
    if m.get("per_el"):
        s.elementLevelRaw[0] = value
    else:
        setattr(s, m["live"], value)


def vnam_resolved(s: State) -> str:
    if s.voiceNameCharCount > 0:
        return s.voiceName[: s.voiceNameCharCount].rstrip()
    return s.lmVoiceName.rstrip()


def main() -> int:
    rows = {r["fixture_id"].strip(): r for r in csv.DictReader((FIX / "lcd_reference.csv").open())}
    expected_elmode = {"01": 8, "02": 8, "03": 4}
    expected_elvl = {"01": (110, 127), "02": (127, 127), "03": (127, 127)}

    results: dict[str, str] = {}
    first_fail: tuple[str, str, int, int, str] | None = None

    def record(key: str, ok: bool, field: str, ctx: str, got: int | str, exp: int | str, src: str) -> None:
        nonlocal first_fail
        results[key] = "PASS" if ok else "FAIL"
        if not ok and first_fail is None:
            first_fail = (field, ctx, got, exp, src)

    for fid in ("01", "02", "03"):
        row = rows[fid]
        data = (FIX / row["syx_file"].strip()).read_bytes()
        p8101 = parse_lm8101_vc(find_lm8101_frame(data))
        f40 = find_frame(data, "0040VC")
        p40 = parse_lm0040_vc(f40) if f40 else {"found0040": False, "fields": {}}
        s = State()
        apply_bulk8101(s, p8101)
        apply_bulk0040(s, p40)

        record(f"bulk/{fid}/VNAM", vnam_resolved(s) == row["sy99_voice_name"].strip(),
               "VNAM", fid, vnam_resolved(s), row["sy99_voice_name"].strip(), "8101")
        for tag, exp in (
            ("ELMODE", expected_elmode[fid]),
            ("WOL", int(row["master_volume_wol_lcd"])),
            ("ELVL", expected_elvl[fid][0]),
        ):
            got, src = resolve(s, tag)
            record(f"bulk/{fid}/{tag}", got == exp, tag, fid, got, exp, src)

        if p40.get("found0040"):
            for tag in ("WPBR", "WLLML", "SPTPNT"):
                exp = p40["fields"][tag]["raw"]
                got, src = resolve(s, tag)
                record(f"bulk/{fid}/{tag}", got == exp, tag, fid, got, exp, src)

    data = (FIX / "01_init_anonim_07x1_voice.syx").read_bytes()
    p8101 = parse_lm8101_vc(find_lm8101_frame(data))
    p40 = parse_lm0040_vc(find_frame(data, "0040VC"))

    for tag, live_val in (
        ("ELMODE", 9), ("WOL", 99), ("WPBR", 7), ("WLLML", 88), ("SPTPNT", 77), ("ELVL", 115),
    ):
        s = State()
        apply_bulk8101(s, p8101)
        apply_bulk0040(s, p40)
        ingest_live(s, tag, live_val)
        got, src = resolve(s, tag)
        record(f"live/{tag}", got == live_val and src == "live", tag, "live-ingest", got, live_val, src)

    s = State()
    apply_bulk8101(s, p8101)
    s.voiceName = "LIVE"
    s.voiceNameCharCount = 4
    record("live/VNAM", vnam_resolved(s) == "LIVE", "VNAM", "live-ingest", vnam_resolved(s), "LIVE", "live")

    s = State()
    s.hasParsedBulk0040 = True
    for tag, attr in (
        ("ATPBR", "lm0040AtpbrRaw"),
        ("PMASN", "lm0040PmasnRaw"),
        ("MCTUN", "lm0040MctunRaw"),
    ):
        setattr(s, attr, 42)
        got, src = resolve(s, tag)
        record(f"allow0040/{tag}", got == 42 and src == "0040", tag, "allow0040", got, 42, src)

    s = State()
    s.hasParsedBulk0040 = True
    for tag, attr in (
        ("AMASN", "lm0040AmasnRaw"),
        ("AMRNG", "lm0040AmrngRaw"),
        ("RNDP", "lm0040RndpRaw"),
    ):
        setattr(s, attr, 42)
        got, src = resolve(s, tag)
        record(f"block0040/{tag}", got < 0 and src == "None", tag, "block0040", got, -1, src)

    for tag in ("WPBR", "WLLML", "SPTPNT", "EFMODE"):
        s = State()
        s.hasParsedBulk0040 = True
        setattr(s, META[tag]["b0040"], 55)
        got, src = resolve(s, tag)
        record(f"allow0040/{tag}", got == 55 and src == "0040", tag, "allow0040", got, 55, src)

    for fid, fn, exp_efmode in (
        ("06", "06_efmode_off_ep_grndual.syx", 0),
        ("07", "07_efmode_serial_ep_grndual.syx", 1),
        ("08", "08_efmode_parallel_ep_grndual.syx", 2),
    ):
        data = (FIX / fn).read_bytes()
        p8101 = parse_lm8101_vc(find_lm8101_frame(data))
        p40 = parse_lm0040_vc(find_frame(data, "0040VC"))
        s = State()
        apply_bulk8101(s, p8101)
        apply_bulk0040(s, p40)
        got, src = resolve(s, "EFMODE")
        record(f"bulk/{fid}/EFMODE", got == exp_efmode and src == "0040", "EFMODE", fid, got, exp_efmode, src)

    grndual = FIX / "baseline_ep_grndual.syx"
    if grndual.is_file():
        data = grndual.read_bytes()
        p8101 = parse_lm8101_vc(find_lm8101_frame(data))
        p40 = parse_lm0040_vc(find_frame(data, "0040VC"))
        s = State()
        apply_bulk8101(s, p8101)
        apply_bulk0040(s, p40)
        got, src = resolve_efsdlv(s, 0)
        record("bulk/grndual/EFSDLV_E1", got == 127 and src == "0040", "EFSDLV", "grndual", got, 127, src)
        got2, src2 = resolve_efsdlv(s, 1)
        record("bulk/grndual/EFSDLV_E2", got2 == 127 and src2 == "0040", "EFSDLV", "grndual", got2, 127, src2)
        cat_e2 = catalog_efsdlv_raw0040(0, p40, 1)
        record("catalog/grndual/EFSDLV_E2_poison_slot", cat_e2 == 127, "EFSDLV", "catalog-poison0", cat_e2, 127, "0040")
        ui_e2 = catalog_efsdlv_ui_value(raw8101=0, raw0040=cat_e2)
        record("catalog/grndual/EFSDLV_E2_uiValue", ui_e2 == 127, "EFSDLV", "catalog-ui", ui_e2, 127, "0040")

        s2 = State()
        apply_bulk8101(s2, p8101)
        apply_bulk0040(s2, p40)
        s2.lmEfsdlvRaw = 106
        s2.elementEfsendlvlRaw[0] = 127
        got3, src3 = resolve_efsdlv(s2, 0)
        record("live/grndual/EFSDLV_E1", got3 == 127 and src3 == "live", "EFSDLV", "grndual-live", got3, 127, src3)

    print("=== REGISTRY PATH VERIFICATION ===")
    for key in sorted(results):
        print(f"{key}: {results[key]}")
    if first_fail:
        field, ctx, got, exp, src = first_fail
        print(f"\nFIRST FAIL: {field} @ {ctx} got={got} exp={exp} src={src}")
    else:
        print("\nALL CHECKS PASS")
    return 0 if all(v == "PASS" for v in results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())

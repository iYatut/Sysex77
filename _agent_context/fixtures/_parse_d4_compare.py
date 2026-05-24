#!/usr/bin/env python3
import sys
from pathlib import Path

FIX = Path(__file__).resolve().parent
sys.path.insert(0, str(FIX))

from _validate_bulk_parse import parse_lm8101_vc
from _parse_0040vc import parse_lm0040_vc

CAPTURE = Path.home() / "AppData/Roaming/Application Support/Sysex77/captures/AUTOSYNC-VC-INT.syx"
MM = 51  # D4


def eldt_ui(raw: int):
    if raw < 0:
        return None
    raw &= 0x7F
    delta = 9
    if raw > delta - 2:
        raw -= delta
        raw = ~raw
    return max(-7, min(7, raw))


def elns_ui(raw: int):
    if raw < 0:
        return None
    return max(-64, min(63, (raw & 0x7F) - 64))


def note_name(n: int) -> str:
    if n < 0:
        return "-"
    names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    return names[n % 12] + str(n // 12 - 2)


def outsel_label(v: int) -> str:
    if v < 0:
        return "-"
    return {0: "off", 2: "G1", 4: "G2", 6: "both"}.get(v & 0x06, hex(v))


def load_frames(data: bytes):
    frames8101, frames0040 = [], []
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
        if b"8101VC" in fr:
            frames8101.append(fr)
        if b"0040VC" in fr:
            frames0040.append(fr)
        i = j + 1
    return frames8101, frames0040


def main():
    data = CAPTURE.read_bytes()
    frames8101, frames0040 = load_frames(data)
    f8101 = frames8101[MM]
    off = data.find(bytes(f8101))
    f0040 = None
    for fr in frames0040:
        pos = data.find(bytes(fr))
        if off < pos < off + 3000:
            f0040 = fr
            break

    p = parse_lm8101_vc(f8101)
    p40 = parse_lm0040_vc(f0040) if f0040 else {}

    print(f"=== D4 (mm={MM}) bulk vs UI ===")
    print(f"VNAM: {p['name']!r}  ELMODE: {p['elmodeRaw']} (9=2AFM+2AWM)  WOL: {p['wolRaw']}")
    print()
    print(f"{'El':<4} {'Dump ELVL':<10} {'Dump Detune':<12} {'Dump NoteSh':<12} {'Dump OUTSEL':<12} {'Dump NoteLo-Hi':<16} {'Dump VelLo-Hi'}")
    for e in range(4):
        elvl = p["elvlRaw"][e] if e else p["elvlE1Raw"]
        os_r = p["outselRaw"][e] if e else p["outselE1Raw"]
        note_rng = f"{note_name(p['enllRaw'][e])}-{note_name(p['enlhRaw'][e])}"
        vel_rng = f"{p['evllRaw'][e]}-{p['evlhRaw'][e]}"
        print(
            f"E{e+1:<3} {elvl:<10} {eldt_ui(p['eldtRaw'][e]):!s:<12} {elns_ui(p['elnsRaw'][e]):!s:<12} "
            f"{outsel_label(os_r):<12} {note_rng:<16} {vel_rng}"
        )

    print()
    print(f"EFLN1EL(raw): {p['efln1ElRaw']}  EFSDLV: {p['efsdlvRaw']}")
    if p40.get("found0040"):
        fld = p40.get("fields", {})
        print("0040 Common:")
        for k in [
            "WPBR", "MCTUN", "RNDP", "SPTPNT", "ATPBR", "AFTMD",
            "PMRNG", "AMRNG", "FMRNG", "PNLRNG", "PNBRNG", "CORNG", "EGBRNG", "WLLML", "EFMODE",
        ]:
            if k in fld:
                print(f"  {k}: {fld[k]}")
    else:
        print("0040: not paired in capture")

    # UI reference from screenshot
    ui = {
        1: dict(elvl=127, det=-1, ns=-12, out="G1", nl="C-2-G8", vl="1-127"),
        2: dict(elvl=127, det=-1, ns=0, out="G1+G2", nl="C-2-G8", vl="1-127"),
        3: dict(elvl=0, det=-1, ns=6, out="off", nl="C-2-G8", vl="1-127"),
        4: dict(elvl=0, det=-1, ns=0, out="off", nl="C-2-G8", vl="1-127"),
    }
    print()
    print("=== Match UI screenshot ===")
    for e in range(4):
        elvl = p["elvlRaw"][e] if e else p["elvlE1Raw"]
        det = eldt_ui(p["eldtRaw"][e])
        ns = elns_ui(p["elnsRaw"][e])
        os_r = p["outselRaw"][e] if e else p["outselE1Raw"]
        out_dump = outsel_label(os_r)
        note_rng = f"{note_name(p['enllRaw'][e])}-{note_name(p['enlhRaw'][e])}"
        vel_rng = f"{p['evllRaw'][e]}-{p['evlhRaw'][e]}"
        u = ui[e + 1]
        ok = (
            elvl == u["elvl"]
            and det == u["det"]
            and ns == u["ns"]
            and note_rng.replace("G#", "G") == u["nl"]  # rough
            and vel_rng == u["vl"]
        )
        out_ok = (e == 1 and out_dump in ("G1", "both")) or (e == 0 and out_dump == "G1") or (e >= 2 and out_dump == "off")
        print(
            f"E{e+1}: ELVL {'OK' if elvl==u['elvl'] else 'MISMATCH dump='+str(elvl)+' ui='+str(u['elvl'])} | "
            f"Detune {'OK' if det==u['det'] else 'MISMATCH dump='+str(det)+' ui='+str(u['det'])} | "
            f"NoteShift {'OK' if ns==u['ns'] else 'MISMATCH dump='+str(ns)+' ui='+str(u['ns'])} | "
            f"OUTSEL dump={out_dump} ui={u['out']} | "
            f"NoteLim {note_rng} | Vel {vel_rng}"
        )


if __name__ == "__main__":
    main()

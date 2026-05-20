#!/usr/bin/env python3
"""Stage 1: discover 8101VC mixer-tail offsets vs lcd_reference."""
from __future__ import annotations

from pathlib import Path

FIX = Path(__file__).resolve().parent

LCD = {
    "01": {
        "eldt": (0, 0),
        "elns": (0, 0),
        "outsel_e1": 0x06,
        "outsel_e2": 0x06,
        "enll": 36,
        "enlh": 103,
        "evll": 1,
        "evlh": 127,
    },
    "02": {
        "eldt": (-1, 2),
        "elns": (0, 0),
        "outsel_e1": 0x06,
        "outsel_e2": 0x06,
        "enll": 36,
        "enlh": 103,
        "evll": 1,
        "evlh": 127,
    },
    "03": {
        "eldt": (2, -2),
        "elns": (0, 0),
        "outsel_e1": 0x06,
        "outsel_e2": 0x06,
        "enll": 36,
        "enlh": 103,
        "evll": 1,
        "evlh": 127,
        "efsend_lvl": 127,
    },
}


def find_frame(data: bytes, tag: str) -> bytes | None:
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
        if tag.encode() in frame:
            return frame
        i = j + 1
    return None


def wol_triple(fr: bytes) -> tuple[int | None, int | None, int | None]:
    for i in range(44, min(len(fr) - 4, 130)):
        if fr[i - 1] in (0x01, 0x03) and fr[i + 1] == 0x00 and fr[i + 2] == 0x00:
            return i, i + 3, i + 4
    return None, None, None


def find_anchors(fr: bytes) -> list[int]:
    out: list[int] = []
    for a in range(90, min(len(fr) - 7, 200)):
        if fr[a : a + 3] == bytes([0x7F, 0x01, 0x7F]):
            out.append(a)
    return out


def elns_sysex(ui: int) -> int:
    return max(0, min(127, ui + 64))


def main() -> None:
    for fid, fn in [
        ("01", "01_init_anonim_07x1_voice.syx"),
        ("02", "02_ap_crsrock_07x1_voice.syx"),
        ("03", "03_ep_classic_07x1_voice.syx"),
    ]:
        fr = find_frame((FIX / fn).read_bytes(), "8101VC")
        assert fr is not None
        w, e, o = wol_triple(fr)
        ans = find_anchors(fr)
        lcd = LCD[fid]
        print(f"\n=== {fid} elmode@{32}={fr[32]} wol@{w} elvl1@{e} outsel1@{o}")
        print(f"  outsel1 raw=0x{fr[o]:02X} bits=0x{fr[o] & 0x06:02X}")
        if ans:
            a = ans[0]
            blk = fr[a : a + 10]
            print(
                f"  anchor0@{a}: +4 outsel=0x{blk[4] & 0x06:02X} "
                f"+5 elvl={blk[5]} +6 eldt={blk[6]} (expect E2 eldt={lcd['eldt'][1]})"
            )
        if len(ans) > 1:
            a = ans[1]
            blk = fr[a : a + 10]
            print(f"  anchor1@{a}: bytes +3..+9 = {list(blk[3:10])}")

        end = ans[0] if ans else (o or 98) + 20
        for off in range(o or 98, end):
            if fr[off] == lcd["eldt"][0]:
                print(f"  E1 ELDT raw UI match @{off} (rel outsel {off - (o or 98)}) = {fr[off]}")
            enc = lcd["eldt"][0] + 7
            if -7 <= lcd["eldt"][0] <= 7 and fr[off] == enc:
                print(f"  E1 ELDT ui+7 match @{off} = {enc}")

        for off in range(o or 98, end + 15):
            if fr[off] == elns_sysex(lcd["elns"][0]):
                print(f"  E1 ELNS sysex match @{off} ui={lcd['elns'][0]}")

        for name, val in [
            ("ENLL", lcd["enll"]),
            ("ENLH", lcd["enlh"]),
            ("EVLL", lcd["evll"]),
            ("EVLH", lcd["evlh"]),
        ]:
            hits = [i for i in range(o or 98, min((o or 98) + 40, len(fr))) if fr[i] == val]
            if hits:
                print(f"  {name}={val} hits rel outsel: {[h - (o or 98) for h in hits[:5]]}")


if __name__ == "__main__":
    main()

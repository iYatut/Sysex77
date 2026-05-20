#!/usr/bin/env python3
"""Discover 8101/0040 offsets from fixtures 01-03."""
from __future__ import annotations

from pathlib import Path

FIX = Path(__file__).resolve().parent

LCD = {
    "01": {
        "elmode": 8,
        "eldt": (0, 0),
        "elns": (0, 0),
        "enll": 36,
        "enlh": 103,
        "evll": 1,
        "evlh": 127,
        "outsel": (0x06, 0x06),
    },
    "02": {
        "elmode": 8,
        "eldt": (-1, 2),
        "elns": (0, 0),
        "enll": 36,
        "enlh": 103,
        "evll": 1,
        "evlh": 127,
        "outsel": (0x06, 0x06),
    },
    "03": {
        "elmode": 4,
        "eldt": (2, -2),
        "elns": (0, 0),
        "enll": 36,
        "enlh": 103,
        "evll": 1,
        "evlh": 127,
        "outsel": (0x06, 0x06),
    },
}

G02_ORDER = [
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x42, 0x43,
]


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
    return [
        a
        for a in range(90, min(len(fr) - 7, 200))
        if fr[a : a + 3] == bytes([0x7F, 0x01, 0x7F])
    ]


def dump_mixer_region() -> None:
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
        print(f"\n=== {fid} {fn} elmode={lcd['elmode']} outsel@{o} anchors={ans}")
        end = ans[0] + 10 if ans else (o or 0) + 15
        for i in range(o or 0, end):
            rel_o = i - (o or 0)
            rel_a = i - ans[0] if ans else -999
            print(f"  @{i:3d} rel_outsel={rel_o:2d} rel_anchor={rel_a:2d} = 0x{fr[i]:02X} ({fr[i]:3d})")


def try_0040_sequential() -> None:
    """0040 payload may be sequential raw bytes for G02 params in NN order."""
    print("\n=== 0040 sequential NN-order hypothesis @payload ===")
    for fn in ["01_init_anonim_07x1_voice.syx", "02_ap_crsrock_07x1_voice.syx", "03_ep_classic_07x1_voice.syx"]:
        f40 = find_frame((FIX / fn).read_bytes(), "0040VC")
        assert f40 is not None
        # payload starts @33 (byte after header constants)
        start = 33
        vals = list(f40[start : start + len(G02_ORDER)])
        print(f"\n{fn} @{start}..:")
        for nn, v in zip(G02_ORDER, vals):
            print(f"  NN=0x{nn:02X} raw={v:3d} 0x{v:02X}")


def try_0040_pair_run() -> None:
    """Hypothesis: 00-separated value runs; compare stable bytes across fixtures."""
    print("\n=== 0040 value-only run (skip 00 bytes) @36-56 ===")
    for fn in ["01_init_anonim_07x1_voice.syx", "02_ap_crsrock_07x1_voice.syx"]:
        f40 = find_frame((FIX / fn).read_bytes(), "0040VC")
        assert f40 is not None
        vals = [f40[i] for i in range(36, 57) if f40[i] != 0x00]
        print(f"{fn}: {vals}")


def main() -> None:
    dump_mixer_region()
    try_0040_sequential()
    try_0040_pair_run()


if __name__ == "__main__":
    main()

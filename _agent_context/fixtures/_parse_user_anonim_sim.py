#!/usr/bin/env python3
"""Simulate C++ parseLm8101VcMinimal for user ANONIM dump."""
from __future__ import annotations

D1 = bytes(
    int(x, 16)
    for x in """
F0 43 0 7A 2 4E 4C 4D 20 20 38 31 30 31 56 43 0 0 0 0 0 0 0 0 0 0 0 0 0 0 7F 0 6
41 4E 4F 4E 49 4D 20 20 20 20 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
1 0 1 40 C 23 1 0 D 0 C 0 A 0 2 0 E 0 0 1 0 0 3 7F 0 0 34 0 44 25 61 1 7F 20 2 7F 0 40 0 7F 1 7F 20 6 0 0 40 0 3C 40 3 3F 3F 3F 3F 40 40 40 40 40 1 0 0 41 0 0 0 0 0 0 0 0 7F 1 0 0 0 0 0 0 40 40 40 40 40 40 40 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 7F 1 0 0 0 0 0 0 40 40 40 40 40 40 40 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 0 0 0 3F 3F 3F 0 3F 3F 3F 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 0 0 0 0 40 0 3C 40 3 3F 3F 3F 3F 40 40 40 40 40 1 0 0 41 0 0 0 0 0 0 0 0 7F 1 0 0 0 0 0 0 40 40 40 40 40 40 40 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 7F 1 0 0 0 0 0 0 40 40 40 40 40 40 40 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 0 0 0 3F 3F 3F 0 3F 3F 3F 0 24 37 4C 60 1 0 1 0 1 0 1 0 0 0 0 0 21 F7
""".split()
)

D2 = bytes(
    int(x, 16)
    for x in """
F0 43 0 7A 0 6B 4C 4D 20 20 30 30 34 30 56 43 0 0 0 0 0 0 0 0 0 0 0 0 0 0 7F 0 6
2 0 0 19 0 8 0 A 2 3F 1 47 0 4 0 37 0 7 0 6 0 14 64 64 0 0 19 0 8 0 A 2 3F 1 47 0 4 0 37 0 7 0 6 0 14 2B 64 64 32 32 0 0 0 63 0 0 0 63 0 41 0 64 0 0 0 40 F 7F 9 0 F 7F 0 0 4 0 3F 4 0 3F 2E F7
""".split()
)


def eldt_ui(raw: int) -> int:
    raw &= 0x7F
    if raw <= 63:
        return raw
    d = 9
    if raw > d - 2:
        raw -= d
        raw = ~raw & 0x7F
    return max(-7, min(7, raw if raw <= 63 else raw - 128))


def elns_ui(raw: int) -> int:
    return max(-64, min(63, raw - 64))


def outsel_from_packed(packed: int, eldt_shares: bool) -> int:
    masked = packed & 0x06
    if eldt_shares and packed != masked:
        return 0
    return masked


def find_wol_elvl(f: bytes) -> tuple[int, int, int, int] | None:
    best = None
    best_score = 9999
    first = None
    for i in range(44, min(len(f) - 4, 130) + 1):
        p = f[i - 1]
        if p not in (0x03, 0x01):
            continue
        if f[i + 1] or f[i + 2]:
            continue
        wol, elvl = f[i], f[i + 3]
        if wol > 127 or elvl > 127:
            continue
        if first is None:
            first = (i, i + 3, wol, elvl)
        if 93 <= i <= 96:
            score = (0 if p == 0x03 else 10) + (96 - i)
            if score < best_score:
                best_score = score
                best = (i, i + 3, wol, elvl)
    return best or first


def find_anchor(f: bytes, start=90) -> int | None:
    for a in range(start, min(len(f) - 7, 125) + 1):
        if f[a : a + 3] == bytes([0x7F, 0x01, 0x7F]):
            return a
    return None


def parse8101(f: bytes) -> dict:
    elmode = f[32]
    eldt_e1_strip = elmode in (1, 4, 6, 8)
    eldt_e2_anchor = elmode in (4, 8)
    pair = find_wol_elvl(f)
    r: dict = {"elmode": elmode, "found_pair": pair is not None}
    if not pair:
        return r
    wol_off, elvl_off, wol, elvl = pair
    outsel_off = elvl_off + 1
    r.update(
        {
            "wol_off": wol_off,
            "elvl_off": elvl_off,
            "WOL": wol,
            "ELVL_E1": elvl,
            "OUTSEL_E1_packed": f[outsel_off] if outsel_off < len(f) else None,
            "OUTSEL_E1": outsel_from_packed(f[outsel_off], eldt_e1_strip)
            if outsel_off < len(f)
            else None,
            "ELDT_E1_raw": f[outsel_off] if eldt_e1_strip and outsel_off < len(f) else None,
            "ELDT_E1": eldt_ui(f[outsel_off])
            if eldt_e1_strip and outsel_off < len(f)
            else None,
            "ELNS_E1_raw": f[outsel_off + 1] if outsel_off + 1 < len(f) else None,
            "ELNS_E1": elns_ui(f[outsel_off + 1]) if outsel_off + 1 < len(f) else None,
        }
    )
    a = find_anchor(f, 98)
    if a is not None and elmode in (1, 2, 4, 6, 7, 8, 9):
        r["anchor"] = a
        r["ELVL_E2"] = f[a + 5]
        r["OUTSEL_E2"] = f[a + 4] & 0x06
        r["ELDT_E2_raw"] = f[a + 6] if eldt_e2_anchor else None
        r["ELDT_E2"] = eldt_ui(f[a + 6]) if eldt_e2_anchor else "(not parsed for elmode 6)"
        r["ELNS_E2"] = elns_ui(f[a + 7])
    return r


def main() -> None:
    print(f"8101 frame: {len(D1)} bytes (full ~586)")
    print(f"0040 frame: {len(D2)} bytes")
    p = parse8101(D1)
    print("\nC++-like 8101 parse:")
    for k, v in p.items():
        print(f"  {k}: {v}")

    print("\n0040 tail:")
    print(f"  WPBR@41 = {D2[41]}")
    print(f"  SPTPNT@98 = {D2[98]} (UI screenshot: 60)")
    print(f"  AFTMD@100 = {D2[100]}")

    print("\nUI screenshot vs bulk parse:")
    ui = {
        "ELVL_E1": 124,
        "ELVL_E2": 127,
        "ELDT_E1": 0,
        "ELDT_E2": 0,
        "ELNS_E1": 0,
        "ELNS_E2": 0,
        "OUTSEL_E1": 2,
        "OUTSEL_E2": 4,
        "SPTPNT": 60,
    }
    bulk = {
        "ELVL_E1": p.get("ELVL_E1"),
        "ELVL_E2": p.get("ELVL_E2"),
        "ELDT_E1": p.get("ELDT_E1"),
        "ELDT_E2": p.get("ELDT_E2"),
        "ELNS_E1": p.get("ELNS_E1"),
        "ELNS_E2": p.get("ELNS_E2"),
        "OUTSEL_E1": p.get("OUTSEL_E1"),
        "OUTSEL_E2": p.get("OUTSEL_E2"),
        "SPTPNT": D2[98],
    }
    for k in ui:
        match = "OK" if ui[k] == bulk.get(k) else "MISMATCH"
        print(f"  {k:10} UI={ui[k]!s:6} bulk={bulk.get(k)!s:6} {match}")


if __name__ == "__main__":
    main()

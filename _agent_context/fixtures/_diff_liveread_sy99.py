#!/usr/bin/env python3
"""Compare LIVEREAD bulk bytes vs user-reported SY99 LCD values."""
from pathlib import Path

SY99 = {
    "WOL": 127,
    "ELVL_E1": 124,
    "ELVL_E2": 127,
    "ELNS_E1": -12,
    "ELNS_E2": 0,
    "ELDT_E1": 3,
    "ELDT_E2": -3,
    "EFMODE": 0,  # Off
    "EFLN1EL": 0,  # off (only bits 0x01|0x04; user says off)
}

PC_PARSER = {
    "WOL": 115,
    "ELVL_E1": 127,
    "ELVL_E2": 127,
    "ELNS_E1": 0,
    "ELNS_E2": 12,
    "ELDT_E1": -1,  # not applied
    "ELDT_E2": 0,
    "EFMODE": 1,
    "EFLN1EL": 0x03,
}


def elns_sysex(ui: int) -> int:
    return (ui + 64) & 0x7F


def main() -> None:
    data = Path(
        r"c:\Users\user\AppData\Roaming\Application Support\Sysex77\captures\LIVEREAD-20260521-161532.syx"
    ).read_bytes()
    frame = data[0:587]
    f40 = data[587:702]

    print("SY99 vs parser output:\n")
    for k in SY99:
        p = PC_PARSER.get(k, "?")
        ok = "OK" if p == SY99[k] else "MISMATCH"
        print(f"  {k:12} SY99={SY99[k]:4}  parser={p!s:6}  {ok}")

    print("\nExpected SysEx bytes for SY99 note shift:")
    print(f"  ELNS E1 -12 -> VV 0x{elns_sysex(-12):02X}")
    print(f"  ELNS E2   0 -> VV 0x{elns_sysex(0):02X}")

    print("\nWOL/ELVL pairs in frame:")
    for i in range(44, 130):
        if frame[i - 1] in (0x03, 0x01) and frame[i + 1] == 0 and frame[i + 2] == 0:
            wol, elvl = frame[i], frame[i + 3]
            if wol <= 127 and elvl <= 127:
                tags = []
                if wol == SY99["WOL"]:
                    tags.append("WOL match SY99")
                if elvl == SY99["ELVL_E1"]:
                    tags.append("ELVL E1 match SY99")
                if wol == PC_PARSER["WOL"]:
                    tags.append("parser WOL")
                if elvl == PC_PARSER["ELVL_E1"]:
                    tags.append("parser ELVL E1")
                print(f"  @{i}: WOL={wol:3} ELVL={elvl:3}  {' | '.join(tags)}")

    outsel = 99
    print(f"\nE1 strip @ outsel {outsel}:")
    print(f"  +0 OUTSEL 0x{frame[outsel]:02X}")
    print(f"  +1 ELNS   0x{frame[outsel+1]:02X} -> UI {frame[outsel+1]-64}")
    print(f"  +3 EVLH   {frame[outsel+3]}")
    print(f"  +4 EVLL   {frame[outsel+4]}")

    for a in range(90, 120):
        if frame[a : a + 3] == bytes([0x7F, 0x01, 0x7F]):
            print(f"\nAnchor @{a}:")
            print(f"  +4 OUTSEL 0x{frame[a+4]:02X}")
            print(f"  +5 ELVL   {frame[a+5]}")
            print(f"  +6 ELDT   {frame[a+6]} (raw)")
            print(f"  +7 ELNS   0x{frame[a+7]:02X} -> UI {frame[a+7]-64}")

    print(f"\n0040 EFMODE @33 = {f40[33]} (SY99 wants 0=Off)")

    efln_off = 133
    print(f"\n@efln_off {efln_off}: EFLN=0x{frame[efln_off]:02X} (SY99 off=0; parser 0x03=send1+bit2)")


if __name__ == "__main__":
    main()

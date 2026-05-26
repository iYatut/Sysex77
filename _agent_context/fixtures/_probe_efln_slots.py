#!/usr/bin/env python3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _validate_bulk_parse import (
    find_wol_elvl_e1_pair,
    efln1_el_raw_from_mixer_strip,
    efln_wire_masked,
    parse_lm8101_vc,
)

cap = Path.home() / "AppData/Roaming/Application Support/Sysex77/captures/AUTOSYNC-VC-INT.syx"
data = cap.read_bytes()
frames = []
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
        mm = frame[31] if len(frame) > 31 else -1
        p = parse_lm8101_vc(frame)
        frames.append((mm, p["name"], frame, p))
    i = j + 1

for slot in [5, 7, 28, 40]:
    hit = [x for x in frames if x[0] == slot]
    if not hit:
        print(f"slot={slot}: NOT FOUND")
        continue
    mm, name, frame, p = hit[0]
    pair = find_wol_elvl_e1_pair(frame)
    elvl_off = pair[1] if pair else -1
    elvl = p["elvlE1Raw"]
    fn = efln1_el_raw_from_mixer_strip(frame, elvl_off, elvl)
    print(
        f"slot={slot} name={name!r} elvl={elvl} elvlOff={elvl_off} "
        f"parsed_efln={p['efln1ElRaw']} fn={fn}"
    )
    for d in [28, 26, 35, 39]:
        o = elvl_off + d
        if o < len(frame):
            print(f"  +{d} @{o}: raw=0x{frame[o]:02X} masked={efln_wire_masked(frame[o])}")

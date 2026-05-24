#!/usr/bin/env python3
"""
Generate sy99_master_catalog.json + sy99_param_bindings.json from manual tables.

Sources:
  _agent_context/sy99_sysex_complete.md
  _agent_context/fixtures/lcd_reference.md (LCD page hints)
  ui/fixtures/sy99CatalogSpecs.json (37 confirmed registry params)
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MD_PATH = ROOT / "_agent_context" / "sy99_sysex_complete.md"
SPECS_PATH = ROOT / "ui" / "fixtures" / "sy99CatalogSpecs.json"
OUT_CATALOG = ROOT / "_agent_context" / "fixtures" / "sy99_master_catalog.json"
OUT_BINDINGS = ROOT / "_agent_context" / "fixtures" / "sy99_param_bindings.json"

GROUP_LABELS = {
    "02": "Voice Common",
    "03": "Normal Element",
    "04": "Drum Set",
    "05": "AFM Element Common",
    "07": "AWM Element Data",
    "08": "Effect Data",
    "09": "Filter Data",
    "0A": "Pan Data",
    "0B": "Micro Tuning",
    "0D": "Master Control",
}

PER_ELEMENT_GROUPS = {"03", "05", "07", "09", "0A"}

LCD_PAGES = {
    "ELMODE": 200,
    "WOL": 212,
    "ELDT": 203,
    "ELNS": 204,
    "ENLL": 205,
    "ENLH": 205,
    "EVLL": 206,
    "EVLH": 206,
    "OUTSEL": 208,
    "PANNM": 207,
}

# Registry id -> bindStatus (rest of catalog = manual_only)
REGISTRY_BIND = {
    "ELMODE": "confirmed_bulk8101",
    "WOL": "confirmed_bulk8101",
    "ELVL": "confirmed_bulk8101",
    "ELDT": "confirmed_bulk8101",
    "ELNS": "confirmed_bulk8101",
    "EVLL": "confirmed_bulk8101",
    "EVLH": "confirmed_bulk8101",
    "OUTSEL": "confirmed_bulk8101",
    "EFLN1EL": "confirmed_bulk8101",
    "EFSDLV": "confirmed_bulk8101",
    "EFSDVSNS": "manual_only",
    "EFSDSCL": "manual_only",
    "ENLL": "confirmed_bulk8101",
    "ENLH": "confirmed_bulk8101",
    "WPBR": "confirmed_bulk0040",
    "ATPBR": "confirmed_bulk0040",
    "PMASN": "confirmed_bulk0040",
    "PMRNG": "confirmed_bulk0040",
    "AMASN": "confirmed_bulk0040",
    "AMRNG": "confirmed_bulk0040",
    "FMASN": "confirmed_bulk0040",
    "FMRNG": "confirmed_bulk0040",
    "PNLASN": "confirmed_bulk0040",
    "PNLRNG": "confirmed_bulk0040",
    "COASN": "confirmed_bulk0040",
    "CORNG": "confirmed_bulk0040",
    "PNBASN": "confirmed_bulk0040",
    "PNBRNG": "confirmed_bulk0040",
    "EGBASN": "confirmed_bulk0040",
    "EGBRNG": "confirmed_bulk0040",
    "WLASN": "confirmed_bulk0040",
    "WLLML": "confirmed_bulk0040",
    "MCTUN": "confirmed_bulk0040",
    "RNDP": "manual_only",
    "AFTMD": "manual_only",
    "SPTPNT": "confirmed_bulk0040",
    "EFMODE": "confirmed_bulk0040",
}

REGISTRY_BULK_READ: dict[str, dict] = {
    "ELMODE": {"frame": "8101", "field": "elmodeRaw"},
    "WOL": {"frame": "8101", "field": "wolRaw"},
    "ELVL": {"frame": "8101", "field": "elvlRaw", "perElement": True},
    "ELDT": {"frame": "8101", "field": "lmEldtRaw", "perElement": True},
    "ELNS": {"frame": "8101", "field": "lmElnsRaw", "perElement": True},
    "EVLL": {"frame": "8101", "field": "lmEvllRaw", "perElement": True},
    "EVLH": {"frame": "8101", "field": "lmEvlhRaw", "perElement": True},
    "OUTSEL": {"frame": "8101", "field": "lmOutselRaw", "perElement": True},
    "EFLN1EL": {"frame": "8101", "field": "lmEfln1ElRaw", "maxElementIndex": 0},
    "EFSDLV": {"frame": "8101", "field": "lmEfsdlvRaw", "maxElementIndex": 0},
    "ENLL": {"frame": "8101", "field": "lmEnllRaw", "perElement": True},
    "ENLH": {"frame": "8101", "field": "lmEnlhRaw", "perElement": True},
    "WPBR": {"frame": "0040", "field": "wpbrRaw"},
    "ATPBR": {"frame": "0040", "field": "atpbrRaw"},
    "PMASN": {"frame": "0040", "field": "pmasnRaw"},
    "PMRNG": {"frame": "0040", "field": "pmrngRaw"},
    "AMASN": {"frame": "0040", "field": "amasnRaw"},
    "AMRNG": {"frame": "0040", "field": "amrngRaw"},
    "FMASN": {"frame": "0040", "field": "fmasnRaw"},
    "FMRNG": {"frame": "0040", "field": "fmrngRaw"},
    "PNLASN": {"frame": "0040", "field": "pnlasnRaw"},
    "PNLRNG": {"frame": "0040", "field": "pnlrngRaw"},
    "COASN": {"frame": "0040", "field": "coasnRaw"},
    "CORNG": {"frame": "0040", "field": "corngRaw"},
    "PNBASN": {"frame": "0040", "field": "pnbasnRaw"},
    "PNBRNG": {"frame": "0040", "field": "pnbrngRaw"},
    "EGBASN": {"frame": "0040", "field": "egbasnRaw"},
    "EGBRNG": {"frame": "0040", "field": "egbrngRaw"},
    "WLASN": {"frame": "0040", "field": "wlasnRaw"},
    "WLLML": {"frame": "0040", "field": "wllmlRaw"},
    "MCTUN": {"frame": "0040", "field": "mctunRaw"},
    "SPTPNT": {"frame": "0040", "field": "sptpntRaw"},
    "EFMODE": {"frame": "0040", "field": "efmodeRaw"},
}

REGISTRY_IDS = set(REGISTRY_BIND.keys())


def parse_nn_cell(cell: str) -> list[int]:
    cell = cell.strip()
    m = re.match(r"^([0-9A-Fa-f]{2})-([0-9A-Fa-f]{2})$", cell)
    if m:
        a, b = int(m.group(1), 16), int(m.group(2), 16)
        return list(range(a, b + 1))
    m = re.match(r"^([0-9A-Fa-f]{2})$", cell)
    if m:
        return [int(m.group(1), 16)]
    return []


def expand_name(name: str, nn: int, nn_list: list[int]) -> list[tuple[int, str]]:
    name = name.strip()
    m = re.match(r"^([A-Z0-9]+)(\d+)-(\d+)$", name)
    if m:
        prefix, start, end = m.group(1), int(m.group(2)), int(m.group(3))
        return [(nn_list[i], f"{prefix}{start + i}") for i in range(end - start + 1) if i < len(nn_list)]
    m = re.match(r"^([A-Z0-9]+)(\d+)-(\d+)$", name.replace(" ", ""))
    if m:
        prefix, start, end = m.group(1), int(m.group(2)), int(m.group(3))
        count = end - start + 1
        return [(nn_list[i] if i < len(nn_list) else nn + i, f"{prefix}{start + i}") for i in range(count)]

    m = re.match(r"^([A-Z0-9]+)(\d+)-(\d+)$", name.replace("EFSDVL", "EFSDVSNS"))
    # VNAM0-9 style in name column
    m2 = re.match(r"^([A-Z]+)(\d+)-(\d+)$", name)
    if m2:
        prefix, start, end = m2.group(1), int(m2.group(2)), int(m2.group(3))
        return [(nn_list[i], f"{prefix}{start + i}") for i in range(end - start + 1)]

    # MCTEN/OUTOSEL -> OUTSEL for registry alignment
    if "OUTSEL" in name.upper() or "OUTOSEL" in name.upper():
        return [(nn, "OUTSEL")]
    if name.startswith("MCTEN"):
        return [(nn, "OUTSEL")]

    # Normalize aliases
    tag = name.split("/")[0].strip()
    tag = re.sub(r"[^A-Z0-9]", "", tag.upper())
    if tag.startswith("EFSDVL"):
        tag = "EFSDVSNS"
    return [(nn, tag)]


def infer_codec(tag: str, range_str: str) -> str:
    r = range_str.lower()
    if tag == "ELDT":
        return "signed7_detune"
    if tag in ("ELNS",) or "-64" in r:
        return "note_shift"
    if tag in ("EFSDVSNS", "EFSDSCL") or ("-7" in r and "+7" in r):
        return "signed7_ladder"
    if tag == "ATPBR" or ("-12" in r and "+12" in r):
        return "signed_atpbr"
    if "ascii" in r:
        return "ascii"
    if "off/on" in r or "enum" in r or "/" in range_str:
        return "enum"
    if "bits" in r:
        return "bits"
    return "identity"


def parse_ui_range(range_str: str) -> tuple[int | None, int | None]:
    r = range_str.strip()
    m = re.search(r"(-?\d+)\.\.(-?\d+)", r)
    if m:
        return int(m.group(1)), int(m.group(2))
    m = re.search(r"(-?\d+)-(-?\d+)", r)
    if m:
        return int(m.group(1)), int(m.group(2))
    m = re.search(r"0-(\d+)", r)
    if m:
        return 0, int(m.group(1))
    return None, None


def parse_md_tables(text: str) -> list[dict]:
    entries: list[dict] = []
    current_group: str | None = None
    in_table = False

    for line in text.splitlines():
        gm = re.match(r"^## Group ([0-9A-Fa-f]{2})", line)
        if gm:
            current_group = gm.group(1).upper()
            in_table = False
            continue
        if current_group is None:
            continue
        if line.strip().startswith("| NN"):
            in_table = True
            continue
        if not in_table or not line.strip().startswith("|"):
            if in_table and line.strip() == "":
                in_table = False
            continue
        if re.match(r"^\|\s*-+", line):
            continue

        parts = [p.strip() for p in line.strip().strip("|").split("|")]
        if len(parts) < 3:
            continue
        nn_cell, name_cell, range_cell = parts[0], parts[1], parts[2]
        desc = parts[3] if len(parts) > 3 else ""

        nn_list = parse_nn_cell(nn_cell)
        if not nn_list:
            # EF1PRM1-10 with single nn 22-2B handled via name expansion
            if "-" in nn_cell and re.match(r"^[0-9A-Fa-f]{2}-[0-9A-Fa-f]{2}$", nn_cell.replace(" ", "")):
                a, b = nn_cell.split("-")
                nn_list = list(range(int(a, 16), int(b, 16) + 1))

        if not nn_list:
            continue

        pairs = expand_name(name_cell, nn_list[0], nn_list)
        if len(pairs) == 1 and len(nn_list) > 1 and pairs[0][0] == nn_list[0]:
            # name like EF1PRM1-10 with nn range
            m = re.match(r"^([A-Z0-9]+)(\d+)-(\d+)$", name_cell.strip())
            if m:
                prefix, start, end = m.group(1), int(m.group(2)), int(m.group(3))
                pairs = [(nn_list[i], f"{prefix}{start + i}") for i in range(min(len(nn_list), end - start + 1))]

        ui_min, ui_max = parse_ui_range(range_cell)
        group_label = GROUP_LABELS.get(current_group, f"Group {current_group}")
        per_element = current_group in PER_ELEMENT_GROUPS

        for nn, tag in pairs:
            if not tag or tag == "MCTEN":
                tag = "OUTSEL"
            ui_label = desc.split(";")[0].strip() if desc else tag
            if len(ui_label) > 80:
                ui_label = tag

            entry = {
                "id": tag,
                "tag": tag,
                "uiLabel": ui_label,
                "group": group_label,
                "sysexGroup": int(current_group, 16),
                "nn": nn,
                "perElement": per_element,
                "elementCount": 4 if per_element else 1,
                "uiMin": ui_min,
                "uiMax": ui_max,
                "codec": infer_codec(tag, range_cell),
                "sy99LcdPage": LCD_PAGES.get(tag, 0),
                "addressHex": f"{int(current_group, 16):02X} EE 00 {nn:02X}",
                "registryId": tag if tag in REGISTRY_IDS else None,
            }
            entries.append(entry)

    return entries


def dedupe_catalog(entries: list[dict]) -> list[dict]:
    seen: set[tuple[str, int]] = set()
    out: list[dict] = []
    for e in entries:
        key = (e["id"], e["nn"])
        if key in seen:
            continue
        seen.add(key)
        out.append(e)
    return out


def build_bindings(catalog: list[dict]) -> list[dict]:
    bindings: list[dict] = []
    for tag, status in REGISTRY_BIND.items():
        bulk = "8101" if "8101" in status else ("0040" if "0040" in status else None)
        item: dict = {
            "id": tag,
            "bindStatus": status,
            "registryId": tag,
            "bulkSource": bulk,
            "notes": "Sy99ParamRegistry confirmed" if "confirmed" in status else "",
        }
        read_spec = REGISTRY_BULK_READ.get(tag)
        if read_spec is not None:
            item["bulkRead"] = read_spec
        bindings.append(item)
    return bindings


def main() -> int:
    if not MD_PATH.is_file():
        print(f"Missing {MD_PATH}", file=sys.stderr)
        return 1

    text = MD_PATH.read_text(encoding="utf-8")
    catalog = dedupe_catalog(parse_md_tables(text))
    bindings = build_bindings(catalog)

    OUT_CATALOG.parent.mkdir(parents=True, exist_ok=True)
    OUT_CATALOG.write_text(json.dumps(catalog, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    OUT_BINDINGS.write_text(json.dumps(bindings, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    # Mirror to ui/fixtures for npm tooling
    ui_catalog = ROOT / "ui" / "fixtures" / "sy99_master_catalog.json"
    ui_bindings = ROOT / "ui" / "fixtures" / "sy99_param_bindings.json"
    ui_catalog.write_text(OUT_CATALOG.read_text(encoding="utf-8"), encoding="utf-8")
    ui_bindings.write_text(OUT_BINDINGS.read_text(encoding="utf-8"), encoding="utf-8")

    reg_in_cat = sum(1 for e in catalog if e.get("registryId"))
    print(f"Catalog: {len(catalog)} params ({reg_in_cat} with registryId)")
    print(f"Bindings: {len(bindings)} registry entries")
    print(f"Wrote {OUT_CATALOG}")
    print(f"Wrote {OUT_BINDINGS}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

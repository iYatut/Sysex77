#!/usr/bin/env python3
"""
Compare parseLm8101VcMinimal:
  OLD — parseLm8101VcMinimal(buffer): findLmBlock on full .syx bytes
  NEW — parseLm8101VcMinimalFromSysexMessages: per-message find + concat fallback

Mirrors YamahaLmVoiceDump.h glue (not audit logs).
Uses parse_lm8101_vc from _validate_bulk_parse for the actual field extraction.
"""
from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path

from _validate_bulk_parse import parse_lm8101_vc

FIX = Path(__file__).resolve().parent
K_LM_TAG_OFFSET_FROM_LM = 4
K_MIN_8101_VC_FRAME_SIZE = 100


@dataclass
class LmBlockView:
    data: memoryview | None
    size: int
    offset: int = 0


@dataclass
class SysexMessage:
    """Minimal stand-in for juce::MidiMessage SysEx capture."""

    raw: bytes
    force_sysex: bool = False

    def is_sysex(self) -> bool:
        if self.force_sysex:
            return len(self.raw) > 0
        return len(self.raw) >= 2 and self.raw[0] == 0xF0 and self.raw[-1] == 0xF7

    def get_raw_data(self) -> bytes:
        return self.raw

    def get_sys_ex_data(self) -> bytes:
        if not self.raw:
            return b""
        if len(self.raw) >= 2 and self.raw[0] == 0xF0 and self.raw[-1] == 0xF7:
            return self.raw[1:-1]
        if self.raw[0] == 0xF0:
            return self.raw[1:]
        return self.raw


def tags_equal6(buf: bytes, tag6: str) -> bool:
    return len(buf) >= 6 and buf[:6] == tag6.encode("ascii")


def find_lm_block(data: bytes | memoryview, tag6: str) -> LmBlockView:
    if data is None or len(data) < 16:
        return LmBlockView(None, 0)

    bytes_mv = memoryview(data) if not isinstance(data, memoryview) else data
    n = len(bytes_mv)

    for i in range(n - 10):
        if bytes_mv[i] != ord("L") or bytes_mv[i + 1] != ord("M"):
            continue
        if not tags_equal6(bytes_mv[i + K_LM_TAG_OFFSET_FROM_LM : i + K_LM_TAG_OFFSET_FROM_LM + 6], tag6):
            continue

        start = i
        while start > 0 and bytes_mv[start] != 0xF0:
            start -= 1
        if bytes_mv[start] != 0xF0:
            continue

        end = start
        while end < n and bytes_mv[end] != 0xF7:
            end += 1
        if end >= n or bytes_mv[end] != 0xF7:
            continue

        frame = bytes_mv[start : end + 1]
        return LmBlockView(frame, len(frame), start)

    return LmBlockView(None, 0)


def append_wrapped_sysex_body(body: bytes) -> bytes | None:
    if not body or body[0] != 0x43:
        return None
    return bytes([0xF0]) + body + bytes([0xF7])


def find_lm_block_in_sysex_messages(messages: list[SysexMessage], tag6: str) -> LmBlockView:
    for m in messages:
        if not m.is_sysex():
            continue

        raw = m.get_raw_data()
        if raw:
            block = find_lm_block(raw, tag6)
            if block.data is not None:
                return block

        body = m.get_sys_ex_data()
        if not body:
            continue

        if body[0] == 0xF0:
            block = find_lm_block(body, tag6)
            if block.data is not None:
                return block
            continue

        if body[0] == 0x43:
            wrapped = append_wrapped_sysex_body(body)
            if wrapped:
                block = find_lm_block(wrapped, tag6)
                if block.data is not None:
                    return block

    return LmBlockView(None, 0)


def concat_live_read_sysex_raw(messages: list[SysexMessage]) -> bytes:
    out = bytearray()
    for m in messages:
        if not m.is_sysex():
            continue
        raw = m.get_raw_data()
        if raw:
            out.extend(raw)
            continue
        body = m.get_sys_ex_data()
        if body:
            wrapped = append_wrapped_sysex_body(body)
            if wrapped:
                out.extend(wrapped)
    return bytes(out)


def parse_lm8101_vc_from_buffer(data: bytes) -> tuple[bool, dict]:
    """OLD: parseLm8101VcMinimal(const void* data, size_t dataSize)."""
    block = find_lm_block(data, "8101VC")
    if block.data is None:
        return False, parse_lm8101_vc(b"")
    frame = bytes(block.data)
    parsed = parse_lm8101_vc(frame)
    return parsed.get("found8101", False), parsed


def parse_lm8101_vc_from_sysex_messages(messages: list[SysexMessage]) -> tuple[bool, dict, str]:
    """NEW: parseLm8101VcMinimalFromSysexMessages glue."""

    block = find_lm_block_in_sysex_messages(messages, "8101VC")
    if block.data is not None:
        frame = bytes(block.data)
        if len(frame) >= K_MIN_8101_VC_FRAME_SIZE and frame[0] == 0xF0 and frame[-1] == 0xF7:
            parsed = parse_lm8101_vc(frame)
            if parsed.get("found8101"):
                return True, parsed, "findLmBlockInSysexMessages"

    for m in messages:
        if not m.is_sysex():
            continue

        raw = m.get_raw_data()
        if raw and raw[0] == 0xF0:
            parsed = parse_lm8101_vc(raw)
            if parsed.get("found8101"):
                return True, parsed, "per-message raw F0"

        body = m.get_sys_ex_data()
        if not body:
            continue

        if body[0] == 0xF0:
            parsed = parse_lm8101_vc(body)
            if parsed.get("found8101"):
                return True, parsed, "per-message body F0"
            continue

        if body[0] != 0x43:
            continue

        wrapped = append_wrapped_sysex_body(body)
        if wrapped:
            parsed = parse_lm8101_vc(wrapped)
            if parsed.get("found8101"):
                return True, parsed, "per-message wrapped 0x43"

    concat = concat_live_read_sysex_raw(messages)
    if concat:
        block = find_lm_block(concat, "8101VC")
        if block.data is not None:
            frame = bytes(block.data)
            parsed = parse_lm8101_vc(frame)
            if parsed.get("found8101"):
                return True, parsed, "concatLiveReadSysexRaw"

    return False, parse_lm8101_vc(b""), "none"


def iter_sysex_frames(data: bytes) -> list[bytes]:
    frames: list[bytes] = []
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
        frames.append(data[i : j + 1])
        i = j + 1
    return frames


def messages_from_syx(data: bytes) -> list[SysexMessage]:
    return [SysexMessage(fr) for fr in iter_sysex_frames(data)]


@dataclass
class RawOnlyChunkMessage:
    """Driver chunk: only getRawData() populated (concat path)."""

    raw: bytes

    def is_sysex(self) -> bool:
        return len(self.raw) > 0

    def get_raw_data(self) -> bytes:
        return self.raw

    def get_sys_ex_data(self) -> bytes:
        return b""


def messages_chunked_8101(data: bytes, split_at: int) -> list:
    """Split 8101VC frame into two driver chunks (raw-only, no body fallback)."""
    frames = iter_sysex_frames(data)
    out = []
    for fr in frames:
        if b"8101VC" in fr and len(fr) > split_at:
            out.append(RawOnlyChunkMessage(fr[:split_at]))
            out.append(RawOnlyChunkMessage(fr[split_at:]))
        else:
            out.append(SysexMessage(fr))
    return out


@dataclass
class BodyOnlySysexMessage:
    """Simulate JUCE capture where getRawData() is empty but getSysExData() has 0x43 body."""

    body: bytes

    def is_sysex(self) -> bool:
        return len(self.body) > 0 and self.body[0] == 0x43

    def get_raw_data(self) -> bytes:
        return b""

    def get_sys_ex_data(self) -> bytes:
        return self.body


def messages_body_only_from_syx(data: bytes) -> list[BodyOnlySysexMessage]:
    return [BodyOnlySysexMessage(fr[1:-1]) for fr in iter_sysex_frames(data)]


def parse_new_generic(messages) -> tuple[bool, dict, str]:
    """Adapter: duck-typed messages (raw and body-only)."""
    return parse_lm8101_vc_from_sysex_messages(messages)


COMPARE_KEYS = (
    "found8101",
    "name",
    "elmodeRaw",
    "wolRaw",
    "elvlE1Raw",
    "outselE1Raw",
    "elvlRaw",
    "outselRaw",
    "eldtRaw",
    "elnsRaw",
    "evllRaw",
    "evlhRaw",
    "efln1ElRaw",
)


def format_parsed(p: dict) -> str:
    parts = []
    for k in COMPARE_KEYS:
        v = p.get(k)
        parts.append(f"{k}={v!r}")
    return " ".join(parts)


def compare_dicts(old: dict, new: dict) -> list[str]:
    diffs: list[str] = []
    for k in COMPARE_KEYS:
        ov = old.get(k)
        nv = new.get(k)
        if ov != nv:
            diffs.append(f"  {k}: old={ov!r} new={nv!r}")
    return diffs


def run_scenario(name: str, data: bytes, messages) -> bool:
    ok_old, old_p = parse_lm8101_vc_from_buffer(data)
    ok_new, new_p, via = parse_new_generic(messages)

    if ok_old != ok_new:
        print(f"  {name}: FAIL success old={ok_old} new={ok_new} (via={via})")
        return False

    diffs = compare_dicts(old_p, new_p)
    if diffs:
        print(f"  {name}: FAIL field mismatch (via={via})")
        for d in diffs:
            print(d)
        return False

    print(f"  {name}: MATCH (via={via})")
    return True


def main() -> int:
    syx_files = sorted(FIX.glob("*.syx"))
    if not syx_files:
        print("No .syx fixtures found")
        return 1

    all_ok = True
    print("=== parseLm8101VcMinimal: OLD (.syx buffer) vs NEW (LiveRead glue) ===\n")

    for path in syx_files:
        data = path.read_bytes()
        print(f"{path.name} ({len(data)} bytes)")

        msgs = messages_from_syx(data)
        if not run_scenario("whole-file messages (raw F0..F7 per frame)", data, msgs):
            all_ok = False

        body_msgs = messages_body_only_from_syx(data)
        if not run_scenario("body-only messages (wrapped 0x43 path)", data, body_msgs):
            all_ok = False

        if any(b"8101VC" in fr for fr in iter_sysex_frames(data)):
            for split_at in (200, 300, 400):
                chunked = messages_chunked_8101(data, split_at)
                if not run_scenario(f"chunked 8101 @ {split_at}", data, chunked):
                    all_ok = False

        print()

    if all_ok:
        print("ALL scenarios MATCH")
        return 0

    print("SOME scenarios FAILED")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

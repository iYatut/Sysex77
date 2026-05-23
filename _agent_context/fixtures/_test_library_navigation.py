#!/usr/bin/env python3
"""Regression gate for SY-99 library navigation (mirrors Sy99LibraryNavigation.h)."""

from __future__ import annotations

K_VOICE_SLOTS = 64
K_MULTI_SLOTS = 16
K_MULTI_PC_BASE = 64

# Sy99LibraryContentPage ids (VoicesTableModel.h)
PAGE_INTERNAL = 0
PAGE_PRE1 = 1
PAGE_PRE2 = 2
PAGE_MULTI_INT = 3

CC32_INTERNAL = 0
CC32_PRE1 = 2
CC32_PRE2 = 5
CC32_MULTI = 18


def bank_lsb_for_page(page: int) -> int:
    if page == PAGE_PRE1:
        return CC32_PRE1
    if page == PAGE_PRE2:
        return CC32_PRE2
    if page == PAGE_MULTI_INT:
        return CC32_MULTI
    return CC32_INTERNAL


def inbound_voice_mm_from_pc(
    page: int,
    pc: int,
    cc0_in_batch: int,
    committed_mm: int,
) -> int:
    if page == PAGE_MULTI_INT:
        if K_MULTI_PC_BASE <= pc < K_MULTI_PC_BASE + K_MULTI_SLOTS:
            return pc - K_MULTI_PC_BASE
        return pc % 16

    if 16 <= pc < K_VOICE_SLOTS:
        return pc

    slot = pc % 16

    if cc0_in_batch >= 0:
        return min(K_VOICE_SLOTS - 1, max(0, cc0_in_batch * 16 + slot))

    if committed_mm < 0:
        return slot

    prev_slot = committed_mm % 16

    if slot == (prev_slot + 1) & 15:
        return (committed_mm + 1) % K_VOICE_SLOTS

    if slot == ((prev_slot + 15) & 15):
        return (committed_mm + K_VOICE_SLOTS - 1) % K_VOICE_SLOTS

    return (committed_mm // 16) * 16 + slot


class RecallContext:
    def __init__(self) -> None:
        self.page = PAGE_INTERNAL
        self.mm = -1
        self.bank_msb = -1
        self.bank_lsb = -1
        self.multi_mode = False

    def clear(self) -> None:
        self.__init__()


def outbound_needs_full_triple(page: int, mm: int, ctx: RecallContext) -> bool:
    if page == PAGE_MULTI_INT:
        return not ctx.multi_mode or ctx.page != page

    if mm < 0 or mm > K_VOICE_SLOTS - 1:
        return True

    if ctx.mm < 0 or ctx.multi_mode:
        return True

    bank_msb = mm // 16
    bank_lsb = bank_lsb_for_page(page)

    if ctx.page != page:
        return True
    if ctx.bank_lsb != bank_lsb:
        return True
    if ctx.bank_msb != bank_msb:
        return True
    return False


def recall_context_after_send(page: int, mm: int, ctx: RecallContext) -> None:
    if page == PAGE_MULTI_INT:
        ctx.page = page
        ctx.mm = mm
        ctx.bank_lsb = bank_lsb_for_page(page)
        ctx.multi_mode = True
        ctx.bank_msb = -2
        return

    ctx.page = page
    ctx.mm = mm
    ctx.bank_msb = mm // 16
    ctx.bank_lsb = bank_lsb_for_page(page)
    ctx.multi_mode = False


def slot_code(mm: int) -> str:
    banks = "ABCD"
    return f"{banks[mm // 16]}{(mm % 16) + 1}"


def test_a16_forward_to_b1() -> None:
    mm = inbound_voice_mm_from_pc(PAGE_INTERNAL, pc=0, cc0_in_batch=-1, committed_mm=15)
    assert mm == 16, f"A16→forward: expected mm=16 (B1), got {mm} ({slot_code(mm)})"


def test_a1_back_to_d16() -> None:
    """Voice 1 -> back -> voice 64 (D16) on linear 1..64 ring."""
    mm = inbound_voice_mm_from_pc(PAGE_INTERNAL, pc=15, cc0_in_batch=-1, committed_mm=0)
    assert mm == 63, f"A1 back: expected mm=63 (D16 / voice 64), got {mm} ({slot_code(mm)})"


def test_d16_forward_to_a1() -> None:
    """Voice 64 -> forward -> voice 1 (A1)."""
    mm = inbound_voice_mm_from_pc(PAGE_INTERNAL, pc=0, cc0_in_batch=-1, committed_mm=63)
    assert mm == 0, f"D16 forward: expected mm=0 (A1), got {mm}"


def test_cc0_batch_no_wrap() -> None:
    mm = inbound_voice_mm_from_pc(PAGE_INTERNAL, pc=3, cc0_in_batch=3, committed_mm=15)
    assert mm == 51, f"CC0=3 PC=3: expected D4 mm=51, got {mm} ({slot_code(mm)})"


def test_pre1_after_internal_needs_full_triple() -> None:
    ctx = RecallContext()
    recall_context_after_send(PAGE_INTERNAL, mm=0, ctx=ctx)
    assert not outbound_needs_full_triple(PAGE_INTERNAL, mm=1, ctx=ctx)
    assert outbound_needs_full_triple(PAGE_PRE1, mm=51, ctx=ctx), (
        "Internal D1 ctx → PRE1 D4 must send CC32=2 (full triple)"
    )


def test_same_column_pc_only() -> None:
    ctx = RecallContext()
    recall_context_after_send(PAGE_INTERNAL, mm=50, ctx=ctx)  # D1
    assert not outbound_needs_full_triple(PAGE_INTERNAL, mm=51, ctx=ctx), (
        "D1→D4 same page/column: PC-only OK"
    )


def test_cross_column_full_triple() -> None:
    ctx = RecallContext()
    recall_context_after_send(PAGE_INTERNAL, mm=15, ctx=ctx)  # A16
    assert outbound_needs_full_triple(PAGE_INTERNAL, mm=16, ctx=ctx), (
        "A16→B1: mm/16 changed → full triple"
    )


def main() -> int:
    tests = [
        test_a16_forward_to_b1,
        test_a1_back_to_d16,
        test_d16_forward_to_a1,
        test_cc0_batch_no_wrap,
        test_pre1_after_internal_needs_full_triple,
        test_same_column_pc_only,
        test_cross_column_full_triple,
    ]
    failed = 0
    for t in tests:
        try:
            t()
            print(f"PASS  {t.__name__}")
        except AssertionError as e:
            failed += 1
            print(f"FAIL  {t.__name__}: {e}")
    if failed:
        print(f"\n{failed}/{len(tests)} failed")
        return 1
    print(f"\nAll {len(tests)} navigation tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

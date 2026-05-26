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
CC32_MULTI_INT = 16
CC32_MULTI_PRE = 18


def bank_lsb_for_page(page: int) -> int:
    if page == PAGE_PRE1:
        return CC32_PRE1
    if page == PAGE_PRE2:
        return CC32_PRE2
    if page == PAGE_MULTI_INT:
        return CC32_MULTI_INT
    return CC32_INTERNAL


def page_from_bank_lsb(cc32: int) -> int | None:
    if cc32 == CC32_INTERNAL:
        return PAGE_INTERNAL
    if cc32 == CC32_PRE1:
        return PAGE_PRE1
    if cc32 == CC32_PRE2:
        return PAGE_PRE2
    if cc32 == CC32_MULTI_INT:
        return PAGE_MULTI_INT
    if cc32 == CC32_MULTI_PRE:
        return PAGE_MULTI_INT  # preset multi page id differs in app; PC decode same
    return None


def inbound_voice_mm_from_pc(
    page: int,
    pc: int,
    cc0_in_batch: int,
    committed_mm: int,
    latched_bank_msb: int = 0,
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

    return min(K_VOICE_SLOTS - 1, max(0, latched_bank_msb * 16 + slot))


class RecallContext:
    def __init__(self) -> None:
        self.page = PAGE_INTERNAL
        self.mm = -1
        self.bank_msb = -1
        self.bank_lsb = -1
        self.multi_mode = False

    def clear(self) -> None:
        self.__init__()


class InboundCommittedState:
    """Mirrors LibraryInboundCommittedState (Sy99LibraryNavigation.h)."""

    def __init__(self) -> None:
        self.page = PAGE_INTERNAL
        self.mm = -1


def host_synth_nav_in_sync(
    page: int,
    committed: InboundCommittedState,
    inbound_cc32: int,
    ctx: RecallContext,
) -> bool:
    """Mirrors sy99HostSynthNavInSync — authoritative outbound PC-only policy."""
    expected_lsb = bank_lsb_for_page(page)

    if committed.mm >= 0:
        if committed.page != page:
            return False
        return bank_lsb_for_page(committed.page) == expected_lsb

    inbound_page = page_from_bank_lsb(inbound_cc32)
    if inbound_page is not None:
        if inbound_page != page:
            return False
        return inbound_cc32 == expected_lsb

    if inbound_cc32 == 0:
        return page == PAGE_INTERNAL

    if ctx.page == page and ctx.bank_lsb == expected_lsb:
        return True

    return False


def library_outbound_needs_full_triple(
    page: int,
    mm: int,
    committed: InboundCommittedState,
    inbound_cc32: int,
    ctx: RecallContext,
) -> bool:
    """Mirrors libraryOutboundNeedsFullTriple (C++ path, not legacy ctx-only)."""
    _ = mm
    return not host_synth_nav_in_sync(page, committed, inbound_cc32, ctx)


def recall_context_after_page_bank_select(page: int, ctx: RecallContext) -> None:
    """Mirrors libraryRecallContextAfterPageBankSelect."""
    ctx.page = page
    ctx.mm = -1
    ctx.bank_lsb = bank_lsb_for_page(page)
    ctx.multi_mode = page == PAGE_MULTI_INT
    ctx.bank_msb = -2 if ctx.multi_mode else -1


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
    return False


def outbound_program_for_slot(page: int, global_mm: int) -> int:
    if page == PAGE_MULTI_INT:
        return min(127, K_MULTI_PC_BASE + global_mm)
    if global_mm >= 16:
        return global_mm
    return min(15, max(0, global_mm))


def outbound_needs_cc0_for_recall(page: int, mm: int, ctx: RecallContext) -> bool:
    if page == PAGE_MULTI_INT or mm < 0 or mm >= 16:
        return False
    if ctx.mm < 0 or ctx.multi_mode or ctx.page != page:
        return True
    return (ctx.mm // 16) != 0


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


def test_cross_column_pc_only() -> None:
    ctx = RecallContext()
    recall_context_after_send(PAGE_INTERNAL, mm=15, ctx=ctx)  # A16
    assert not outbound_needs_full_triple(PAGE_INTERNAL, mm=16, ctx=ctx), (
        "A16→B1 same page: direct PC 15→16, no CC0/CC32"
    )


def test_outbound_pc_is_global_mm() -> None:
    assert outbound_program_for_slot(PAGE_INTERNAL, 27) == 27, "B12 → PC=27"
    assert outbound_program_for_slot(PAGE_INTERNAL, 34) == 34, "C3 → PC=34"
    assert outbound_program_for_slot(PAGE_INTERNAL, 11) == 11, "A12 → PC=11"


def test_outbound_cc0_for_bank_a() -> None:
    ctx = RecallContext()
    recall_context_after_send(PAGE_INTERNAL, mm=27, ctx=ctx)  # B12
    assert outbound_needs_cc0_for_recall(PAGE_INTERNAL, 11, ctx), "B→A needs CC0=0"
    recall_context_after_send(PAGE_INTERNAL, mm=11, ctx=ctx)  # A12
    assert not outbound_needs_cc0_for_recall(PAGE_INTERNAL, 12, ctx), "A12→A13 PC only"


def test_multi_inbound_pc_64_79() -> None:
    for pc in range(64, 80):
        mm = inbound_voice_mm_from_pc(PAGE_MULTI_INT, pc=pc, cc0_in_batch=-1, committed_mm=-1)
        assert mm == pc - K_MULTI_PC_BASE, f"PC={pc} → M{mm + 1}"


def test_multi_cc32_16_maps_page() -> None:
    assert page_from_bank_lsb(16) == PAGE_MULTI_INT
    assert page_from_bank_lsb(18) is not None
    assert page_from_bank_lsb(99) is None


def test_b_to_a_via_cc0_latch() -> None:
    """B12 → A12: PC=11 with latched CC0 MSB=0 (bank A)."""
    mm = inbound_voice_mm_from_pc(
        PAGE_INTERNAL, pc=11, cc0_in_batch=-1, committed_mm=27, latched_bank_msb=0
    )
    assert mm == 11, f"B→A: expected A12 mm=11, got {mm} ({slot_code(mm)})"


def test_b_to_a_via_cc0_batch() -> None:
    mm = inbound_voice_mm_from_pc(
        PAGE_INTERNAL, pc=11, cc0_in_batch=0, committed_mm=27, latched_bank_msb=1
    )
    assert mm == 11, f"CC0 batch=0 PC=11: expected A12 mm=11, got {mm}"


def test_b_same_bank_pc_only() -> None:
    """B12 → B13: dial +1 without CC0."""
    mm = inbound_voice_mm_from_pc(
        PAGE_INTERNAL, pc=12, cc0_in_batch=-1, committed_mm=27, latched_bank_msb=1
    )
    assert mm == 28, f"B12→B13: expected mm=28, got {mm} ({slot_code(mm)})"


def test_host_nav_in_sync_committed_internal() -> None:
    committed = InboundCommittedState()
    committed.page = PAGE_INTERNAL
    committed.mm = 15
    ctx = RecallContext()
    assert host_synth_nav_in_sync(PAGE_INTERNAL, committed, inbound_cc32=-1, ctx=ctx)
    assert not library_outbound_needs_full_triple(
        PAGE_INTERNAL, 16, committed, inbound_cc32=-1, ctx=ctx
    ), "A16→B1: committed internal → PC-only"


def test_host_nav_cross_page_needs_triple() -> None:
    committed = InboundCommittedState()
    committed.page = PAGE_INTERNAL
    committed.mm = 0
    ctx = RecallContext()
    assert not host_synth_nav_in_sync(PAGE_PRE1, committed, inbound_cc32=-1, ctx=ctx)
    assert library_outbound_needs_full_triple(
        PAGE_PRE1, 51, committed, inbound_cc32=-1, ctx=ctx
    ), "Internal ctx → PRE1 must full triple"


def test_host_nav_cc32_latch_internal() -> None:
    committed = InboundCommittedState()
    ctx = RecallContext()
    assert host_synth_nav_in_sync(PAGE_INTERNAL, committed, inbound_cc32=0, ctx=ctx)
    assert not host_synth_nav_in_sync(PAGE_INTERNAL, committed, inbound_cc32=2, ctx=ctx)


def test_host_nav_recall_ctx_fallback() -> None:
    committed = InboundCommittedState()
    ctx = RecallContext()
    recall_context_after_send(PAGE_INTERNAL, mm=27, ctx=ctx)
    assert host_synth_nav_in_sync(PAGE_INTERNAL, committed, inbound_cc32=99, ctx=ctx)


def test_host_nav_after_tab_bank_select_pc_only() -> None:
    """After tab CC0+CC32 without PC, next slot recall is PC-only when ctx bank matches."""
    committed = InboundCommittedState()
    ctx = RecallContext()
    recall_context_after_page_bank_select(PAGE_INTERNAL, ctx)
    assert host_synth_nav_in_sync(PAGE_INTERNAL, committed, inbound_cc32=99, ctx=ctx)
    assert not library_outbound_needs_full_triple(
        PAGE_INTERNAL, 5, committed, inbound_cc32=99, ctx=ctx
    )


def test_legacy_ctx_policy_differs_from_host_sync() -> None:
    """Document drift: legacy ctx-only check vs sy99HostSynthNavInSync."""
    ctx = RecallContext()
    recall_context_after_send(PAGE_INTERNAL, mm=15, ctx=ctx)
    committed = InboundCommittedState()
    assert not outbound_needs_full_triple(PAGE_INTERNAL, mm=16, ctx=ctx)
    committed.page = PAGE_PRE2
    committed.mm = 3
    assert library_outbound_needs_full_triple(
        PAGE_INTERNAL, 16, committed, inbound_cc32=-1, ctx=ctx
    ), "Inbound committed PRE2 blocks PC-only on Internal host page"


def main() -> int:
    tests = [
        test_a16_forward_to_b1,
        test_a1_back_to_d16,
        test_d16_forward_to_a1,
        test_cc0_batch_no_wrap,
        test_pre1_after_internal_needs_full_triple,
        test_same_column_pc_only,
        test_cross_column_pc_only,
        test_outbound_pc_is_global_mm,
        test_outbound_cc0_for_bank_a,
        test_multi_inbound_pc_64_79,
        test_multi_cc32_16_maps_page,
        test_b_to_a_via_cc0_latch,
        test_b_to_a_via_cc0_batch,
        test_b_same_bank_pc_only,
        test_host_nav_in_sync_committed_internal,
        test_host_nav_cross_page_needs_triple,
        test_host_nav_cc32_latch_internal,
        test_host_nav_recall_ctx_fallback,
        test_host_nav_after_tab_bank_select_pc_only,
        test_legacy_ctx_policy_differs_from_host_sync,
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

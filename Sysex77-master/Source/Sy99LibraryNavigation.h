/*
  SY-99 library navigation — canonical (page, mm 0..63).
  Include after Sy99LibraryContentPage, libraryPageIsMulti, kSy99* in VoicesTableModel.h.

  OUTBOUND RECALL POLICY (v1 — do not regress):
  - Strong step (CC0=0 + CC32 + PC) when host page/CC32 is not in sync with synth (inbound
    or last successful outbound). After tab switch: CC0+CC32 without PC, then PC-only in-sync.
  - PC-only only when sy99HostSynthNavInSync(page) — never skip CC32 on mismatch.
  - libraryRecallContextAfterSend only after successful MIDI TX (MidiDemo).

  NAV STATE QUERY (passive RX — no active "get current slot" SysEx):
  - Inbound CC0/CC32/PC on ch1 → libraryInboundCommittedState + inboundLibraryBankLsb
  - Unsolicited 8101VC/MU bulk → librarySynthPanelNavigateCallback
  - Edit buffer dump mt=7F is buffer contents, not navigation slot
  - Active dump 0040SY / bulk 0x7A — library sync only, not live nav poll
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Sy99Ui.h"

/** Last committed inbound selection (MIDI thread). */
struct LibraryInboundCommittedState
{
    Sy99LibraryContentPage page = Sy99LibraryContentPage::internalVoices;
    int mm = -1;
};

/** CC0/CC32 seen shortly before PC on the same MIDI thread tick. */
struct LibraryInboundNavCcBatch
{
    int cc0 = -1;
    int cc32 = -1;
    juce::uint32 lastAtMs = 0;

    static constexpr juce::uint32 kWindowMs = 50;

    bool cc0Fresh (juce::uint32 nowMs) const noexcept
    {
        return cc0 >= 0 && nowMs - lastAtMs <= kWindowMs;
    }
};

/** Outbound recall context — page + mm + wire encoding last established on synth. */
struct LibraryRecallContext
{
    Sy99LibraryContentPage page = Sy99LibraryContentPage::internalVoices;
    int mm = -1;
    int bankMsb = -1;
    int bankLsb = -1;
    bool multiMode = false;
};

inline LibraryInboundCommittedState& libraryInboundCommittedState() noexcept
{
    static LibraryInboundCommittedState s;
    return s;
}

inline LibraryInboundNavCcBatch& libraryInboundNavCcBatch() noexcept
{
    static LibraryInboundNavCcBatch b;
    return b;
}

inline LibraryRecallContext& libraryRecallContext() noexcept
{
    static LibraryRecallContext c;
    return c;
}

inline void resetLibraryRecallContext() noexcept
{
    libraryRecallContext() = {};
}

/** Legacy shim — MSB only; prefer libraryRecallContext(). */
inline int& libraryRecallContextBankMsb() noexcept
{
    return libraryRecallContext().bankMsb;
}

inline constexpr int kSy99LibraryRecallMultiContext = -2;

inline void libraryInboundNoteCc0 (int channel, int value) noexcept
{
    if (channel != 1)
        return;

    auto& batch = libraryInboundNavCcBatch();
    batch.cc0 = value;
    batch.lastAtMs = juce::Time::getMillisecondCounter();
}

inline void libraryInboundNoteCc32 (int channel, int value) noexcept
{
    if (channel != 1)
        return;

    auto& batch = libraryInboundNavCcBatch();
    batch.cc32 = value;
    batch.lastAtMs = juce::Time::getMillisecondCounter();
}

/** Decode voice page mm (0..63) from PC + optional batch CC0 + committed mm. */
inline int sy99InboundVoiceMmFromPc (Sy99LibraryContentPage page,
                                     int pc,
                                     int cc0InBatch,
                                     const LibraryInboundCommittedState& committed) noexcept
{
    if (libraryPageIsMulti (page))
    {
        if (pc >= kSy99MultiPcBase
            && pc < kSy99MultiPcBase + kSy99LibraryMultiSlotCount)
            return pc - kSy99MultiPcBase;

        return juce::jlimit (0, kSy99LibraryMultiSlotCount - 1, pc % 16);
    }

    if (pc >= 16 && pc < kSy99LibraryVoiceSlotCount)
        return pc;

    const int slot = pc % 16;

    if (cc0InBatch >= 0)
        return juce::jlimit (0, kSy99LibraryVoiceSlotCount - 1, cc0InBatch * 16 + slot);

    const int committedMm = committed.mm;

    if (committedMm < 0)
        return slot;

    const int prevSlot = committedMm % 16;

    if (slot == ((prevSlot + 1) & 15))
        return (committedMm + 1) % kSy99LibraryVoiceSlotCount;

    if (slot == ((prevSlot + 15) & 15))
        return (committedMm + kSy99LibraryVoiceSlotCount - 1) % kSy99LibraryVoiceSlotCount;

    // PC 0..15 = slot within bank; CC0 MSB latched on ch1 (SY99 may gap CC0 before PC > 8 ms).
    return juce::jlimit (0, kSy99LibraryVoiceSlotCount - 1,
                         inboundLibraryBankMsb() * 16 + slot);
}

/** True when inbound CC32 indicates synth is on Multi while host UI is on VOICE/PRE. */
inline bool sy99InboundSynthInMultiModeWhileHostOnVoice() noexcept
{
    const auto hostPage = libraryContentPage();

    if (libraryPageIsMulti (hostPage))
        return false;

    const auto& committed = libraryInboundCommittedState();

    if (committed.mm >= 0 && libraryPageIsMulti (committed.page))
        return true;

    const int cc32 = inboundLibraryBankLsb();
    return cc32 == kSy99Cc32MultiInternal || cc32 == kSy99Cc32MultiPreset;
}

/** Suffix for labelInfoLine when synth Multi mode blocks voice recall. */
inline juce::String sy99LibraryNavMismatchHintSuffix() noexcept
{
    if (! sy99InboundSynthInMultiModeWhileHostOnVoice())
        return {};

    return Sy99Ui::navMultiModeHintSuffix();
}

/** Host Librairie page matches synth bank (inbound CC32/page or last successful outbound). */
inline bool sy99HostSynthNavInSync (Sy99LibraryContentPage page) noexcept
{
    const int expectedLsb = sy99BankLsbForLibraryContentPage (page);
    const auto& committed = libraryInboundCommittedState();

    if (committed.mm >= 0)
    {
        if (committed.page != page)
            return false;

        return sy99BankLsbForLibraryContentPage (committed.page) == expectedLsb;
    }

    const int cc32 = inboundLibraryBankLsb();
    Sy99LibraryContentPage inboundPage;

    if (sy99LibraryContentPageFromBankLsb (cc32, inboundPage))
    {
        if (inboundPage != page)
            return false;

        return cc32 == expectedLsb;
    }

    if (cc32 == 0)
        return page == Sy99LibraryContentPage::internalVoices;

    const auto& ctx = libraryRecallContext();

    if (ctx.page == page && ctx.bankLsb == expectedLsb)
        return true;

    return false;
}

/** Strong CC0+CC32+PC when page/CC32 not in sync; PC-only when in sync (panel-like). */
inline bool libraryOutboundNeedsFullTriple (Sy99LibraryContentPage page, int mm) noexcept
{
    juce::ignoreUnused (mm);
    return ! sy99HostSynthNavInSync (page);
}

inline void libraryRecallContextAfterSend (Sy99LibraryContentPage page, int mm) noexcept
{
    auto& ctx = libraryRecallContext();

    if (libraryPageIsMulti (page))
    {
        ctx.page = page;
        ctx.mm = mm;
        ctx.bankLsb = sy99BankLsbForLibraryContentPage (page);
        ctx.multiMode = true;
        ctx.bankMsb = kSy99LibraryRecallMultiContext;
        return;
    }

    ctx.page = page;
    ctx.mm = mm;
    ctx.bankMsb = mm / 16;
    ctx.bankLsb = sy99BankLsbForLibraryContentPage (page);
    ctx.multiMode = false;
}

/** After tab switch: CC0+CC32 sent without PC — bank established, mm unknown. */
inline void libraryRecallContextAfterPageBankSelect (Sy99LibraryContentPage page) noexcept
{
    auto& ctx = libraryRecallContext();
    ctx.page = page;
    ctx.mm = -1;
    ctx.bankLsb = sy99BankLsbForLibraryContentPage (page);
    ctx.multiMode = libraryPageIsMulti (page);

    if (ctx.multiMode)
        ctx.bankMsb = kSy99LibraryRecallMultiContext;
    else
        ctx.bankMsb = -1;
}

/** CC0 with CC32: always 0 (panel). PC-only within bank A: CC0 when leaving B/C/D. */
inline bool sy99OutboundNeedsCc0ForRecall (Sy99LibraryContentPage page,
                                           int mm,
                                           bool sendingCc32) noexcept
{
    if (sendingCc32)
        return true;

    if (libraryPageIsMulti (page) || mm < 0 || mm >= 16)
        return false;

    const auto& ctx = libraryRecallContext();

    if (ctx.mm < 0 || ctx.multiMode || ctx.page != page)
        return true;

    return (ctx.mm / 16) != 0;
}

inline void libraryNavAuditLog (const char* direction,
                                Sy99LibraryContentPage page,
                                int cc0,
                                int cc32,
                                int pc,
                                int prevMm,
                                int mm,
                                bool fullTriple,
                                bool suppressed) noexcept
{
    juce::Logger::writeToLog ("[NAV audit] " + juce::String (direction)
                              + " page=" + juce::String ((int) page)
                              + " CC0=" + juce::String (cc0)
                              + " CC32=" + juce::String (cc32)
                              + " PC=" + juce::String (pc)
                              + " prevMm=" + juce::String (prevMm)
                              + " mm=" + juce::String (mm)
                              + " fullTriple=" + juce::String (fullTriple ? 1 : 0)
                              + " suppressed=" + juce::String (suppressed ? 1 : 0));
}

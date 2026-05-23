/*
  SY-99 library navigation — canonical (page, mm 0..63).
  Include after Sy99LibraryContentPage, libraryPageIsMulti, kSy99* in VoicesTableModel.h.
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

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

    static constexpr juce::uint32 kWindowMs = 8;

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

    return (committedMm / 16) * 16 + slot;
}

inline bool libraryOutboundNeedsFullTriple (Sy99LibraryContentPage page, int mm) noexcept
{
    if (libraryPageIsMulti (page))
    {
        const auto& ctx = libraryRecallContext();
        return ! ctx.multiMode || ctx.page != page;
    }

    if (mm < 0 || mm > kSy99LibraryVoiceSlotCount - 1)
        return true;

    const auto& ctx = libraryRecallContext();

    if (ctx.mm < 0 || ctx.multiMode)
        return true;

    const int bankMsb = mm / 16;
    const int bankLsb = sy99BankLsbForLibraryContentPage (page);

    if (ctx.page != page)
        return true;

    if (ctx.bankLsb != bankLsb)
        return true;

    if (ctx.bankMsb != bankMsb)
        return true;

    return false;
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

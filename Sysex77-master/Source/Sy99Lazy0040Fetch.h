/*
  Lazy 0040VC fetch on BankClick (dump request to SY99). Invoke bulk TX is manual (Librairie menu #14/#15).
*/

#pragma once

#include "LiveSynthState.h"
#include "Sy99DumpRequest.h"
#include "YamahaLmVoiceDump.h"
#include "../JuceLibraryCode/JuceHeader.h"

extern bool requestSysex;

/** Wired from Librairie (avoids include cycle with Sy99BulkLibraryCapture). */
inline std::function<bool()>& sy99AnyLibrarySyncActiveCallback() noexcept
{
    static std::function<bool()> cb;
    return cb;
}

/** Wired from Librairie — mutex with BankClick HW refresh (avoids Lazy↔Invoke include cycle). */
inline std::function<bool()>& sy99InvokeBulkLoadActiveCallback() noexcept
{
    static std::function<bool()> cb;
    return cb;
}

enum class Sy99BankClickHwRefreshKind : uint8
{
    none = 0,
    slot0040,
    editBuffer
};

inline Sy99BankClickHwRefreshKind& sy99BankClickHwRefreshKind() noexcept
{
    static Sy99BankClickHwRefreshKind kind = Sy99BankClickHwRefreshKind::none;
    return kind;
}

inline bool sy99InvokeBulkLoadIsActiveForBankClick() noexcept
{
    if (auto& fn = sy99InvokeBulkLoadActiveCallback(); fn != nullptr)
        return fn();

    return false;
}

inline bool sy99IsLibrarySyncActiveForBankClick() noexcept
{
    if (auto& fn = sy99AnyLibrarySyncActiveCallback(); fn != nullptr)
        return fn();

    return false;
}

/** True while Librairie ingests a BankClick 0040 response (skip saving .syx). */
inline bool& sy99BankClick0040CaptureActive() noexcept
{
    static bool active = false;
    return active;
}

inline std::function<void()>& sy99BankClick0040ReapplyCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

inline std::function<void (int globalMm, uint8 memoryType)>& sy99BankClick0040FetchCallback() noexcept
{
    static std::function<void (int, uint8)> cb;
    return cb;
}

/** UI thread: finish bankclick capture as soon as 0040VC bulk arrives (do not wait for timer). */
inline std::function<void()>& sy99BankClick0040CompleteCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

inline std::function<void()>& sy99BankClick0040AbortCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

inline void sy99AbortActiveBankClick0040CaptureIfAny() noexcept
{
    if (! sy99BankClick0040CaptureActive())
        return;

    juce::Logger::writeToLog ("[BankClick] fetch0040 aborted: slot reopen");

    if (auto& abort = sy99BankClick0040AbortCallback(); abort != nullptr)
        juce::MessageManager::callAsync (abort);
}

inline int& sy99BankClick0040FetchGeneration() noexcept
{
    static int generation = 0;
    return generation;
}

/** Defer edit-buffer / slot 0040 TX until SY99 finishes PC recall (Sprint 5.1 regression fix). */
struct Sy99PendingHwRefreshAfterRecall
{
    bool valid = false;
    int mm = -1;
    uint8 memoryType = 0;
    int generation = -1;
};

inline Sy99PendingHwRefreshAfterRecall& sy99PendingHwRefreshAfterRecall() noexcept
{
    static Sy99PendingHwRefreshAfterRecall pending;
    return pending;
}

inline void sy99ClearPendingHwRefreshAfterRecall() noexcept
{
    sy99PendingHwRefreshAfterRecall() = {};
}

inline void sy99ScheduleBankClickHwRefresh (int globalMm, uint8 memoryType, bool parsed) noexcept;

inline void sy99FlushPendingHwRefreshAfterRecall() noexcept
{
    auto& p = sy99PendingHwRefreshAfterRecall();

    if (! p.valid)
        return;

    if (p.generation >= 0 && p.generation != sy99BankClick0040FetchGeneration())
    {
        p.valid = false;
        return;
    }

    const int mm = p.mm;
    const uint8 mt = p.memoryType;
    p.valid = false;

    juce::Logger::writeToLog ("[BankClick] hwRefresh after recall mm="
                              + juce::String::toHexString (mm).paddedLeft ('0', 2));
    sy99ScheduleBankClickHwRefresh (mm, mt, true);
}

inline void sy99ArmBankClickHwRefreshAfterRecall (int globalMm, uint8 memoryType) noexcept
{
    auto& p = sy99PendingHwRefreshAfterRecall();
    p.valid = true;
    p.mm = globalMm;
    p.memoryType = memoryType;
    p.generation = sy99BankClick0040FetchGeneration();
}

/** Start echo/timer wait — only after PC was actually TX'd (not deferred to MIDI open). */
inline void sy99BeginBankClickHwRefreshAfterRecallWait() noexcept
{
    auto& p = sy99PendingHwRefreshAfterRecall();

    if (! p.valid)
        return;

    juce::Logger::writeToLog ("[BankClick] hwRefresh wait recall mm="
                              + juce::String::toHexString (p.mm).paddedLeft ('0', 2));

    juce::Timer::callAfterDelay (400, []
    {
        auto& pending = sy99PendingHwRefreshAfterRecall();

        if (! pending.valid)
            return;

        if (pending.generation != sy99BankClick0040FetchGeneration())
        {
            pending.valid = false;
            return;
        }

        sy99FlushPendingHwRefreshAfterRecall();
    });
}

inline void sy99ScheduleBankClickHwRefreshAfterRecall (int globalMm, uint8 memoryType) noexcept
{
    sy99ArmBankClickHwRefreshAfterRecall (globalMm, memoryType);
    sy99BeginBankClickHwRefreshAfterRecallWait();
}

inline juce::Array<juce::MidiMessage>& sy99BankClick0040RxBuffer() noexcept
{
    static juce::Array<juce::MidiMessage> buffer;
    return buffer;
}

inline int& sy99BankClick0040FetchTargetMm() noexcept
{
    static int mm = -1;
    return mm;
}

inline bool sy99BankClick0040RxHas0040 (const juce::Array<juce::MidiMessage>& messages) noexcept
{
    const auto block = YamahaLmVoiceDump::findBestLm0040VcBlockInSysexMessages (messages);
    YamahaLmVoiceDump::Lm0040VcMinimal p0040;

    return block.data != nullptr
           && YamahaLmVoiceDump::parseLm0040VcMinimal (block.data, block.size, p0040)
           && p0040.found0040;
}

inline bool sy99BankClick8101MessageMatchesTargetMm (const juce::MidiMessage& message,
                                                       int targetMm) noexcept
{
    if (targetMm < 0 || ! message.isSysEx())
        return false;

    const uint8* d = message.getSysExData();
    const int n = message.getSysExDataSize();

    if (d == nullptr || n < 64 || d[0] != 0x43 || d[1] == 0x20)
        return false;

    if (! YamahaLmVoiceDump::frameContainsLmTag (d, n, "8101VC"))
        return false;

    for (int i = 0; i <= n - 26; ++i)
    {
        if (d[i] != 'L' || d[i + 1] != 'M')
            continue;

        if (! YamahaLmVoiceDump::tagsEqual6 (d + i + YamahaLmVoiceDump::kLmTagOffsetFromLm, "8101VC"))
            continue;

        const int addr = i + 24;

        if (addr + 1 >= n)
            return false;

        return (int) d[addr + 1] == targetMm;
    }

    return false;
}

/** MIDI thread: accumulate bankclick dump RX; async-complete when 0040VC seen. */
inline void sy99BankClick0040NoteIncoming (const juce::MidiMessage& message) noexcept
{
    if (! sy99BankClick0040CaptureActive() || ! message.isSysEx())
        return;

    sy99BankClick0040RxBuffer().add (message);

    extern int timeOut;
    timeOut = 0;

    const int targetMm = sy99BankClick0040FetchTargetMm();

    if (sy99BankClick8101MessageMatchesTargetMm (message, targetMm))
    {
        auto& s = getLiveSynthState();

        if (s.ingestLm8101FromSysexMessages (sy99BankClick0040RxBuffer()))
        {
            juce::Logger::writeToLog ("[BankClick] fetch0040 live 8101 mm="
                                      + juce::String::toHexString (targetMm).paddedLeft ('0', 2)
                                      + " WOL=" + juce::String (s.lmWolRaw));
        }
    }

    if (! sy99BankClick0040RxHas0040 (sy99BankClick0040RxBuffer()))
        return;

    juce::Logger::writeToLog ("[BankClick] fetch0040 RX 0040VC ready msgs="
                              + juce::String (sy99BankClick0040RxBuffer().size()));

    for (const auto& msg : sy99BankClick0040RxBuffer())
    {
        if (! msg.isSysEx())
            continue;

        const int n = msg.getRawDataSize();

        if (n >= YamahaLmVoiceDump::kMin0040VcFrameSize
            && YamahaLmVoiceDump::frameContainsLmTag (msg.getRawData(), n, "0040VC"))
        {
            juce::Logger::writeToLog ("[BankClick] fetch0040 RX frame bytes=" + juce::String (n));
            break;
        }
    }

    if (auto& done = sy99BankClick0040CompleteCallback(); done != nullptr)
        juce::MessageManager::callAsync (done);
}

struct Sy99PendingBankClick0040
{
    bool valid = false;
    int mm = -1;
    uint8 memoryType = 0;
};

inline Sy99PendingBankClick0040& sy99PendingBankClick0040() noexcept
{
    static Sy99PendingBankClick0040 pending;
    return pending;
}

enum class Sy99BankClickRndpFetchPhase : uint8
{
    none = 0,
    slot0040,
    editBuffer
};

inline Sy99BankClickRndpFetchPhase& sy99BankClickRndpFetchPhase() noexcept
{
    static Sy99BankClickRndpFetchPhase phase = Sy99BankClickRndpFetchPhase::none;
    return phase;
}

inline std::function<void()>& sy99BankClickEditBufferRndpFetchCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

inline bool sy99RndpRawValidForUi (int raw) noexcept
{
    return raw >= 0 && raw <= 7;
}

/** Ingest live param9 frames already present in a BankClick capture buffer. */
inline void sy99IngestParam9FramesFromMessages (LiveSynthState& s,
                                                const juce::Array<juce::MidiMessage>& messages) noexcept
{
    for (const auto& msg : messages)
    {
        if (! msg.isSysEx())
            continue;

        const uint8* d = msg.getSysExData();
        const int n = msg.getSysExDataSize();

        if (d == nullptr || n < 9 || d[0] != 0x43 || d[2] != 0x34)
            continue;

        s.ingestParameterFrame (d[3], d[4], d[5], d[6], d[7], d[8]);
    }
}

/** Edit-buffer 0040VC: authoritative RNDP for SY99 LCD (slot dump may differ). */
inline bool sy99ApplyEditBufferRndpFromMessages (LiveSynthState& s,
                                                 const juce::Array<juce::MidiMessage>& messages) noexcept
{
    YamahaLmVoiceDump::Lm0040VcMinimal parsed;

    if (! YamahaLmVoiceDump::parseLm0040VcMinimalFromSysexMessages (messages, parsed)
        || ! parsed.found0040
        || ! sy99RndpRawValidForUi (parsed.rndpRaw))
        return false;

    s.liveRndpRaw = parsed.rndpRaw;
    ++s.parameterFrameCount;
    return true;
}

inline int& sy99BankClick0040InFlightGeneration() noexcept
{
    static int generation = -1;
    return generation;
}

inline void sy99ClearPendingBankClick0040() noexcept
{
    sy99PendingBankClick0040() = {};
}

/** MIDI thread: synth changed slot — cancel deferred/active BankClick 0040 fetch. */
inline void sy99OnInboundSynthNavigation (int mm) noexcept
{
    sy99ClearPendingHwRefreshAfterRecall();
    sy99ClearPendingBankClick0040();
    ++sy99BankClick0040FetchGeneration();
    sy99BankClick0040InFlightGeneration() = -1;

    if (! sy99BankClick0040CaptureActive())
        return;

    juce::Logger::writeToLog ("[BankClick] fetch0040 aborted: synth navigation mm="
                              + juce::String::toHexString (mm).paddedLeft ('0', 2));

    if (auto& abort = sy99BankClick0040AbortCallback(); abort != nullptr)
        juce::MessageManager::callAsync (abort);
}

inline void sy99DeferBankClick0040Fetch (int globalMm, uint8 memoryType) noexcept
{
    auto& p = sy99PendingBankClick0040();
    p.valid = true;
    p.mm = globalMm;
    p.memoryType = memoryType;
    juce::Logger::writeToLog ("[BankClick] fetch0040 deferred until sync idle mm="
                              + juce::String::toHexString (globalMm).paddedLeft ('0', 2));
}

inline void sy99MaybeRequestBankClick0040Fetch (int globalMm,
                                                 uint8 memoryType = 0x00,
                                                 bool forceLiveRefresh = false) noexcept;
inline void sy99ScheduleBankClickHwRefresh (int globalMm, uint8 memoryType, bool parsed) noexcept;

/** Retry deferred fetch after library sync ends (one slot). */
inline void sy99DrainPendingBankClick0040Fetch() noexcept
{
    auto& p = sy99PendingBankClick0040();

    if (! p.valid)
        return;

    const int mm = p.mm;
    const uint8 mt = p.memoryType;
    p.valid = false;
    juce::Logger::writeToLog ("[BankClick] fetch0040 drain deferred mm="
                              + juce::String::toHexString (mm).paddedLeft ('0', 2));
    sy99ScheduleBankClickHwRefresh (mm, mt, true);
}

/** True when elmode 4/8 needs 0040 before effect/common-tail editor apply. */
inline bool sy99ShouldDeferBankClickEditorApply() noexcept
{
    const auto& s = getLiveSynthState();

    return s.hasParsedBulk8101
           && ! s.hasParsedBulk0040
           && YamahaLmVoiceDump::efsendBulk0040TrustedForElmodeRaw (s.lmElmodeRaw);
}

/** BankClick may drive editor (8101 mixer at minimum). */
inline bool sy99BankClickApplyReady() noexcept
{
    return getLiveSynthState().hasParsedBulk8101;
}

/** Skip Effect Send + 0040-common sliders until lazy 0040 ingest completes. */
inline bool sy99BankClickSkip0040DependentBlocks() noexcept
{
    return sy99ShouldDeferBankClickEditorApply();
}

inline void sy99MaybeRequestBankClickEditBufferRndpFetch() noexcept;

/** Ingest 0040 from capture buffer; keep existing 8101 in LiveSynthState. */
inline bool finishBankClick0040IngestFromMessages (const juce::Array<juce::MidiMessage>& messages) noexcept
{
    auto& s = getLiveSynthState();

    if (! s.hasParsedBulk8101)
    {
        juce::Logger::writeToLog ("[BankClick] fetch0040 skipped: no 8101 ingest");
        return false;
    }

    const int inFlightGen = sy99BankClick0040InFlightGeneration();

    if (inFlightGen >= 0 && inFlightGen != sy99BankClick0040FetchGeneration())
    {
        juce::Logger::writeToLog ("[BankClick] fetch0040 stale generation — drop reapply gen="
                                  + juce::String (inFlightGen)
                                  + " current="
                                  + juce::String (sy99BankClick0040FetchGeneration()));
        sy99BankClick0040InFlightGeneration() = -1;
        return false;
    }

    s.ingestLm8101FromSysexMessages (messages);

    if (YamahaLmVoiceDump::efsendBulk0040TrustedForElmodeRaw (s.lmElmodeRaw))
    {
        s.elementEfsendlvlRaw[0] = -1;
        s.elementEfsendlvlRaw[1] = -1;
    }

    s.clearBulk0040ParsedState();

    const bool ingested = s.ingestLm0040FromSysexMessages (messages);
    sy99IngestParam9FramesFromMessages (s, messages);

    if (s.lmEfln1ElRaw >= 0)
        s.elementEfsendselRaw[0] = -1;

    juce::String log = "[BankClick] fetch0040 elmode=" + juce::String (s.lmElmodeRaw)
                       + " has0040=" + juce::String (s.hasParsedBulk0040 ? 1 : 0);

    if (s.lm0040EfsdlvE1Raw >= 0)
        log += " EFSDLV_E1=" + juce::String (s.lm0040EfsdlvE1Raw);

    if (s.lm0040EfsdlvE2Raw >= 0)
        log += " EFSDLV_E2=" + juce::String (s.lm0040EfsdlvE2Raw);

    if (s.liveRndpRaw >= 0)
        log += " RNDP_live=" + juce::String (s.liveRndpRaw);
    else if (s.lm0040RndpRaw >= 0)
        log += " RNDP_bulk=" + juce::String (s.lm0040RndpRaw);

    if (s.lm0040WpbrRaw >= 0)
        log += " WPBR=" + juce::String (s.lm0040WpbrRaw);

    if (s.lm0040EfmodeRaw >= 0)
        log += " EFMODE_bulk=" + juce::String (s.lm0040EfmodeRaw);

    if (s.lmEfln1ElRaw >= 0)
        log += " EFLN_E1_bulk=" + juce::String (s.lmEfln1ElRaw);

    if (s.lm0040EflnE1Raw >= 0)
        log += " EFLN_E1_0040=" + juce::String (s.lm0040EflnE1Raw);

    if (s.lm0040EflnE2Raw >= 0)
        log += " EFLN_E2_0040=" + juce::String (s.lm0040EflnE2Raw);

    if (s.elementEfsendselRaw[1] >= 0)
        log += " EFLN_E2_live=" + juce::String (s.elementEfsendselRaw[1]);

    juce::ignoreUnused (ingested);
    juce::Logger::writeToLog (log + " ingested=" + juce::String (ingested ? 1 : 0));

    sy99BankClick0040InFlightGeneration() = -1;

    if (! ingested || ! s.hasParsedBulk0040)
    {
        juce::Logger::writeToLog ("[BankClick] fetch0040 ingest failed — companion EFSDLV cleared, "
                                  "need full 0040VC RX (>= "
                                  + juce::String (YamahaLmVoiceDump::kMin0040VcFrameSize)
                                  + " B)");
        return false;
    }

    if (s.liveRndpRaw < 0 && sy99BankClickRndpFetchPhase() == Sy99BankClickRndpFetchPhase::slot0040)
        sy99MaybeRequestBankClickEditBufferRndpFetch();

    if (auto& reapply = sy99BankClick0040ReapplyCallback(); reapply != nullptr)
    {
        juce::MessageManager::callAsync ([reapply]
        {
            reapply();
        });
    }

    return true;
}

inline void sy99MaybeRequestBankClickEditBufferRndpFetch() noexcept
{
    if (getLiveSynthState().liveRndpRaw >= 0)
        return;

    if (requestSysex || sy99IsLibrarySyncActiveForBankClick())
        return;

    if (auto& portStatus = sy99MidiPortStatusCallback(); portStatus != nullptr)
    {
        if (portStatus().isNotEmpty())
            return;
    }

    if (sy99BankClick0040CaptureActive())
        return;

    if (auto& fetch = sy99BankClickEditBufferRndpFetchCallback(); fetch != nullptr)
    {
        sy99BankClickRndpFetchPhase() = Sy99BankClickRndpFetchPhase::editBuffer;
        juce::Logger::writeToLog ("[BankClick] fetch0040 edit-buffer RNDP request");
        fetch();
    }
}

/** Edit-buffer 0040 ingest: live RNDP only, then reapply editor. */
inline bool finishBankClickEditBufferRndpFromMessages (const juce::Array<juce::MidiMessage>& messages) noexcept
{
    auto& s = getLiveSynthState();
    sy99BankClickRndpFetchPhase() = Sy99BankClickRndpFetchPhase::none;

    if (! sy99ApplyEditBufferRndpFromMessages (s, messages))
    {
        juce::Logger::writeToLog ("[BankClick] fetch0040 edit-buffer RNDP: no valid 0040 RNDP");
        return false;
    }

    juce::Logger::writeToLog ("[BankClick] fetch0040 edit-buffer RNDP_live="
                              + juce::String (s.liveRndpRaw));

    if (auto& reapply = sy99BankClick0040ReapplyCallback(); reapply != nullptr)
    {
        juce::MessageManager::callAsync ([reapply]
        {
            reapply();
        });
    }

    return true;
}

/** Router: one live TX per BankClick — SYM7 slot 0040VC (mm=N), not edit-buffer 7F/00. */
inline void sy99ScheduleBankClickHwRefresh (int globalMm, uint8 memoryType, bool parsed) noexcept
{
    sy99BankClickHwRefreshKind() = Sy99BankClickHwRefreshKind::none;

    if (! parsed)
        return;

    static int lastHwRefreshMm = -1;
    static juce::uint32 lastHwRefreshMs = 0;
    const juce::uint32 nowMs = juce::Time::getMillisecondCounter();

    if (globalMm == lastHwRefreshMm && nowMs - lastHwRefreshMs < 600)
    {
        juce::Logger::writeToLog ("[BankClick] hwRefresh debounced mm="
                                  + juce::String::toHexString (globalMm).paddedLeft ('0', 2));
        return;
    }

    lastHwRefreshMm = globalMm;
    lastHwRefreshMs = nowMs;

    const auto& s = getLiveSynthState();

    if (! s.hasParsedBulk8101 || s.lmElmodeRaw < 0)
        return;

    if (sy99InvokeBulkLoadIsActiveForBankClick())
    {
        juce::Logger::writeToLog ("[BankClick] hwRefresh skipped: invoke active");
        return;
    }

    if (sy99IsLibrarySyncActiveForBankClick())
    {
        juce::Logger::writeToLog ("[BankClick] hwRefresh skipped: library sync active");
        return;
    }

    if (auto& portStatus = sy99MidiPortStatusCallback(); portStatus != nullptr)
    {
        const juce::String err = portStatus();

        if (err.isNotEmpty())
        {
            juce::Logger::writeToLog ("[BankClick] hwRefresh deferred: " + err);
            sy99DeferBankClick0040Fetch (globalMm, memoryType);
            return;
        }
    }

    if (requestSysex || sy99IsLibrarySyncActiveForBankClick())
    {
        sy99DeferBankClick0040Fetch (globalMm, memoryType);
        return;
    }

    if (sy99BankClick0040CaptureActive())
    {
        juce::Logger::writeToLog ("[BankClick] hwRefresh skipped: capture already active");
        return;
    }

    const bool forceLiveRefresh = s.hasParsedBulk0040;
    sy99BankClickHwRefreshKind() = Sy99BankClickHwRefreshKind::slot0040;
    sy99BankClick0040FetchTargetMm() = globalMm;
    sy99BankClick0040InFlightGeneration() = sy99BankClick0040FetchGeneration();
    sy99BankClickRndpFetchPhase() = Sy99BankClickRndpFetchPhase::slot0040;
    juce::Logger::writeToLog ("[BankClick] hwRefresh slot mm="
                              + juce::String::toHexString (globalMm).paddedLeft ('0', 2)
                              + " mt="
                              + juce::String::toHexString ((int) memoryType).paddedLeft ('0', 2)
                              + " live="
                              + juce::String (forceLiveRefresh ? 1 : 0));
    sy99MaybeRequestBankClick0040Fetch (globalMm, memoryType, forceLiveRefresh);
}

/** Request lazy 0040 when library slot has 8101 but no paired 0040 in .syx. */
inline void sy99MaybeRequestBankClick0040Fetch (int globalMm,
                                                 uint8 memoryType,
                                                 bool forceLiveRefresh) noexcept
{
    const auto& s = getLiveSynthState();

    if (! s.hasParsedBulk8101 || s.lmElmodeRaw < 0)
    {
        juce::Logger::writeToLog ("[BankClick] fetch0040 skipped: has8101="
                                  + juce::String (s.hasParsedBulk8101 ? 1 : 0)
                                  + " elmode=" + juce::String (s.lmElmodeRaw));
        return;
    }

    if (s.hasParsedBulk0040 && ! forceLiveRefresh)
        return;

    if (sy99InvokeBulkLoadIsActiveForBankClick())
    {
        juce::Logger::writeToLog ("[BankClick] fetch0040 skipped: invoke active");
        return;
    }

    if (auto& portStatus = sy99MidiPortStatusCallback(); portStatus != nullptr)
    {
        const juce::String err = portStatus();

        if (err.isNotEmpty())
        {
            juce::Logger::writeToLog ("[BankClick] fetch0040 blocked: " + err);
            sy99DeferBankClick0040Fetch (globalMm, memoryType);
            return;
        }
    }

    if (requestSysex || sy99IsLibrarySyncActiveForBankClick())
    {
        sy99DeferBankClick0040Fetch (globalMm, memoryType);
        return;
    }

    if (sy99BankClick0040CaptureActive())
    {
        juce::Logger::writeToLog ("[BankClick] fetch0040 skipped: capture already active");
        return;
    }

    if (auto& fetch = sy99BankClick0040FetchCallback(); fetch != nullptr)
    {
        sy99BankClick0040FetchTargetMm() = globalMm;
        sy99BankClick0040InFlightGeneration() = sy99BankClick0040FetchGeneration();
        sy99BankClickRndpFetchPhase() = Sy99BankClickRndpFetchPhase::slot0040;
        juce::Logger::writeToLog ("[BankClick] fetch0040 request mm="
                                  + juce::String::toHexString (globalMm).paddedLeft ('0', 2)
                                  + " mt="
                                  + juce::String::toHexString ((int) memoryType).paddedLeft ('0', 2)
                                  + " elmode=" + juce::String (s.lmElmodeRaw)
                                  + " gen=" + juce::String (sy99BankClick0040FetchGeneration()));
        fetch (globalMm, memoryType);
        return;
    }

    juce::Logger::writeToLog ("[BankClick] fetch0040 skipped: callback not wired");
}

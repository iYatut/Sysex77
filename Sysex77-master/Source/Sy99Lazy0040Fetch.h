/*
  Lazy 0040VC fetch after library BankClick (8101 from .syx, 0040 from SY99 on demand).
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

inline int& sy99BankClick0040FetchGeneration() noexcept
{
    static int generation = 0;
    return generation;
}

inline int& sy99BankClick0040FetchTargetMm() noexcept
{
    static int mm = -1;
    return mm;
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

inline void sy99DeferBankClick0040Fetch (int globalMm, uint8 memoryType) noexcept
{
    auto& p = sy99PendingBankClick0040();
    p.valid = true;
    p.mm = globalMm;
    p.memoryType = memoryType;
    juce::Logger::writeToLog ("[BankClick] fetch0040 deferred until sync idle mm="
                              + juce::String::toHexString (globalMm).paddedLeft ('0', 2));
}

inline void sy99MaybeRequestBankClick0040Fetch (int globalMm, uint8 memoryType = 0x00) noexcept;

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
    sy99MaybeRequestBankClick0040Fetch (mm, mt);
}

/** True when elmode 4/8 needs 0040 before first editor apply. */
inline bool sy99ShouldDeferBankClickEditorApply() noexcept
{
    const auto& s = getLiveSynthState();

    return s.hasParsedBulk8101
           && ! s.hasParsedBulk0040
           && YamahaLmVoiceDump::efsendBulk0040TrustedForElmodeRaw (s.lmElmodeRaw);
}

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

    if (YamahaLmVoiceDump::efsendBulk0040TrustedForElmodeRaw (s.lmElmodeRaw))
    {
        s.elementEfsendlvlRaw[0] = -1;
        s.elementEfsendlvlRaw[1] = -1;
    }

    const bool ingested = s.ingestLm0040FromSysexMessages (messages);

    juce::String log = "[BankClick] fetch0040 elmode=" + juce::String (s.lmElmodeRaw)
                       + " has0040=" + juce::String (s.hasParsedBulk0040 ? 1 : 0);

    if (s.lm0040EfsdlvE1Raw >= 0)
        log += " EFSDLV_E1=" + juce::String (s.lm0040EfsdlvE1Raw);

    if (s.lm0040EfsdlvE2Raw >= 0)
        log += " EFSDLV_E2=" + juce::String (s.lm0040EfsdlvE2Raw);

    if (s.lm0040RndpRaw >= 0)
        log += " RNDP=" + juce::String (s.lm0040RndpRaw);

    if (s.lm0040WpbrRaw >= 0)
        log += " WPBR=" + juce::String (s.lm0040WpbrRaw);

    juce::ignoreUnused (ingested);
    juce::Logger::writeToLog (log);

    sy99BankClick0040InFlightGeneration() = -1;

    if (! s.hasParsedBulk0040)
        return false;

    if (auto& reapply = sy99BankClick0040ReapplyCallback(); reapply != nullptr)
        reapply();

    return true;
}

/** Request lazy 0040 when library slot has 8101 but no paired 0040 in .syx. */
inline void sy99MaybeRequestBankClick0040Fetch (int globalMm, uint8 memoryType) noexcept
{
    const auto& s = getLiveSynthState();

    if (! s.hasParsedBulk8101 || s.lmElmodeRaw < 0)
    {
        juce::Logger::writeToLog ("[BankClick] fetch0040 skipped: has8101="
                                  + juce::String (s.hasParsedBulk8101 ? 1 : 0)
                                  + " elmode=" + juce::String (s.lmElmodeRaw));
        return;
    }

    if (s.hasParsedBulk0040)
    {
        juce::Logger::writeToLog ("[BankClick] fetch0040 skipped: paired0040 in syx");
        return;
    }

    if (auto& portStatus = sy99MidiPortStatusCallback(); portStatus != nullptr)
    {
        const juce::String err = portStatus();

        if (err.isNotEmpty())
        {
            juce::Logger::writeToLog ("[BankClick] fetch0040 blocked: " + err);
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

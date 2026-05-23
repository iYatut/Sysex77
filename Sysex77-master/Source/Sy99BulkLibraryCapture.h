/*
  ==============================================================================

    Sy99BulkLibraryCapture.h
    Receive SY99 bulk dumps into the library folder (%AppData%/Sysex77).

    SY99: Utility → MIDI → Bulk Dump → choose dump type, then DUMP OUT.
    Disable Bulk Protect on synth and in Sysex77 MIDI page if transfer fails.

  ==============================================================================
*/

#pragma once

#include <atomic>

#include "LiveSynthState.h"
#include "VoicesTableModel.h"
#include "YamahaLmVoiceDump.h"
#include "../JuceLibraryCode/JuceHeader.h"

extern juce::Array<juce::MidiMessage> arraySysex;
extern bool newMessage;
extern bool requestSysex;
extern int timeOut;
extern bool loadBankRequest;

enum class Sy99BulkCaptureKind
{
    voiceBank64,
    voiceSingle,
    receiveAll
};

enum class Sy99BulkCaptureSessionMode
{
    singleAutoClose,
    continuous
};

struct Sy99BulkDumpAnalysis
{
    int messageCount = 0;
    int64 totalBytes = 0;
    int lm8101Frames = 0;
    int lm0040Frames = 0;

    juce::String suggestFileBaseName() const noexcept
    {
        if (lm8101Frames >= 48)
            return "VOICE64";

        if (lm8101Frames >= 8)
            return "VOICEBANK";

        if (lm8101Frames >= 1 && messageCount <= 8)
            return "VOICE1";

        if (messageCount >= 100)
            return "BULK";

        return "DUMP";
    }

    juce::String makeSummaryLine() const noexcept
    {
        return String (messageCount) + " msg, "
               + String (totalBytes) + " B, "
               + String (lm8101Frames) + "×8101, "
               + String (lm0040Frames) + "×0040";
    }
};

inline Sy99BulkDumpAnalysis analyzeCapturedSysexMessages (
    const juce::Array<juce::MidiMessage>& messages) noexcept
{
    Sy99BulkDumpAnalysis a;
    a.messageCount = messages.size();

    for (const auto& m : messages)
    {
        if (! m.isSysEx())
            continue;

        a.totalBytes += m.getRawDataSize();

        const auto* raw = m.getRawData();
        const int rawSize = m.getRawDataSize();

        if (rawSize < 16)
            continue;

        if (YamahaLmVoiceDump::findLmBlock (raw, (size_t) rawSize, "8101VC").data != nullptr)
            ++a.lm8101Frames;

        if (YamahaLmVoiceDump::findLmBlock (raw, (size_t) rawSize, "0040VC").data != nullptr)
            ++a.lm0040Frames;
    }

    return a;
}

inline int bulkCaptureIdleThresholdTicks (int capturedMessages) noexcept
{
    // 500 ms per tick → 12 s default, 25 s while a large bank is still streaming.
    if (capturedMessages > 80)
        return 50;

    if (capturedMessages > 20)
        return 36;

    return 24;
}

inline juce::String bulkCaptureInstructionForKind (Sy99BulkCaptureKind kind) noexcept
{
    switch (kind)
    {
        case Sy99BulkCaptureKind::voiceBank64:
            return "SY99: Utility → MIDI → Bulk Dump → 05: 64 Voice → DUMP OUT. "
                   "Press STOP when the synth shows Completed.";

        case Sy99BulkCaptureKind::voiceSingle:
            return "SY99: Utility → MIDI → Bulk Dump → 07: 1 Voice → DUMP OUT. "
                   "Press STOP when done.";

        case Sy99BulkCaptureKind::receiveAll:
            return "SY99: Bulk Dump each bank/type you need (start with 05: 64 Voice). "
                   "Each dump saves automatically; press STOP when finished.";
    }

    return {};
}

/** Programmatic sync (menu 31 / auto-64) — one canonical file per phase, overwrite in place. */
inline bool sy99UsesStableSyncCaptureName (const juce::String& namePrefix) noexcept
{
    return namePrefix.startsWithIgnoreCase ("AUTOSYNC-");
}

inline juce::File sy99StableSyncCapturePath (const juce::String& namePrefix) noexcept
{
    return libraryCapturesDirPath().getChildFile (namePrefix + ".syx");
}

/** Remove legacy `PREFIX-YYYYMMDD-HHMMSS.syx` after writing `PREFIX.syx`. */
inline void sy99PruneTimestampedSyncCaptures (const juce::String& namePrefix,
                                              const juce::File& keepFile) noexcept
{
    const juce::File cap = libraryCapturesDirPath();

    if (! cap.isDirectory())
        return;

    juce::Array<juce::File> stale;
    cap.findChildFiles (stale, juce::File::findFiles, false, namePrefix + "-*.syx");

    for (const auto& f : stale)
    {
        if (f == keepFile)
            continue;

        if (f.deleteFile())
            juce::Logger::writeToLog ("[BulkRecv] Pruned stale " + f.getFileName());
    }
}

inline juce::File sy99FindSyncCaptureFile (const juce::Array<juce::File>& savedFiles,
                                           const char* filePrefix) noexcept
{
    if (filePrefix == nullptr || filePrefix[0] == '\0')
        return {};

    const juce::String prefix (filePrefix);
    const juce::String canonicalName = prefix + ".syx";

    for (const auto& f : savedFiles)
    {
        if (f.getFileName().equalsIgnoreCase (canonicalName))
            return f;
    }

    for (const auto& f : savedFiles)
    {
        if (f.getFileName().startsWith (prefix))
            return f;
    }

    const juce::File onDisk = sy99StableSyncCapturePath (prefix);

    if (onDisk.existsAsFile())
        return onDisk;

    return {};
}

inline juce::File saveCapturedSysexToLibrary (const juce::Array<juce::MidiMessage>& messages,
                                              const juce::String& namePrefix,
                                              Sy99BulkDumpAnalysis* analysisOut = nullptr) noexcept
{
    if (messages.isEmpty() || ! messages[0].isSysEx())
        return {};

    if (! appDirPath.exists())
        appDirPath.createDirectory();

    const File capturesDir = libraryCapturesDirPath();

    if (! capturesDir.exists())
        capturesDir.createDirectory();

    const auto analyzed = analyzeCapturedSysexMessages (messages);

    if (analysisOut != nullptr)
        *analysisOut = analyzed;

    juce::File outFile;

    if (sy99UsesStableSyncCaptureName (namePrefix))
        outFile = sy99StableSyncCapturePath (namePrefix);
    else
    {
        const auto stamp = Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
        outFile = capturesDir.getChildFile (namePrefix + "-" + stamp + ".syx");

        if (outFile.existsAsFile())
            outFile = outFile.getNonexistentSibling();
    }

    FileOutputStream fos (outFile);

    if (fos.failedToOpen())
    {
        Logger::writeToLog ("[BulkRecv] Failed to write " + outFile.getFullPathName());
        return {};
    }

    for (const auto& m : messages)
        fos.write (m.getRawData(), m.getRawDataSize());

    fos.flush();

    if (sy99UsesStableSyncCaptureName (namePrefix))
    {
        sy99PruneTimestampedSyncCaptures (namePrefix, outFile);
        Logger::writeToLog ("[BulkRecv] Updated sync capture " + outFile.getFileName()
                            + " (" + analyzed.makeSummaryLine() + ")");
    }
    else
    {
        Logger::writeToLog ("[BulkRecv] Saved " + outFile.getFileName()
                            + " (" + analyzed.makeSummaryLine() + ")");
    }

    return outFile;
}

// =============================================================================
// Auto-sync 64 internal voices (SYM7-style programmatic dump requests)
// =============================================================================

static constexpr int kSy99AutoSyncInternalVoiceCount = 64;

/** Poll interval for SYM7 gap scheduling (ms). */
static constexpr int kSy99LibrarySyncTimerIntervalMs = 25;

/** No-RX skip: 2 s at 25 ms/tick. */
static constexpr int kSy99LibrarySyncIdleTicksTimeout = 80;

/** VC phase avg TX-to-TX gap from SYM7 capture (8101VC b28=0). */
static constexpr int kSym7VcInterRequestDelayMs = 258;

/** Timer ticks: 0 = advance on complete SysEx; partial debounce ~50 ms. */
static constexpr int kSy99LibrarySyncIdleTicksComplete = 0;
static constexpr int kSy99LibrarySyncIdleTicksPartial = 2;

/** Idle after complete 8101VC — match SYM7 ~250 ms debounce. */
inline int autoSync64IdleThresholdTicks (
    const juce::Array<juce::MidiMessage>& buffer) noexcept
{
    if (buffer.isEmpty())
        return kSy99LibrarySyncIdleTicksTimeout;

    YamahaLmVoiceDump::Lm8101VcMinimal parsed;

    if (YamahaLmVoiceDump::parseLm8101VcMinimalFromSysexMessages (buffer, parsed)
        && parsed.found8101)
        return kSy99LibrarySyncIdleTicksComplete;

    return kSy99LibrarySyncIdleTicksPartial;
}

/** @deprecated Use autoSync64IdleThresholdTicks during auto-sync. */
inline int bulkCaptureIdleThresholdTicksSingleVoice() noexcept
{
    return 1;
}

inline juce::uint32 sy99Sym7NowMs() noexcept
{
    return juce::Time::getMillisecondCounter();
}

/** SYM7 paces TX-to-TX from previous send time, not from RX completion. */
struct Sy99Sym7Pacing
{
    juce::uint32 lastTxTimeMs = 0;
    juce::uint32 nextSendEarliestMs = 0;
    bool pendingSend = false;
    int lastRxLatencyMs = 0;
    int lastScheduledGapMs = 0;
    int lastBreatheExtraMs = 0;
    int consecutiveSlowRxSlots = 0;
    int consecutiveHealthyRxSlots = 0;
    bool adaptiveBreatheActive = false;
};

/** Turn adaptive breathe on when rx exceeds steady SYM7 (~300 ms). */
static constexpr int kSym7AdaptiveEnableRxMs = 380;

/** Turn off after this many healthy slots (hysteresis). */
static constexpr int kSym7AdaptiveDisableRxMs = 320;
static constexpr int kSym7AdaptiveHealthySlotsToDisable = 6;

/** UI backlog: rx above ~SYM7 steady-state. */
static constexpr int kSym7SlowRxThresholdMs = 450;

/** After this many slow slots in a row, insert an extra pause. */
static constexpr int kSym7SlowRxSlotsTrigger = 2;

/** Extra TX-to-TX gap to let the message thread drain. */
static constexpr int kSym7BreatheExtraMs = 400;

/** Settle pause when switching VC/MU bank (phase boundary). */
static constexpr int kSym7PhaseSettleMs = 500;

inline void sy99Sym7MarkTx (Sy99Sym7Pacing& pacing) noexcept
{
    pacing.lastTxTimeMs = sy99Sym7NowMs();
}

inline void sy99Sym7ScheduleGapFromLastTx (Sy99Sym7Pacing& pacing, int gapMs) noexcept
{
    pacing.lastScheduledGapMs = juce::jmax (0, gapMs);
    pacing.nextSendEarliestMs = pacing.lastTxTimeMs + (juce::uint32) pacing.lastScheduledGapMs;
    pacing.pendingSend = true;
}

inline void sy99Sym7ResetBreatheState (Sy99Sym7Pacing& pacing) noexcept
{
    pacing.consecutiveSlowRxSlots = 0;
    pacing.consecutiveHealthyRxSlots = 0;
    pacing.adaptiveBreatheActive = false;
    pacing.lastBreatheExtraMs = 0;
}

inline void sy99Sym7UpdateAdaptiveBreatheMode (Sy99Sym7Pacing& pacing) noexcept
{
    if (pacing.lastRxLatencyMs <= 0)
        return;

    if (! pacing.adaptiveBreatheActive)
    {
        if (pacing.lastRxLatencyMs >= kSym7AdaptiveEnableRxMs)
            pacing.adaptiveBreatheActive = true;

        return;
    }

    if (pacing.lastRxLatencyMs <= kSym7AdaptiveDisableRxMs)
        ++pacing.consecutiveHealthyRxSlots;
    else
        pacing.consecutiveHealthyRxSlots = 0;

    if (pacing.consecutiveHealthyRxSlots >= kSym7AdaptiveHealthySlotsToDisable)
    {
        pacing.adaptiveBreatheActive = false;
        pacing.consecutiveSlowRxSlots = 0;
        pacing.consecutiveHealthyRxSlots = 0;
    }
}

/** Extend SYM7 gap when UI rx latency runs hot for a couple of slots. */
inline int sy99Sym7ComputeAdaptiveGapMs (Sy99Sym7Pacing& pacing, int baseGapMs) noexcept
{
    pacing.lastBreatheExtraMs = 0;
    sy99Sym7UpdateAdaptiveBreatheMode (pacing);

    if (! pacing.adaptiveBreatheActive)
        return baseGapMs;

    if (pacing.lastRxLatencyMs > kSym7SlowRxThresholdMs)
        ++pacing.consecutiveSlowRxSlots;
    else if (pacing.lastRxLatencyMs > 0)
        pacing.consecutiveSlowRxSlots = 0;

    if (pacing.consecutiveSlowRxSlots >= kSym7SlowRxSlotsTrigger)
    {
        pacing.lastBreatheExtraMs = kSym7BreatheExtraMs;

        const int severity = pacing.lastRxLatencyMs - kSym7SlowRxThresholdMs;

        if (severity > 0)
            pacing.lastBreatheExtraMs += juce::jmin (600, severity / 2);

        pacing.consecutiveSlowRxSlots = 0;
        pacing.consecutiveHealthyRxSlots = 0;
    }

    return baseGapMs + pacing.lastBreatheExtraMs;
}

inline void sy99Sym7ScheduleAdaptiveGapFromLastTx (Sy99Sym7Pacing& pacing, int baseGapMs) noexcept
{
    sy99Sym7ScheduleGapFromLastTx (pacing, sy99Sym7ComputeAdaptiveGapMs (pacing, baseGapMs));
}

inline int sy99Sym7MsUntilSend (const Sy99Sym7Pacing& pacing) noexcept
{
    if (! pacing.pendingSend)
        return 0;

    const auto now = sy99Sym7NowMs();

    if (now >= pacing.nextSendEarliestMs)
        return 0;

    return (int) (pacing.nextSendEarliestMs - now);
}

inline void sy99Sym7ClearPendingSend (Sy99Sym7Pacing& pacing) noexcept
{
    pacing.pendingSend = false;
}

inline void sy99Sym7RecordRxLatency (Sy99Sym7Pacing& pacing) noexcept
{
    const auto now = sy99Sym7NowMs();
    pacing.lastRxLatencyMs = pacing.lastTxTimeMs > 0
                                 ? (int) (now - pacing.lastTxTimeMs)
                                 : 0;
}

struct Sy99AutoSync64Session
{
    bool active = false;
    bool handlingRxSlot = false;
    int currentMm = 0;
    uint8 byte28 = 0;
    Sy99Sym7Pacing pacing;
    int loadedCount = 0;
    juce::Array<juce::MidiMessage> accumulator;
};

inline Sy99AutoSync64Session& sy99AutoSync64Session() noexcept
{
    static Sy99AutoSync64Session session;
    return session;
}

inline void sy99AutoSync64Reset() noexcept
{
    auto& s = sy99AutoSync64Session();
    s.active = false;
    s.currentMm = 0;
    s.byte28 = 0;
    sy99Sym7ClearPendingSend (s.pacing);
    s.pacing = {};
    s.loadedCount = 0;
    s.handlingRxSlot = false;
    s.accumulator.clear();
}

// =============================================================================
// Synth panel → Librairie page navigation (unsolicited 8101VC / 8101MU bulk RX)
// =============================================================================

struct Sy99SynthPanelNavigateHint
{
    bool valid = false;
    Sy99LibraryContentPage page = Sy99LibraryContentPage::internalVoices;
    int mm = 0;
};

inline Sy99LibraryContentPage sy99LibraryContentPageFromSym78101 (bool isMultiBlock,
                                                                  uint8 byte28) noexcept
{
    if (isMultiBlock)
        return byte28 == 0x02 ? Sy99LibraryContentPage::multiPreset
                              : Sy99LibraryContentPage::multiInternal;

    switch (byte28)
    {
        case 0x02: return Sy99LibraryContentPage::preset1Voices;
        case 0x03: return Sy99LibraryContentPage::preset2Voices;
        default:   return Sy99LibraryContentPage::internalVoices;
    }
}

/** LM + tag6 + 00 00 + 12×00 → byte28, mm (SYM7 request/response header). */
inline bool sy99ReadSym78101AddressFromSysexBody (const uint8* d,
                                                    int n,
                                                    const char* tag6,
                                                    uint8& byte28Out,
                                                    uint8& mmOut) noexcept
{
    if (d == nullptr || tag6 == nullptr || n < 26)
        return false;

    for (int i = 0; i <= n - 26; ++i)
    {
        if (d[i] != 'L' || d[i + 1] != 'M')
            continue;

        if (! YamahaLmVoiceDump::tagsEqual6 (d + i + YamahaLmVoiceDump::kLmTagOffsetFromLm, tag6))
            continue;

        const int addr = i + 24;

        if (addr + 1 >= n)
            return false;

        byte28Out = d[addr];
        mmOut = d[addr + 1];
        return true;
    }

    return false;
}

inline Sy99SynthPanelNavigateHint sy99ParseSynthPanelNavigateFromSysexBody (const uint8* d,
                                                                              int n) noexcept
{
    Sy99SynthPanelNavigateHint hint;

    if (d == nullptr || n < 64 || d[0] != 0x43 || d[2] != 0x7A)
        return hint;

    uint8 byte28 = 0;
    uint8 mm = 0;
    bool isMu = false;

    if (sy99ReadSym78101AddressFromSysexBody (d, n, "8101MU", byte28, mm))
        isMu = true;
    else if (! sy99ReadSym78101AddressFromSysexBody (d, n, "8101VC", byte28, mm))
        return hint;

    hint.valid = true;
    hint.mm = (int) mm;
    hint.page = sy99LibraryContentPageFromSym78101 (isMu, byte28);
    return hint;
}

inline juce::String sy99AutoSync64SlotLabel (int mm) noexcept
{
    static const char banks[] = "ABCD";
    const int bankIdx = juce::jlimit (0, 3, mm / 16);
    const int voiceNo = (mm % 16) + 1;
    return juce::String::charToString (banks[bankIdx]) + juce::String (voiceNo)
           + " (mm=" + juce::String::toHexString (mm).paddedLeft ('0', 2) + ")";
}

/** Name-only scan for sync preview — avoids full 8101 parse + debug audit spam. */
inline juce::String sy99PreviewNameFromSysexMessages (
    const juce::Array<juce::MidiMessage>& messages) noexcept
{
    static const char* kPreviewLmTags[] = { "8101VC", "8101MU" };

    for (const char* tag : kPreviewLmTags)
    {
        const auto block = YamahaLmVoiceDump::findLmBlockInSysexMessages (messages, tag);

        if (block.data == nullptr || block.size <= 0)
            continue;

        char name[11] {};
        YamahaLmVoiceDump::copyLm8101BlockName (block.data, block.size, tag, name, (int) sizeof (name));

        if (name[0] != '\0')
            return juce::String (name).trimEnd();
    }

    return {};
}

/** Fill library page slot names from an ordered capture batch (one slot per SysEx frame). */
inline void sy99FillLibraryPageNamesFromMessages (Sy99LibraryContentPage page,
                                                  const juce::Array<juce::MidiMessage>& messages,
                                                  int slotCount) noexcept
{
    beginLibraryPagePreview (page, slotCount);
    auto& slots = libraryPageSlotNames (page);
    int slot = 0;

    for (const auto& m : messages)
    {
        if (! m.isSysEx() || slot >= slotCount)
            continue;

        juce::Array<juce::MidiMessage> one;
        one.add (m);

        const juce::String name = sy99PreviewNameFromSysexMessages (one);
        slots.set (slot, name);
        ++slot;
    }
}

/** Reload slot names from a saved .syx capture file. */
inline int sy99LoadLibraryPageNamesFromCaptureFile (const juce::File& file,
                                                    Sy99LibraryContentPage page,
                                                    int slotCount) noexcept
{
    if (! file.existsAsFile() || slotCount <= 0)
        return 0;

    juce::MemoryBlock data;

    if (! file.loadFileAsData (data))
        return 0;

    const auto* bytes = static_cast<const juce::uint8*> (data.getData());
    const size_t size = data.getSize();

    beginLibraryPagePreview (page, slotCount);
    auto& slots = libraryPageSlotNames (page);
    int slot = 0;

    for (size_t i = 0; i < size && slot < slotCount; ++i)
    {
        if (bytes[i] != 0xF0)
            continue;

        size_t end = i + 1;

        while (end < size && bytes[end] != 0xF7)
            ++end;

        if (end >= size || bytes[end] != 0xF7)
            break;

        const int frameLen = (int) (end - i + 1);
        juce::Array<juce::MidiMessage> one;
        one.add (juce::MidiMessage (bytes + i, frameLen));

        slots.set (slot, sy99PreviewNameFromSysexMessages (one));
        ++slot;
        i = end;
    }

    return slot;
}

inline juce::String sy99AutoSyncVoiceNameFromMessages (
    const juce::Array<juce::MidiMessage>& messages) noexcept
{
    return sy99PreviewNameFromSysexMessages (messages);
}

inline juce::File saveAutoSync64CombinedCapture (
    const juce::Array<juce::MidiMessage>& messages) noexcept
{
    return saveCapturedSysexToLibrary (messages, "AUTOSYNC-VOICE64", nullptr);
}

// =============================================================================
// Full library sync — SYM7 startup parity (multi-phase queue)
// =============================================================================

struct Sy99LibrarySyncPhase
{
    const char* lmType6;
    uint8 byte28;
    int mmCount;
    const char* filePrefix;
    bool updatesVoicePreview;
    uint8 libraryPage;       // Sy99LibraryContentPage
    bool switchesLibraryPage;
    bool inFastSync;
    int interRequestDelayMs;
};

inline const Sy99LibrarySyncPhase* sy99FullLibrarySyncPhases() noexcept
{
    static const Sy99LibrarySyncPhase phases[] =
    {
        // Order + inter-request gaps from SYM7 — 02_startup_full_20260523.txt
        { "8101SY", 0x00,  1, "AUTOSYNC-SY",       false, 0, false, false,   0 },
        { "0040MU", 0x00,  1, "AUTOSYNC-0040MU",   false, 0, false, false,   0 },
        { "8101VC", 0x00, 64, "AUTOSYNC-VC-INT",   true,  0, true,  true, 258 },
        { "8101VC", 0x02, 64, "AUTOSYNC-VC-P2",    true,  1, true,  true, 260 },
        { "8101VC", 0x03, 64, "AUTOSYNC-VC-P3",    true,  2, true,  true, 245 },
        { "8101MU", 0x00, 16, "AUTOSYNC-MU-INT",   true,  3, true,  true, 126 },
        { "8101MU", 0x02, 16, "AUTOSYNC-MU-P2",    true,  4, true,  true, 128 },
        { "0040SA", 0x00, 64, "AUTOSYNC-0040SA",   false, 0, false, false, 140 },
        { "0040WV", 0x00, 64, "AUTOSYNC-0040WV",   false, 0, false, false,  82 },
        { "8101PN", 0x00, 32, "AUTOSYNC-PN-INT",   false, 0, false, false,  92 },
        { "8101PN", 0x02, 64, "AUTOSYNC-PN-P2",    false, 0, false, false,  92 },
        { "8101MT", 0x00,  2, "AUTOSYNC-MT",       false, 0, false, false, 159 },
    };

    return phases;
}

inline int sy99FullLibrarySyncPhaseCount() noexcept
{
    return 12;
}

inline const Sy99LibrarySyncPhase& sy99FullLibrarySyncPhaseAt (int index) noexcept
{
    jassert (index >= 0 && index < sy99FullLibrarySyncPhaseCount());
    return sy99FullLibrarySyncPhases()[index];
}

inline bool sy99LibrarySyncPhaseIncluded (int phaseIndex, bool extended) noexcept
{
    if (phaseIndex < 0 || phaseIndex >= sy99FullLibrarySyncPhaseCount())
        return false;

    const auto& p = sy99FullLibrarySyncPhaseAt (phaseIndex);
    return extended || p.inFastSync;
}

inline int sy99LibrarySyncFirstPhaseIndex (bool extended) noexcept
{
    for (int i = 0; i < sy99FullLibrarySyncPhaseCount(); ++i)
        if (sy99LibrarySyncPhaseIncluded (i, extended))
            return i;

    return 0;
}

inline int sy99LibrarySyncNextPhaseIndex (int afterIndex, bool extended) noexcept
{
    for (int i = afterIndex + 1; i < sy99FullLibrarySyncPhaseCount(); ++i)
        if (sy99LibrarySyncPhaseIncluded (i, extended))
            return i;

    return -1;
}

inline int sy99LibrarySyncIncludedPhaseCount (bool extended) noexcept
{
    int n = 0;

    for (int i = 0; i < sy99FullLibrarySyncPhaseCount(); ++i)
        if (sy99LibrarySyncPhaseIncluded (i, extended))
            ++n;

    return n;
}

inline int sy99LibrarySyncIncludedPhaseOrdinal (int phaseIndex, bool extended) noexcept
{
    int n = 0;

    for (int i = 0; i <= phaseIndex && i < sy99FullLibrarySyncPhaseCount(); ++i)
        if (sy99LibrarySyncPhaseIncluded (i, extended))
            ++n;

    return n;
}

inline int sy99LibrarySyncTotalRequestsForMode (bool extended) noexcept
{
    int total = 0;

    for (int i = 0; i < sy99FullLibrarySyncPhaseCount(); ++i)
        if (sy99LibrarySyncPhaseIncluded (i, extended))
            total += sy99FullLibrarySyncPhaseAt (i).mmCount;

    return total;
}

inline std::function<void()>& sy99LibrarySyncAdvanceOnReceiveCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

/** Wake background sync engine (MIDI thread — no UI callAsync). */
inline void sy99ScheduleLibrarySyncAdvanceOnReceive() noexcept;

/** Idle debounce for mixed bulk types (complete SysEx ending in F7). */
inline int sy99FullLibrarySyncIdleThresholdTicks (
    const juce::Array<juce::MidiMessage>& buffer) noexcept
{
    if (buffer.isEmpty())
        return kSy99LibrarySyncIdleTicksTimeout;

    const auto& last = buffer.getReference (buffer.size() - 1);

    if (last.isSysEx())
    {
        const int n = last.getRawDataSize();

        if (n >= 8 && last.getRawData()[n - 1] == 0xF7)
            return kSy99LibrarySyncIdleTicksComplete;
    }

    return kSy99LibrarySyncIdleTicksPartial;
}

struct Sy99FullLibrarySyncSession
{
    bool active = false;
    bool includeExtendedPhases = false;
    int phaseIndex = 0;
    int currentMm = 0;
    int requestsCompleted = 0;
    bool handlingRxSlot = false;
    Sy99Sym7Pacing pacing;
    juce::Array<juce::MidiMessage> phaseAccumulator;
    juce::Array<juce::File> savedFiles;
};

inline Sy99FullLibrarySyncSession& sy99FullLibrarySyncSession() noexcept
{
    static Sy99FullLibrarySyncSession session;
    return session;
}

inline int sy99FullLibrarySyncEffectivePhaseCount() noexcept
{
    const auto& s = sy99FullLibrarySyncSession();
    return sy99LibrarySyncIncludedPhaseCount (s.active ? s.includeExtendedPhases : false);
}

inline int sy99FullLibrarySyncTotalRequests() noexcept
{
    const auto& s = sy99FullLibrarySyncSession();
    return sy99LibrarySyncTotalRequestsForMode (s.active ? s.includeExtendedPhases : false);
}

inline void sy99FullLibrarySyncReset() noexcept
{
    auto& s = sy99FullLibrarySyncSession();
    s.active = false;
    s.includeExtendedPhases = false;
    s.phaseIndex = 0;
    s.currentMm = 0;
    s.requestsCompleted = 0;
    s.handlingRxSlot = false;
    sy99Sym7ClearPendingSend (s.pacing);
    s.pacing = {};
    s.phaseAccumulator.clear();
    s.savedFiles.clear();
}

inline bool sy99AnyLibrarySyncActive() noexcept
{
    return sy99AutoSync64Session().active || sy99FullLibrarySyncSession().active;
}

/** Map inbound SY99 bulk dump to Librairie VOICE/PRE1/PRE2/MULTI tab + slot. */
inline void handleIncomingLibrarySynthPanelSysex (const juce::MidiMessage& message) noexcept
{
    if (sy99AnyLibrarySyncActive() || requestSysex || gRequestLiveVoiceRead)
        return;

    if (! message.isSysEx())
        return;

    const int n = message.getSysExDataSize();

    if (n < 64)
        return;

    const auto hint = sy99ParseSynthPanelNavigateFromSysexBody (message.getSysExData(), n);

    if (! hint.valid)
        return;

    juce::MessageManager::callAsync ([hint]
    {
        if (auto& nav = librarySynthPanelNavigateCallback(); nav != nullptr)
            nav (hint.page, hint.mm);
    });
}

inline juce::String sy99FullLibrarySyncPhaseLabel (int phaseIndex, int mm) noexcept
{
    if (phaseIndex < 0 || phaseIndex >= sy99FullLibrarySyncPhaseCount())
        return {};

    const auto& phase = sy99FullLibrarySyncPhaseAt (phaseIndex);
    return juce::String (phase.lmType6)
           + " b28=" + juce::String::toHexString ((int) phase.byte28).paddedLeft ('0', 2)
           + " mm=" + juce::String::toHexString (mm).paddedLeft ('0', 2);
}

/** After full sync: rebuild all preview pages from saved captures (source of truth). */
inline void sy99RefreshLibraryNamesFromSyncCaptures (
    const juce::Array<juce::File>& savedFiles,
    bool extended) noexcept
{
    for (int phaseIndex = 0; phaseIndex < sy99FullLibrarySyncPhaseCount(); ++phaseIndex)
    {
        if (! sy99LibrarySyncPhaseIncluded (phaseIndex, extended))
            continue;

        const auto& phase = sy99FullLibrarySyncPhaseAt (phaseIndex);

        if (! phase.updatesVoicePreview)
            continue;

        const juce::File match = sy99FindSyncCaptureFile (savedFiles, phase.filePrefix);

        if (! match.existsAsFile())
            continue;

        const int loaded = sy99LoadLibraryPageNamesFromCaptureFile (
            match,
            libraryContentPageFromId (phase.libraryPage),
            phase.mmCount);

        juce::Logger::writeToLog ("[FullLibrarySync] Names loaded "
                                  + match.getFileName()
                                  + " page="
                                  + juce::String (phase.libraryPage)
                                  + " slots="
                                  + juce::String (loaded)
                                  + "/"
                                  + juce::String (phase.mmCount));
    }
}

inline juce::File saveFullLibrarySyncPhaseCapture (
    const juce::Array<juce::MidiMessage>& messages,
    const char* filePrefix) noexcept
{
    if (filePrefix == nullptr || filePrefix[0] == '\0')
        return {};

    return saveCapturedSysexToLibrary (messages, filePrefix, nullptr);
}

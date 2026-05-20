/*
  ==============================================================================

    LiveSynthState.h
    Separate storage for parameter data received from the SY99 during a
    manual "read current voice" session. Does not replace ValueTree / editor defaults.

  ==============================================================================
*/

#pragma once

#include "YamahaLmVoiceDump.h"

extern juce::File appDirPath; // defined in MidiDemo.h

inline juce::File libraryCapturesDirPath() noexcept
{
    return appDirPath.getChildFile ("captures");
}

/** Parsed live data from the synth (not the editor ValueTree).
    Parameter addresses per _agent_context/sy99_sysex_complete.md (groups 02/03). */
struct LiveSynthState
{
    enum class ParamSource : uint8 { None, Live, Bulk8101, Bulk0040 };

    static int elementIndexFromOffset (uint8 b4) noexcept
    {
        switch (b4)
        {
            case 0x00: return 0;
            case 0x20: return 1;
            case 0x40: return 2;
            case 0x60: return 3;
            default:   return -1;
        }
    }

    static int resolveInt (int liveRaw, int bulk8101Raw, int bulk0040Raw, ParamSource& src) noexcept
    {
        if (liveRaw >= 0)
        {
            src = ParamSource::Live;
            return liveRaw;
        }

        if (bulk8101Raw >= 0)
        {
            src = ParamSource::Bulk8101;
            return bulk8101Raw;
        }

        if (bulk0040Raw >= 0)
        {
            src = ParamSource::Bulk0040;
            return bulk0040Raw;
        }

        src = ParamSource::None;
        return -1;
    }

    static const char* paramSourceTag (ParamSource src) noexcept
    {
        switch (src)
        {
            case ParamSource::Live:      return "live";
            case ParamSource::Bulk8101:  return "8101";
            case ParamSource::Bulk0040:  return "0040";
            default:                     return "-";
        }
    }

    void reset() noexcept
    {
        parameterFrameCount = 0;
        nonParameterMessageCount = 0;
        largestPayloadBytes = 0;
        elmodeRaw = -1;
        commonVolumeRaw = -1;
        memset (voiceName, 0, sizeof (voiceName));
        voiceNameCharCount = 0;
        memset (elementLevelRaw, -1, sizeof (elementLevelRaw));
        memset (elementOutselRaw, -1, sizeof (elementOutselRaw));
        memset (elementEldtRaw, -1, sizeof (elementEldtRaw));
        memset (elementElnsRaw, -1, sizeof (elementElnsRaw));
        memset (elementEnllRaw, -1, sizeof (elementEnllRaw));
        memset (elementEnlhRaw, -1, sizeof (elementEnlhRaw));
        memset (elementEvllRaw, -1, sizeof (elementEvllRaw));
        memset (elementEvlhRaw, -1, sizeof (elementEvlhRaw));
        memset (elementEfsendselRaw, -1, sizeof (elementEfsendselRaw));
        memset (elementEfsendlvlRaw, -1, sizeof (elementEfsendlvlRaw));
        memset (elementEfsdvsnsRaw, -1, sizeof (elementEfsdvsnsRaw));
        memset (groupFrameCount, 0, sizeof (groupFrameCount));

        hasParsedBulk8101 = false;
        hasParsedBulk0040 = false;
        lmElmodeRaw = -1;
        lmWolRaw = -1;
        lmElvlE1Raw = -1;
        lmOutselE1Raw = -1;
        memset (lmElvlRaw, -1, sizeof (lmElvlRaw));
        memset (lmOutselRaw, -1, sizeof (lmOutselRaw));
        memset (lmEldtRaw, -1, sizeof (lmEldtRaw));
        memset (lmElnsRaw, -1, sizeof (lmElnsRaw));
        memset (lmEvllRaw, -1, sizeof (lmEvllRaw));
        memset (lmEvlhRaw, -1, sizeof (lmEvlhRaw));
        lmEfln1ElRaw = -1;
        lmEfsdlvRaw = -1;
        lmEfsdvlRaw = -1;
        memset (lmVoiceName, 0, sizeof (lmVoiceName));

        liveWpbrRaw = -1;
        liveAtpbrRaw = -1;
        livePmasnRaw = -1;
        livePmrngRaw = -1;
        liveAmasnRaw = -1;
        liveAmrngRaw = -1;
        liveFmasnRaw = -1;
        liveFmrngRaw = -1;
        livePnlasnRaw = -1;
        livePnlrngRaw = -1;
        liveCoasnRaw = -1;
        liveCorngRaw = -1;
        livePnbasnRaw = -1;
        livePnbrngRaw = -1;
        liveEgbasnRaw = -1;
        liveEgbrngRaw = -1;
        liveWlasnRaw = -1;
        liveWllmlRaw = -1;
        liveMctunRaw = -1;
        liveRndpRaw = -1;
        liveAftmdRaw = -1;
        liveSptpntRaw = -1;

        lm0040WpbrRaw = -1;
        lm0040AtpbrRaw = -1;
        lm0040PmasnRaw = -1;
        lm0040PmrngRaw = -1;
        lm0040AmasnRaw = -1;
        lm0040AmrngRaw = -1;
        lm0040FmasnRaw = -1;
        lm0040FmrngRaw = -1;
        lm0040PnlasnRaw = -1;
        lm0040PnlrngRaw = -1;
        lm0040CoasnRaw = -1;
        lm0040CorngRaw = -1;
        lm0040PnbasnRaw = -1;
        lm0040PnbrngRaw = -1;
        lm0040EgbasnRaw = -1;
        lm0040EgbrngRaw = -1;
        lm0040WlasnRaw = -1;
        lm0040WllmlRaw = -1;
        lm0040MctunRaw = -1;
        lm0040RndpRaw = -1;
        lm0040AftmdRaw = -1;
        lm0040SptpntRaw = -1;
    }

    bool hasAnySyncSource() const noexcept
    {
        return hasParsedBulk8101 || hasParsedBulk0040 || parameterFrameCount > 0;
    }

    /** Fill bulk-derived fields from LM 8101VC (07:1 Voice); does not touch parameter-change counters. */
    bool ingestLm8101FromBuffer (const void* data, size_t dataSize) noexcept
    {
        YamahaLmVoiceDump::Lm8101VcMinimal parsed;

        if (! YamahaLmVoiceDump::parseLm8101VcMinimal (data, dataSize, parsed))
        {
            hasParsedBulk8101 = false;
            return false;
        }

        Logger::writeToLog ("[LM8101 ingest] " + YamahaLmVoiceDump::formatLm8101ElnsCoverageLine (parsed)
                            + " | " + YamahaLmVoiceDump::formatLm8101MixerTailCoverageLine (parsed));
        applyLm8101Minimal (parsed);
        return true;
    }

    bool ingestLm8101FromSysexMessages (const juce::Array<juce::MidiMessage>& messages) noexcept
    {
        YamahaLmVoiceDump::Lm8101VcMinimal parsed;

        if (! YamahaLmVoiceDump::parseLm8101VcMinimalFromSysexMessages (messages, parsed))
        {
            hasParsedBulk8101 = false;
            return false;
        }

        Logger::writeToLog ("[LM8101 ingest] " + YamahaLmVoiceDump::formatLm8101ElnsCoverageLine (parsed)
                            + " | " + YamahaLmVoiceDump::formatLm8101MixerTailCoverageLine (parsed));
        applyLm8101Minimal (parsed);
        return true;
    }

    bool ingestLm0040FromBuffer (const void* data, size_t dataSize) noexcept
    {
        YamahaLmVoiceDump::Lm0040VcMinimal parsed;

        if (! YamahaLmVoiceDump::parseLm0040VcMinimal (data, dataSize, parsed))
        {
            hasParsedBulk0040 = false;
            return false;
        }

        applyLm0040Minimal (parsed);
        return true;
    }

    bool ingestLm0040FromSysexMessages (const juce::Array<juce::MidiMessage>& messages) noexcept
    {
        YamahaLmVoiceDump::Lm0040VcMinimal parsed;

        if (! YamahaLmVoiceDump::parseLm0040VcMinimalFromSysexMessages (messages, parsed))
        {
            hasParsedBulk0040 = false;
            return false;
        }

        applyLm0040Minimal (parsed);
        return true;
    }

    void applyLm8101Minimal (const YamahaLmVoiceDump::Lm8101VcMinimal& parsed) noexcept;
    void applyLm0040Minimal (const YamahaLmVoiceDump::Lm0040VcMinimal& parsed) noexcept;
    void ingestParameterFrame (uint8 b3, uint8 b4, uint8 b5, uint8 b6, uint8 b7, uint8 b8) noexcept;

    void noteNonParameterMessage (int payloadBytes) noexcept
    {
        ++nonParameterMessageCount;
        largestPayloadBytes = jmax (largestPayloadBytes, payloadBytes);
    }

    String makeSummary (int rawMessageCount, int64 rawTotalBytes) const
    {
        String s;
        s << "param9=" << parameterFrameCount
          << " otherSysex=" << nonParameterMessageCount
          << " maxPayload=" << largestPayloadBytes
          << " rawMsgs=" << rawMessageCount
          << " rawBytes=" << rawTotalBytes;

        if (elmodeRaw >= 0)
            s << " ELMODE_raw=" << elmodeRaw;

        if (voiceNameCharCount > 0)
            s << " VNAM=\"" << String (voiceName, voiceNameCharCount).trimEnd() << "\"";

        if (commonVolumeRaw >= 0)
            s << " WOL=" << commonVolumeRaw;

        if (hasParsedBulk8101)
        {
            s << " LM8101";
            if (lmVoiceName[0] != '\0')
                s << " name=\"" << String (lmVoiceName).trimEnd() << "\"";

            if (lmElmodeRaw >= 0)
                s << " ELMODE=" << lmElmodeRaw;

            if (lmWolRaw >= 0)
                s << " WOL=" << lmWolRaw;

            if (lmElvlE1Raw >= 0)
                s << " ELVL_E1=" << lmElvlE1Raw;

            if (lmOutselE1Raw >= 0)
                s << " OUTSEL_E1=0x" << String::toHexString (lmOutselE1Raw).toUpperCase();

            for (int e = 1; e < 4; ++e)
                if (lmElvlRaw[e] >= 0)
                    s << " ELVL_E" << (e + 1) << "=" << lmElvlRaw[e];
        }

        if (hasParsedBulk0040)
        {
            s << " LM0040";

            if (lm0040WpbrRaw >= 0)
                s << " WPBR=" << lm0040WpbrRaw;

            if (lm0040WllmlRaw >= 0)
                s << " WLLML=" << lm0040WllmlRaw;

            if (lm0040SptpntRaw >= 0)
                s << " SPTPNT=" << lm0040SptpntRaw;
        }

        for (int el = 0; el < 4; ++el)
        {
            if (elementLevelRaw[(size_t) el] >= 0)
                s << " E" << (el + 1) << "_ELVL=" << elementLevelRaw[(size_t) el];
            if (elementOutselRaw[(size_t) el] >= 0)
                s << " E" << (el + 1) << "_OUTSEL=" << elementOutselRaw[(size_t) el];
        }

        for (int g = 0; g < 16; ++g)
            if (groupFrameCount[(size_t) g] > 0)
                s << " GG" << String::toHexString (g).toUpperCase() << "=" << groupFrameCount[(size_t) g];

        return s;
    }

    int parameterFrameCount = 0;
    int nonParameterMessageCount = 0;
    int largestPayloadBytes = 0;
    int elmodeRaw = -1;
    int commonVolumeRaw = -1;
    char voiceName[11] {};
    int voiceNameCharCount = 0;
    int elementLevelRaw[4] {};
    int elementOutselRaw[4] {};
    int elementEldtRaw[4] {};
    int elementElnsRaw[4] {};
    int elementEnllRaw[4] {};
    int elementEnlhRaw[4] {};
    int elementEvllRaw[4] {};
    int elementEvlhRaw[4] {};
    int elementEfsendselRaw[4] {};
    int elementEfsendlvlRaw[4] {};
    int elementEfsdvsnsRaw[4] {};
    int groupFrameCount[16] {};

    bool hasParsedBulk8101 = false;
    bool hasParsedBulk0040 = false;
    int lmElmodeRaw = -1;
    int lmWolRaw = -1;
    int lmElvlE1Raw = -1;
    int lmOutselE1Raw = -1;
    int lmElvlRaw[4] { -1, -1, -1, -1 };
    int lmOutselRaw[4] { -1, -1, -1, -1 };
    int lmEldtRaw[4] { -1, -1, -1, -1 };
    int lmElnsRaw[4] { -1, -1, -1, -1 };
    int lmEvllRaw[4] { -1, -1, -1, -1 };
    int lmEvlhRaw[4] { -1, -1, -1, -1 };
    int lmEfln1ElRaw = -1;
    int lmEfsdlvRaw = -1;
    int lmEfsdvlRaw = -1;
    char lmVoiceName[11] {};

    int liveWpbrRaw = -1;
    int liveAtpbrRaw = -1;
    int livePmasnRaw = -1;
    int livePmrngRaw = -1;
    int liveAmasnRaw = -1;
    int liveAmrngRaw = -1;
    int liveFmasnRaw = -1;
    int liveFmrngRaw = -1;
    int livePnlasnRaw = -1;
    int livePnlrngRaw = -1;
    int liveCoasnRaw = -1;
    int liveCorngRaw = -1;
    int livePnbasnRaw = -1;
    int livePnbrngRaw = -1;
    int liveEgbasnRaw = -1;
    int liveEgbrngRaw = -1;
    int liveWlasnRaw = -1;
    int liveWllmlRaw = -1;
    int liveMctunRaw = -1;
    int liveRndpRaw = -1;
    int liveAftmdRaw = -1;
    int liveSptpntRaw = -1;

    int lm0040WpbrRaw = -1;
    int lm0040AtpbrRaw = -1;
    int lm0040PmasnRaw = -1;
    int lm0040PmrngRaw = -1;
    int lm0040AmasnRaw = -1;
    int lm0040AmrngRaw = -1;
    int lm0040FmasnRaw = -1;
    int lm0040FmrngRaw = -1;
    int lm0040PnlasnRaw = -1;
    int lm0040PnlrngRaw = -1;
    int lm0040CoasnRaw = -1;
    int lm0040CorngRaw = -1;
    int lm0040PnbasnRaw = -1;
    int lm0040PnbrngRaw = -1;
    int lm0040EgbasnRaw = -1;
    int lm0040EgbrngRaw = -1;
    int lm0040WlasnRaw = -1;
    int lm0040WllmlRaw = -1;
    int lm0040MctunRaw = -1;
    int lm0040RndpRaw = -1;
    int lm0040AftmdRaw = -1;
    int lm0040SptpntRaw = -1;
};

// --- Manual live-voice read session (globals; same pattern as requestSysex) ---

static LiveSynthState gLiveSynthState;
static Array<MidiMessage> gArrayLiveReadSysex;
static bool gRequestLiveVoiceRead = false;
static int gLiveVoiceReadIdleTicks = 0;
/** Clears MIDI monitor on next async tick when a live-read session starts. */
static bool gLiveReadClearMonitorPending = false;

/** ~512 KB monitor budget: enough hex for one SY99 voice dump (several LM blocks). */
static constexpr int kMidiMonitorMaxCharsLiveRead = 512 * 1024;

inline int64 liveReadSysexTotalBytes() noexcept
{
    int64 total = 0;
    for (const auto& m : gArrayLiveReadSysex)
        total += m.getSysExDataSize();
    return total;
}

inline bool isLiveVoiceReadActive() noexcept
{
    return gRequestLiveVoiceRead;
}

inline LiveSynthState& getLiveSynthState() noexcept
{
    return gLiveSynthState;
}

/** Slice one F0…F7 voice frame from a bank .syx using parsed offset table (BankTableModel). */
inline bool extractBankVoiceFrameSlice (const File& bankFile,
                                        int voiceIndex,
                                        const Array<int>& offsets,
                                        const Array<int>& lengths,
                                        MemoryBlock& outFrame,
                                        String& failReason) noexcept
{
    outFrame.reset();
    failReason.clear();

    if (voiceIndex < 0)
    {
        failReason = "voice index " + String (voiceIndex) + " out of range";
        return false;
    }

    if (voiceIndex >= offsets.size() || voiceIndex >= lengths.size())
    {
        failReason = "voice index " + String (voiceIndex)
                     + " out of range (parsed slots=" + String (offsets.size()) + ")";
        return false;
    }

    if (! bankFile.existsAsFile())
    {
        failReason = "bank file missing: " + bankFile.getFullPathName();
        return false;
    }

    MemoryBlock bankData;

    if (! bankFile.loadFileAsData (bankData))
    {
        failReason = "failed to load bank file: " + bankFile.getFullPathName();
        return false;
    }

    const int offset = offsets[voiceIndex];
    const int length = lengths[voiceIndex];

    if (offset < 0 || length <= 0
        || (size_t) (offset + length) > bankData.getSize())
    {
        failReason = "stale offset/length for slot " + String (voiceIndex)
                     + " (offset=" + String (offset) + " length=" + String (length)
                     + " fileBytes=" + String ((int64) bankData.getSize()) + ")";
        return false;
    }

    const auto* bytes = static_cast<const uint8*> (bankData.getData());

    if (bytes[offset] != 0xf0 || bytes[offset + length - 1] != 0xf7)
    {
        failReason = "slot " + String (voiceIndex) + " is not a valid F0…F7 frame (length "
                     + String (length) + ")";
        return false;
    }

    outFrame.append (bytes + offset, (size_t) length);
    return true;
}

/** Stage 5: parse LM 8101VC + paired 0040VC from library slot; fills gLiveSynthState (no synth I/O). */
inline bool ingestLm8101FromBankVoiceSlotAndLog (int voiceIndex,
                                                 const String& bankFileName,
                                                 const Array<int>& offsets,
                                                 const Array<int>& lengths) noexcept
{
    if (bankFileName.isEmpty())
    {
        gLiveSynthState.hasParsedBulk8101 = false;
        gLiveSynthState.hasParsedBulk0040 = false;
        Logger::writeToLog ("[BankClick] parse failed: no bank selected");
        return false;
    }

    MemoryBlock frame;
    String failReason;
    const File bankFile = appDirPath.getChildFile (bankFileName);

    if (! extractBankVoiceFrameSlice (bankFile, voiceIndex, offsets, lengths, frame, failReason))
    {
        gLiveSynthState.hasParsedBulk8101 = false;
        gLiveSynthState.hasParsedBulk0040 = false;
        Logger::writeToLog ("[BankClick] parse failed: " + failReason);
        return false;
    }

    if (frame.getSize() < (size_t) YamahaLmVoiceDump::kMin8101VcFrameSize)
    {
        gLiveSynthState.hasParsedBulk8101 = false;
        gLiveSynthState.hasParsedBulk0040 = false;
        Logger::writeToLog ("[BankClick] parse failed: truncated frame (length "
                            + String ((int64) frame.getSize()) + ", need >= "
                            + String (YamahaLmVoiceDump::kMin8101VcFrameSize) + ")");
        return false;
    }

    const bool ingested8101 = gLiveSynthState.ingestLm8101FromBuffer (frame.getData(), frame.getSize());

    if (! ingested8101)
    {
        gLiveSynthState.hasParsedBulk8101 = false;
        Logger::writeToLog ("[BankClick] parse failed: no LM 8101VC in frame (length "
                            + String ((int64) frame.getSize()) + ")");
        return false;
    }

    MemoryBlock bankData;

    if (bankFile.loadFileAsData (bankData))
    {
        const auto* bytes = static_cast<const uint8*> (bankData.getData());
        const int slotOffset = offsets[voiceIndex];
        const int slotLength = lengths[voiceIndex];
        int pairedOff = -1;
        int pairedLen = 0;

        if (YamahaLmVoiceDump::findPaired0040FrameAfter (bytes, bankData.getSize(),
                                                          slotOffset + slotLength,
                                                          pairedOff, pairedLen))
        {
            gLiveSynthState.ingestLm0040FromBuffer (bytes + (size_t) pairedOff, (size_t) pairedLen);
        }
    }

    const auto& lm = gLiveSynthState;
    YamahaLmVoiceDump::Lm8101VcMinimal p8101;
    YamahaLmVoiceDump::parseLm8101VcMinimal (frame.getData(), frame.getSize(), p8101);
    Logger::writeToLog ("[BankClick] parsed8101: " + YamahaLmVoiceDump::formatLm8101VcMinimalLogLine (p8101));

    if (lm.hasParsedBulk0040)
    {
        YamahaLmVoiceDump::Lm0040VcMinimal p0040;
        p0040.found0040 = true;
        p0040.wpbrRaw = lm.lm0040WpbrRaw;
        p0040.wllmlRaw = lm.lm0040WllmlRaw;
        p0040.sptpntRaw = lm.lm0040SptpntRaw;
        p0040.atpbrRaw = lm.lm0040AtpbrRaw;
        p0040.aftmdRaw = lm.lm0040AftmdRaw;
        Logger::writeToLog ("[BankClick] parsed0040: " + YamahaLmVoiceDump::formatLm0040VcMinimalLogLine (p0040));
    }

    return true;
}

/** Yamaha SY99 bulk / single-voice dump REQUEST — NOT in sy99_sysex_complete.md
    (that file covers only F0 43 1n 34 parameter change). CHANGELOG cites F0 43 2n 34 ..
    from SY99E2.PDF — still unverified on hardware. */
inline void sendCurrentVoiceDumpRequestPendingVerification()
{
    Logger::writeToLog ("[LiveRead] TODO: dump-request SysEx (F0 43 2n …) not in "
                        "sy99_sysex_complete.md — need SY99E2.PDF capture or DUMP OUT on synth. "
                        "Listening for parameter-change stream (F0 43 1n 34 …).");
}

inline void saveLiveReadCaptureToSyx()
{
    if (gArrayLiveReadSysex.isEmpty())
        return;

    if (! appDirPath.exists())
        appDirPath.createDirectory();

    const File capturesDir = libraryCapturesDirPath();

    if (! capturesDir.exists())
        capturesDir.createDirectory();

    auto stamp = Time::getCurrentTime().formatted ("LIVEREAD-%Y%m%d-%H%M%S");
    File outFile = capturesDir.getChildFile (stamp + ".syx");

    if (outFile.existsAsFile())
        outFile = outFile.getNonexistentSibling();

    FileOutputStream fos (outFile);

    if (fos.failedToOpen())
    {
        Logger::writeToLog ("[LiveRead] Failed to write " + outFile.getFullPathName());
        return;
    }

    for (auto& m : gArrayLiveReadSysex)
        fos.write (m.getRawData(), m.getRawDataSize());

    fos.flush();
    Logger::writeToLog ("[LiveRead] Saved " + outFile.getFullPathName()
                        + " (" + String (gArrayLiveReadSysex.size()) + " messages)");
}

inline void beginLiveVoiceReadFromSy99()
{
    gLiveSynthState.reset();
    gArrayLiveReadSysex.clear();
    gRequestLiveVoiceRead = true;
    gLiveVoiceReadIdleTicks = 0;
    gLiveReadClearMonitorPending = true;
    Logger::writeToLog ("[SyncFromSY99] Started — SY99: Utility → MIDI → Bulk Dump → 07:1 Voice → "
                        "DUMP OUT, then Stop sync in app.");
    sendCurrentVoiceDumpRequestPendingVerification();
}

inline bool liveReadPayloadContainsLm8101Tag (const uint8* d, int n) noexcept
{
    if (d == nullptr || n < 16)
        return false;

    for (int i = 0; i + 10 < n; ++i)
    {
        if (d[i] != 'L' || d[i + 1] != 'M')
            continue;

        if (std::memcmp (d + i + YamahaLmVoiceDump::kLmTagOffsetFromLm, "8101VC", 6) == 0)
            return true;
    }

    return false;
}

inline void finishLiveVoiceReadFromSy99()
{
    if (! gRequestLiveVoiceRead)
        return;

    gRequestLiveVoiceRead = false;
    saveLiveReadCaptureToSyx();

    const int totalMsgs = gArrayLiveReadSysex.size();
    int param9Msgs = 0;
    int nearBulk8101Msgs = 0;
    bool lm8101TagInPayload = false;
    bool lm8101WrappedFrame = false;

    for (const auto& m : gArrayLiveReadSysex)
    {
        if (! m.isSysEx())
            continue;

        const int n = m.getSysExDataSize();
        const uint8* d = m.getSysExData();

        if (n == 9 && d != nullptr && d[0] == 0x43 && d[2] == 0x34)
            ++param9Msgs;

        if (n >= 400 && n <= 700)
            ++nearBulk8101Msgs;

        if (liveReadPayloadContainsLm8101Tag (d, n))
            lm8101TagInPayload = true;

        const uint8* raw = m.getRawData();
        const int rawN = m.getRawDataSize();

        if (raw != nullptr && rawN > 0 && raw[0] == 0xf0)
        {
            const auto block = YamahaLmVoiceDump::findLmBlock (raw, (size_t) rawN, "8101VC");

            if (block.data != nullptr)
                lm8101WrappedFrame = true;
        }
    }

    const bool lm8101Ingested = gLiveSynthState.ingestLm8101FromSysexMessages (gArrayLiveReadSysex);
    const bool lm0040Ingested = gLiveSynthState.ingestLm0040FromSysexMessages (gArrayLiveReadSysex);

    Logger::writeToLog ("[SyncFromSY99] capture diag: msgs=" + String (totalMsgs)
                        + " param9=" + String (param9Msgs)
                        + " nearBulk8101=" + String (nearBulk8101Msgs)
                        + " lmTagInPayload=" + String ((int) lm8101TagInPayload)
                        + " lm8101Wrapped=" + String ((int) lm8101WrappedFrame)
                        + " lm8101Ingested=" + String ((int) lm8101Ingested)
                        + " lm0040Ingested=" + String ((int) lm0040Ingested));

    if (lm8101Ingested || lm0040Ingested || gLiveSynthState.parameterFrameCount > 0)
    {
        YamahaLmVoiceDump::Lm8101VcMinimal p8101;
        YamahaLmVoiceDump::parseLm8101VcMinimalFromSysexMessages (gArrayLiveReadSysex, p8101);

        if (p8101.found8101)
            Logger::writeToLog ("[SyncFromSY99] ingest8101: "
                                + YamahaLmVoiceDump::formatLm8101VcMinimalLogLine (p8101));

        if (lm0040Ingested)
        {
            YamahaLmVoiceDump::Lm0040VcMinimal p0040;
            p0040.found0040 = true;
            p0040.wpbrRaw = gLiveSynthState.lm0040WpbrRaw;
            p0040.wllmlRaw = gLiveSynthState.lm0040WllmlRaw;
            p0040.sptpntRaw = gLiveSynthState.lm0040SptpntRaw;
            Logger::writeToLog ("[SyncFromSY99] ingest0040: "
                                + YamahaLmVoiceDump::formatLm0040VcMinimalLogLine (p0040));
        }
    }
    else
    {
        Logger::writeToLog ("[SyncFromSY99] finishLiveVoiceRead: no LM8101VC / LM0040VC ingested");
    }

    Logger::writeToLog ("[SyncFromSY99] finishLiveVoiceRead done: "
                        + gLiveSynthState.makeSummary (gArrayLiveReadSysex.size(),
                                                       liveReadSysexTotalBytes()));
}

inline void toggleLiveVoiceReadFromSy99()
{
    if (gRequestLiveVoiceRead)
        finishLiveVoiceReadFromSy99();
    else
        beginLiveVoiceReadFromSy99();
}

inline void notifyLiveVoiceReadActivity() noexcept
{
    gLiveVoiceReadIdleTicks = 0;
}

inline void tickLiveVoiceReadIdle() noexcept
{
    juce::ignoreUnused (gLiveVoiceReadIdleTicks);
    // Live read ends only when the user toggles the button off (see Voice.h).
}

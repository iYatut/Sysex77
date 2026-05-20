/*
  ==============================================================================

    Sy99BulkLibraryCapture.h
    Receive SY99 bulk dumps into the library folder (%AppData%/Sysex77).

    SY99: Utility → MIDI → Bulk Dump → choose dump type, then DUMP OUT.
    Disable Bulk Protect on synth and in Sysex77 MIDI page if transfer fails.

  ==============================================================================
*/

#pragma once

#include "LiveSynthState.h"
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

    const auto stamp = Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
    juce::File outFile = capturesDir.getChildFile (namePrefix + "-" + stamp + ".syx");

    if (outFile.existsAsFile())
        outFile = outFile.getNonexistentSibling();

    FileOutputStream fos (outFile);

    if (fos.failedToOpen())
    {
        Logger::writeToLog ("[BulkRecv] Failed to write " + outFile.getFullPathName());
        return {};
    }

    for (const auto& m : messages)
        fos.write (m.getRawData(), m.getRawDataSize());

    fos.flush();

    Logger::writeToLog ("[BulkRecv] Saved " + outFile.getFileName()
                        + " (" + analyzed.makeSummaryLine() + ")");

    return outFile;
}

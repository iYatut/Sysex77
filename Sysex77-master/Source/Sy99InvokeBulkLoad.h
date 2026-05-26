/*
  SYM7-style invoke load: TX bulk 8101VC + 0040VC (43 00 7A) + param9 ELMODE to SY99 edit buffer.
  Probe menus #14 / #15.
*/

#pragma once

#include "DeviceModel.h"
#include "LiveSynthState.h"
#include "YamahaLmVoiceDump.h"
#include "Sy99Lazy0040Fetch.h"
#include "../JuceLibraryCode/JuceHeader.h"

inline std::function<void (const juce::MidiMessage&)>& sy99InvokeBulkLoadSendCallback() noexcept
{
    static std::function<void (const juce::MidiMessage&)> cb;
    return cb;
}

inline bool sy99AppendNextSysexFrame (const uint8* data,
                                      size_t dataSize,
                                      size_t& cursor,
                                      juce::MemoryBlock& outFrame) noexcept
{
    outFrame.reset();

    if (data == nullptr || cursor >= dataSize)
        return false;

    while (cursor < dataSize && data[cursor] != 0xf0)
        ++cursor;

    if (cursor >= dataSize)
        return false;

    size_t end = cursor + 1;

    while (end < dataSize && data[end] != 0xf7)
        ++end;

    if (end >= dataSize)
        return false;

    outFrame.append (data + cursor, end - cursor + 1);
    cursor = end + 1;
    return true;
}

inline bool sy99CollectInvokeBulkFramesFromFileData (const void* fileData,
                                                     size_t fileSize,
                                                     juce::MemoryBlock& out8101,
                                                     juce::MemoryBlock& out0040,
                                                     juce::String& failReason) noexcept
{
    out8101.reset();
    out0040.reset();
    failReason.clear();

    const auto* bytes = static_cast<const uint8*> (fileData);

    if (bytes == nullptr || fileSize < 16)
    {
        failReason = "file too small";
        return false;
    }

    size_t cursor = 0;
    juce::MemoryBlock frame;

    while (sy99AppendNextSysexFrame (bytes, fileSize, cursor, frame))
    {
        if (YamahaLmVoiceDump::frameContainsLmTag (static_cast<const uint8*> (frame.getData()),
                                                   (int) frame.getSize(),
                                                   "8101VC")
            && out8101.isEmpty())
        {
            out8101 = frame;
            continue;
        }

        if (YamahaLmVoiceDump::frameContainsLmTag (static_cast<const uint8*> (frame.getData()),
                                                   (int) frame.getSize(),
                                                   "0040VC")
            && out0040.isEmpty())
        {
            out0040 = frame;
            continue;
        }

        if (! out8101.isEmpty() && ! out0040.isEmpty())
            break;
    }

    if (out8101.isEmpty())
    {
        failReason = "no 8101VC frame in file";
        return false;
    }

    if (out0040.isEmpty())
    {
        failReason = "no 0040VC frame in file";
        return false;
    }

    return true;
}

inline bool sy99CollectInvokeBulkFramesFromSyxFile (const juce::File& syxFile,
                                                    juce::MemoryBlock& out8101,
                                                    juce::MemoryBlock& out0040,
                                                    juce::String& failReason) noexcept
{
    juce::MemoryBlock fileData;

    if (! syxFile.existsAsFile())
    {
        failReason = "missing: " + syxFile.getFullPathName();
        return false;
    }

    if (! syxFile.loadFileAsData (fileData))
    {
        failReason = "read failed: " + syxFile.getFullPathName();
        return false;
    }

    return sy99CollectInvokeBulkFramesFromFileData (fileData.getData(),
                                                    fileData.getSize(),
                                                    out8101,
                                                    out0040,
                                                    failReason);
}

inline juce::File sy99BaselineAnonimFixtureFile() noexcept
{
    const juce::File candidates[] =
    {
        juce::File::getCurrentWorkingDirectory()
            .getChildFile ("_agent_context/fixtures/baseline_anonim.syx"),
        juce::File::getCurrentWorkingDirectory()
            .getChildFile ("_agent_context/fixtures/baseline_a1_voice.syx"),
        juce::File ("c:/projects/sy_99/_agent_context/fixtures/baseline_anonim.syx"),
        juce::File ("c:/projects/sy_99/_agent_context/fixtures/baseline_a1_voice.syx"),
        juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
            .getChildFile ("Sysex77/captures/baseline_anonim.syx"),
    };

    for (const auto& f : candidates)
    {
        if (! f.existsAsFile())
            continue;

        juce::MemoryBlock b8101, b0040;
        juce::String err;

        if (sy99CollectInvokeBulkFramesFromSyxFile (f, b8101, b0040, err))
            return f;
    }

    return candidates[2];
}

inline bool sy99CollectInvokeBulkFramesFromVoiceSlot (const juce::File& bankFile,
                                                      int voiceIndex,
                                                      const juce::Array<int>& offsets,
                                                      const juce::Array<int>& lengths,
                                                      juce::MemoryBlock& out8101,
                                                      juce::MemoryBlock& out0040,
                                                      juce::String& failReason) noexcept
{
    out8101.reset();
    out0040.reset();
    failReason.clear();

    if (! extractBankVoiceFrameSlice (bankFile, voiceIndex, offsets, lengths, out8101, failReason))
        return false;

    juce::MemoryBlock bankData;

    if (! bankFile.loadFileAsData (bankData))
    {
        failReason = "read failed: " + bankFile.getFullPathName();
        return false;
    }

    juce::String resolveErr;

    if (! sy99Resolve0040FrameForVoiceSlot (bankFile, voiceIndex, offsets, lengths,
                                           bankData, out0040, resolveErr))
    {
        failReason = resolveErr;
        return false;
    }

    return true;
}

/** SYM7 invoke param9 after bulk load (0040 EFMODE raw → param9 value byte). */
inline juce::MidiMessage sy99BuildInvokeEfmodeParam9Message (uint8 sysexDeviceByte, int efmodeRaw) noexcept
{
    juce::Array<uint8> bytes;
    bytes.add (0xF0);
    bytes.add (0x43);
    bytes.add (uint8 (0x10 | (sysexDeviceByte & 0x0F)));
    bytes.add (0x34);
    bytes.add (0x02);
    bytes.add (0x00);
    bytes.add (0x00);
    bytes.add (0x00);
    bytes.add (0x00);
    bytes.add (uint8 (juce::jlimit (0, 2, efmodeRaw)));
    bytes.add (0xF7);
    return juce::MidiMessage (bytes.getRawDataPointer(), bytes.size());
}

inline int sy99EfmodeRawForInvokeFromFrames (const juce::MemoryBlock& frame0040,
                                             const juce::MemoryBlock& /*frame8101*/) noexcept
{
    YamahaLmVoiceDump::Lm0040VcMinimal p0040;

    if (YamahaLmVoiceDump::parseLm0040VcMinimal (frame0040.getData(),
                                                 frame0040.getSize(),
                                                 p0040)
        && p0040.efmodeRaw >= 0)
        return p0040.efmodeRaw;

    return -1;
}

inline bool sy99IsBulkDataVoiceSysex (const juce::MidiMessage& msg) noexcept
{
    if (! msg.isSysEx())
        return false;

    const uint8* d = msg.getSysExData();
    const int n = msg.getSysExDataSize();

    if (d == nullptr || n < 4)
        return false;

    return d[0] == 0x43 && d[1] == 0x00 && d[2] == 0x7A;
}

struct Sy99InvokeBulkLoadPlayer : private juce::Timer
{
    struct QueuedSend
    {
        juce::MidiMessage message;
        juce::uint32 dueAtMs = 0;
        const char* logTag = "";
    };

    bool active() const noexcept { return running; }

    void cancel() noexcept
    {
        stopTimer();
        queue.clear();
        nextIndex = 0;
        running = false;
    }

    bool beginFromFrames (const juce::MemoryBlock& frame8101,
                          const juce::MemoryBlock& frame0040,
                          const juce::String& contextLabel) noexcept
    {
        cancel();

        if (frame8101.isEmpty() || frame0040.isEmpty())
        {
            juce::Logger::writeToLog ("[InvokeLoad] aborted: empty frame (" + contextLabel + ")");
            return false;
        }

        if (sy99InvokeBulkLoadSendCallback() == nullptr)
        {
            juce::Logger::writeToLog ("[InvokeLoad] aborted: MIDI send callback not wired");
            return false;
        }

        if (sy99BankClick0040CaptureActive())
        {
            juce::Logger::writeToLog ("[InvokeLoad] blocked: BankClick fetch active");
            return false;
        }

        const juce::MidiMessage msg8101 (static_cast<const uint8*> (frame8101.getData()),
                                         (int) frame8101.getSize());
        const juce::MidiMessage msg0040 (static_cast<const uint8*> (frame0040.getData()),
                                         (int) frame0040.getSize());

        const int efmodeRaw = sy99EfmodeRawForInvokeFromFrames (frame0040, frame8101);
        const juce::uint32 t0 = juce::Time::getMillisecondCounter();

        queue.add ({ msg8101, t0, "8101VC" });
        queue.add ({ msg0040, t0 + 27, "0040VC" });

        if (efmodeRaw >= 0)
        {
            const auto efmsg = sy99BuildInvokeEfmodeParam9Message (
                DeviceModel::getInstance().getSysExDeviceID(),
                efmodeRaw);
            queue.add ({ efmsg, t0 + 27 + 500, "EFMODE-param9" });
        }

        nextIndex = 0;
        running = true;

        juce::Logger::writeToLog ("[InvokeLoad] BEGIN " + contextLabel
                                  + " 8101=" + juce::String ((int) frame8101.getSize())
                                  + " B 0040=" + juce::String ((int) frame0040.getSize())
                                  + " B efmode="
                                  + juce::String (efmodeRaw >= 0 ? efmodeRaw : -1));

        startTimer (10);
        return true;
    }

private:
    juce::Array<QueuedSend> queue;
    int nextIndex = 0;
    bool running = false;

    void timerCallback() override
    {
        if (nextIndex >= queue.size())
        {
            stopTimer();
            running = false;
            return;
        }

        const juce::uint32 now = juce::Time::getMillisecondCounter();

        while (nextIndex < queue.size() && now >= queue.getReference (nextIndex).dueAtMs)
        {
            const auto& step = queue.getReference (nextIndex);

            if (auto& sendFn = sy99InvokeBulkLoadSendCallback(); sendFn != nullptr)
            {
                sendFn (step.message);
                juce::String bulkHint;

                if (sy99IsBulkDataVoiceSysex (step.message))
                    bulkHint = " bulkData=1";
                else if (step.message.getSysExDataSize() >= 10
                         && step.message.getSysExData()[1] == 0x10)
                    bulkHint = " param9=" + juce::String ((int) step.message.getSysExData()[9]);

                juce::Logger::writeToLog ("[InvokeLoad] TX " + juce::String (step.logTag)
                                          + " " + juce::String (step.message.getRawDataSize())
                                          + " B" + bulkHint);
            }

            ++nextIndex;
        }

        if (nextIndex >= queue.size())
        {
            stopTimer();
            running = false;
            juce::Logger::writeToLog ("[InvokeLoad] DONE");
        }
    }
};

inline Sy99InvokeBulkLoadPlayer& sy99InvokeBulkLoadPlayer() noexcept
{
    static Sy99InvokeBulkLoadPlayer player;
    return player;
}

inline bool sy99BeginInvokeBulkLoadFromSyxFile (const juce::File& syxFile) noexcept
{
    juce::MemoryBlock b8101, b0040;
    juce::String err;

    if (! sy99CollectInvokeBulkFramesFromSyxFile (syxFile, b8101, b0040, err))
    {
        juce::Logger::writeToLog ("[InvokeLoad] failed: " + err);
        return false;
    }

    return sy99InvokeBulkLoadPlayer().beginFromFrames (b8101, b0040, syxFile.getFileName());
}

inline bool sy99BeginInvokeBulkLoadFromVoiceSlot (const juce::File& bankFile,
                                                  int voiceIndex,
                                                  const juce::Array<int>& offsets,
                                                  const juce::Array<int>& lengths) noexcept
{
    juce::MemoryBlock b8101, b0040;
    juce::String err;

    if (! sy99CollectInvokeBulkFramesFromVoiceSlot (bankFile,
                                                    voiceIndex,
                                                    offsets,
                                                    lengths,
                                                    b8101,
                                                    b0040,
                                                    err))
    {
        juce::Logger::writeToLog ("[InvokeLoad] failed slot " + juce::String (voiceIndex) + ": " + err);
        return false;
    }

    return sy99InvokeBulkLoadPlayer().beginFromFrames (
        b8101,
        b0040,
        bankFile.getFileName() + " mm=" + juce::String::toHexString (voiceIndex).paddedLeft ('0', 2));
}

/** Wired from Librairie — BankClick TX invoke (8101+0040 from capture). */
inline std::function<bool (int globalMm, uint8 memoryType)>& sy99BankClickInvokeLoadCallback() noexcept
{
    static std::function<bool (int, uint8)> cb;
    return cb;
}

struct Sy99PendingBankClickInvokeLoad
{
    bool valid = false;
    int mm = -1;
    uint8 memoryType = 0;
};

inline Sy99PendingBankClickInvokeLoad& sy99PendingBankClickInvokeLoad() noexcept
{
    static Sy99PendingBankClickInvokeLoad pending;
    return pending;
}

inline void sy99DeferBankClickInvokeLoad (int globalMm, uint8 memoryType) noexcept
{
    auto& p = sy99PendingBankClickInvokeLoad();
    p.valid = true;
    p.mm = globalMm;
    p.memoryType = memoryType;
    juce::Logger::writeToLog ("[BankClick] invoke deferred until sync idle mm="
                              + juce::String::toHexString (globalMm).paddedLeft ('0', 2));
}

/** BankClick: TX invoke disabled — use lazy 0040 dump request (Sy99Lazy0040Fetch.h). */
inline void sy99MaybeRequestBankClickInvokeLoad (int globalMm, uint8 memoryType = 0x00) noexcept
{
    sy99MaybeRequestBankClick0040Fetch (globalMm, memoryType);
}

/** Retry deferred invoke after library sync ends (legacy alias → fetch0040). */
inline void sy99DrainPendingBankClickInvokeLoad() noexcept
{
    auto& p = sy99PendingBankClickInvokeLoad();

    if (! p.valid)
        return;

    const int mm = p.mm;
    const uint8 mt = p.memoryType;
    p.valid = false;
    juce::Logger::writeToLog ("[BankClick] invoke drain deferred mm="
                              + juce::String::toHexString (mm).paddedLeft ('0', 2)
                              + " → fetch0040");
    sy99MaybeRequestBankClick0040Fetch (mm, mt);
}

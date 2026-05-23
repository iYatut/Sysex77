/*
  ==============================================================================

    Sy99MidiTransport.h
    Thread-safe MIDI I/O for library sync (pro-audio pattern).

    MIDI input thread  -> postReceive()     -> RX accumulator
    Sync worker thread -> tryTakeCompleteFrame() / send()
    UI thread          -> never touches hardware directly during sync

  ==============================================================================
*/

#pragma once

#include "Sy99BulkLibraryCapture.h"
#include <functional>

class Sy99MidiTransport
{
public:
    using SenderFn = std::function<void (const juce::MidiMessage&)>;

    static Sy99MidiTransport& instance() noexcept
    {
        static Sy99MidiTransport transport;
        return transport;
    }

    void setSender (SenderFn fn) noexcept
    {
        const juce::ScopedLock sl (ioLock);
        sender = std::move (fn);
    }

    void clearSender() noexcept
    {
        const juce::ScopedLock sl (ioLock);
        sender = nullptr;
    }

    /** Called from JUCE MIDI input thread only. */
    void postReceive (const juce::MidiMessage& message) noexcept
    {
        if (! message.isSysEx())
            return;

        const juce::ScopedLock sl (rxLock);
        rxBuffer.add (message);
        lastRxAtMs = sy99Sym7NowMs();
    }

    juce::uint32 lastReceiveAtMs() const noexcept
    {
        return lastRxAtMs.load();
    }

    /** Sync worker: move out a complete dump frame, if any. */
    bool tryTakeCompleteFrame (juce::Array<juce::MidiMessage>& out) noexcept
    {
        const juce::ScopedLock sl (rxLock);

        if (rxBuffer.isEmpty())
            return false;

        if (! sy99AnyLibrarySyncActive())
            return false;

        const int threshold = sy99FullLibrarySyncSession().active
                                  ? sy99FullLibrarySyncIdleThresholdTicks (rxBuffer)
                                  : autoSync64IdleThresholdTicks (rxBuffer);

        if (threshold != kSy99LibrarySyncIdleTicksComplete)
            return false;

        out = rxBuffer;
        rxBuffer.clear();
        return true;
    }

    bool receiveBufferEmpty() const noexcept
    {
        const juce::ScopedLock sl (rxLock);
        return rxBuffer.isEmpty();
    }

    void clearReceiveBuffer() noexcept
    {
        const juce::ScopedLock sl (rxLock);
        rxBuffer.clear();
    }

    /** Sync worker: send next dump request (hardware only — no UI). */
    void send (const juce::MidiMessage& message) noexcept
    {
        SenderFn fn;

        {
            const juce::ScopedLock sl (ioLock);
            fn = sender;
        }

        if (fn != nullptr)
            fn (message);
    }

private:
    Sy99MidiTransport() = default;

    mutable juce::CriticalSection rxLock;
    mutable juce::CriticalSection ioLock;
    juce::Array<juce::MidiMessage> rxBuffer;
    std::atomic<juce::uint32> lastRxAtMs { 0 };
    SenderFn sender;
};

inline int sy99CountSysexFramesInFile (const juce::File& file) noexcept
{
    if (! file.existsAsFile())
        return -1;

    juce::MemoryBlock data;

    if (! file.loadFileAsData (data))
        return -1;

    const auto* bytes = static_cast<const juce::uint8*> (data.getData());
    const size_t size = data.getSize();
    int frames = 0;

    for (size_t i = 0; i < size; ++i)
        if (bytes[i] == 0xF0)
            ++frames;

    return frames;
}

/** Verify saved .syx frame counts match fast-sync phase plan (224 total). */
inline bool sy99AuditFullLibrarySyncCaptures (const juce::Array<juce::File>& savedFiles,
                                              bool extended) noexcept
{
    int expectedTotal = 0;
    int actualTotal = 0;
    bool ok = true;

    for (int phaseIndex = 0; phaseIndex < sy99FullLibrarySyncPhaseCount(); ++phaseIndex)
    {
        if (! sy99LibrarySyncPhaseIncluded (phaseIndex, extended))
            continue;

        const auto& phase = sy99FullLibrarySyncPhaseAt (phaseIndex);
        expectedTotal += phase.mmCount;

        const juce::File match = sy99FindSyncCaptureFile (savedFiles, phase.filePrefix);

        if (! match.existsAsFile())
        {
            juce::Logger::writeToLog ("[FullLibrarySync] Audit FAIL missing file prefix="
                                      + juce::String (phase.filePrefix)
                                      + " expected=" + juce::String (phase.mmCount) + " frames");
            ok = false;
            continue;
        }

        const int frames = sy99CountSysexFramesInFile (match);
        actualTotal += juce::jmax (0, frames);

        if (frames != phase.mmCount)
        {
            juce::Logger::writeToLog ("[FullLibrarySync] Audit FAIL "
                                      + match.getFileName()
                                      + " frames=" + juce::String (frames)
                                      + " expected=" + juce::String (phase.mmCount));
            ok = false;
        }
        else
        {
            juce::Logger::writeToLog ("[FullLibrarySync] Audit OK "
                                      + match.getFileName()
                                      + " frames=" + juce::String (frames)
                                      + " bytes=" + juce::String (match.getSize()));
        }
    }

    juce::Logger::writeToLog ("[FullLibrarySync] Audit total frames="
                              + juce::String (actualTotal)
                              + " expected="
                              + juce::String (expectedTotal)
                              + (ok ? " PASS" : " FAIL"));

    return ok && actualTotal == expectedTotal;
}

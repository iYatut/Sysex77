#pragma once

#include <JuceHeader.h>

/** Append-only SysEx-only stream log (Sysex77_MIDI.log next to executable). */
class MidiStreamLogger
{
public:
    /** New app run → fresh log (matches Sysex77_OUT.log). Previous file kept as .last once. */
    static void beginSession()
    {
        const juce::ScopedLock sl (getLock());

        getStreamPtr().reset();

        const juce::File logFile = getLogFile();

        if (logFile.existsAsFile() && logFile.getSize() > 0)
        {
            const juce::File backup = logFile.getSiblingFile ("Sysex77_MIDI.last.log");
            logFile.copyFileTo (backup);
        }

        logFile.deleteFile();

        getStreamPtr() = logFile.createOutputStream();

        if (auto* out = getStream())
        {
            out->writeText ("--- sysex session "
                            + juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H:%M:%S")
                            + " ---\n",
                            false, false, nullptr);
            out->flush();
            juce::Logger::writeToLog ("[MidiStream] SysEx log (new session): "
                                      + logFile.getFullPathName());
        }
        else
        {
            juce::Logger::writeToLog ("[MidiStream] ERROR failed to open "
                                      + logFile.getFullPathName());
        }
    }

    static void logIncoming (const juce::MidiMessage& msg, const juce::String& portName = {})
    {
        if (msg.isSysEx())
            writeLine ("RX", msg, portName);
    }

    static void logOutgoing (const juce::MidiMessage& msg, const juce::String& portName = {})
    {
        if (msg.isSysEx())
            writeLine ("TX", msg, portName);
    }

    /** SYM7 pacing analysis — interleaved with TX/RX in Sysex77_MIDI.log */
    static void logSyncTiming (const juce::String& detail)
    {
        ensureOpen();

        const juce::ScopedLock sl (getLock());
        auto* out = getStream();

        if (out == nullptr)
            return;

        out->writeText (formatTimestamp() + " [TIMING] " + detail + "\n", false, false, nullptr);
        out->flush();
    }

private:
    static juce::CriticalSection& getLock()
    {
        static juce::CriticalSection lock;
        return lock;
    }

    static juce::File getLogFile()
    {
        return juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                   .getSiblingFile ("Sysex77_MIDI.log");
    }

    static std::unique_ptr<juce::FileOutputStream>& getStreamPtr()
    {
        static std::unique_ptr<juce::FileOutputStream> stream;
        return stream;
    }

    static juce::FileOutputStream* getStream()
    {
        return getStreamPtr().get();
    }

    static void ensureOpen()
    {
        const juce::ScopedLock sl (getLock());

        if (getStream() != nullptr)
            return;

        const juce::File logFile = getLogFile();
        logFile.deleteFile();
        getStreamPtr() = logFile.createOutputStream();

        if (getStream() != nullptr)
        {
            getStream()->writeText ("--- sysex session "
                                    + juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H:%M:%S")
                                    + " (reopen) ---\n",
                                    false, false, nullptr);
            getStream()->flush();
            juce::Logger::writeToLog ("[MidiStream] SysEx log (reopen truncate): "
                                      + logFile.getFullPathName());
        }
    }

    static juce::String formatTimestamp()
    {
        return juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H:%M:%S.")
             + juce::String ((int) (juce::Time::getMillisecondCounter() % 1000)).paddedLeft ('0', 3);
    }

    static juce::String formatMessageBody (const juce::MidiMessage& msg)
    {
        jassert (msg.isSysEx());

        juce::String hex = "F0";
        const uint8* d = msg.getSysExData();
        const int len = msg.getSysExDataSize();

        for (int i = 0; i < len; ++i)
            hex << ' ' << juce::String::toHexString (d[i]).paddedLeft ('0', 2).toUpperCase();

        hex << " F7";
        return hex;
    }

    static void writeLine (const char* direction,
                           const juce::MidiMessage& msg,
                           const juce::String& portName)
    {
        jassert (msg.isSysEx());

        ensureOpen();

        const juce::ScopedLock sl (getLock());
        auto* out = getStream();

        if (out == nullptr)
            return;

        juce::String line = formatTimestamp() + " [" + direction + "]";

        if (portName.isNotEmpty())
            line << " port=" << portName;

        line << ' ' << formatMessageBody (msg) << '\n';

        out->writeText (line, false, false, nullptr);
        out->flush();
    }
};

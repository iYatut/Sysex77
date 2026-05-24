/*
  ==============================================================================

    Sy99LibrarySyncEngine.h
    Background sync loop (MIDI pacing off UI thread).
    UI: coalesced AsyncUpdater — write slot name, one repaint, no per-slot queue.

  ==============================================================================
*/

#pragma once

#include "Sy99BulkLibraryCapture.h"
#include "Sy99MidiTransport.h"
#include "Sy99DumpRequest.h"
#include "DeviceModel.h"
#include "VoicesTableModel.h"
#include "MidiStreamLogger.h"

extern bool requestSysex;

inline juce::CriticalSection& sy99LibrarySyncPreviewLock() noexcept
{
    static juce::CriticalSection lock;
    return lock;
}

struct Sy99LibrarySyncHost
{
    std::function<void()> requestUiRefresh;
    std::function<void (int phaseIndex)> preparePhaseOnUi;
    std::function<void()> finishFullSyncOnUi;
    std::function<void (const juce::File& saved)> finishAuto64OnUi;
    std::function<void()> failAuto64OnUi;
};

inline Sy99LibrarySyncHost& sy99LibrarySyncHost() noexcept
{
    static Sy99LibrarySyncHost host;
    return host;
}

inline void sy99LibrarySyncLogTiming (const Sy99Sym7Pacing& pacing,
                                      const juce::String& label,
                                      bool timeoutSkip = false) noexcept
{
    const int waitMs = sy99Sym7MsUntilSend (pacing);
    juce::String line = label
                        + (timeoutSkip ? " timeout" : "")
                        + " rx=" + juce::String (pacing.lastRxLatencyMs) + "ms"
                        + " gap=" + juce::String (pacing.lastScheduledGapMs) + "ms"
                        + " adapt=" + juce::String (pacing.adaptiveBreatheActive ? "on" : "off");

    if (pacing.lastBreatheExtraMs > 0)
        line += " breathe=+" + juce::String (pacing.lastBreatheExtraMs) + "ms";

    line += " wait=" + juce::String (waitMs) + "ms";

    juce::Logger::writeToLog ("[SyncTiming] " + line);
    MidiStreamLogger::logSyncTiming (line);
}

inline void sy99LibrarySyncSendAuto64Request() noexcept
{
    auto& s = sy99AutoSync64Session();
    sendSym78101VcRequest (DeviceModel::getInstance().getSysExDeviceID(),
                           uint8 (s.currentMm),
                           s.byte28);
    sy99Sym7MarkTx (s.pacing);
}

inline void sy99LibrarySyncSendAuto640040Request() noexcept
{
    auto& s = sy99AutoSync64Session();
    sendSym7VcRequest (DeviceModel::getInstance().getSysExDeviceID(),
                       uint8 (s.currentMm),
                       s.byte28);
    sy99Sym7MarkTx (s.pacing);
}

inline void sy99LibrarySyncSendFullRequest() noexcept
{
    auto& s = sy99FullLibrarySyncSession();

    if (s.phaseIndex < 0 || s.phaseIndex >= sy99FullLibrarySyncPhaseCount())
        return;

    const auto& phase = sy99FullLibrarySyncPhaseAt (s.phaseIndex);
    sendSym78101BulkRequest (DeviceModel::getInstance().getSysExDeviceID(),
                             phase.lmType6,
                             phase.byte28,
                             uint8 (s.currentMm));
    sy99Sym7MarkTx (s.pacing);
}

inline void sy99LibrarySyncWritePreviewSlot (Sy99LibraryContentPage page,
                                             int mm,
                                             const juce::String& name) noexcept
{
    const juce::ScopedLock sl (sy99LibrarySyncPreviewLock());

    auto& slots = libraryPageSlotNames (page);

    while (slots.size() <= mm)
        slots.add ({});

    slots.set (mm, name.trimEnd());
}

inline void sy99LibrarySyncRequestUiRefresh() noexcept
{
    if (auto& host = sy99LibrarySyncHost(); host.requestUiRefresh != nullptr)
        host.requestUiRefresh();
}

inline void sy99LibrarySyncSaveFinishedPhase() noexcept
{
    auto& s = sy99FullLibrarySyncSession();

    if (s.phaseIndex < 0 || s.phaseIndex >= sy99FullLibrarySyncPhaseCount())
        return;

    if (s.phaseAccumulator.isEmpty())
        return;

    const File saved = saveFullLibrarySyncPhaseCapture (
        s.phaseAccumulator,
        sy99FullLibrarySyncPhaseAt (s.phaseIndex).filePrefix);

    if (saved.existsAsFile())
    {
        s.savedFiles.add (saved);
        juce::Logger::writeToLog ("[FullLibrarySync] Saved phase " + juce::String (s.phaseIndex)
                                  + " " + saved.getFullPathName());
    }
}

class Sy99LibrarySyncEngine : private juce::Thread
{
public:
    static Sy99LibrarySyncEngine& instance() noexcept
    {
        static Sy99LibrarySyncEngine engine;
        return engine;
    }

    void ensureStarted() noexcept
    {
        if (! isThreadRunning())
            startThread (juce::Thread::Priority::normal);
    }

    void notifySysexReceived() noexcept
    {
        wakeEvent.signal();
    }

private:
    Sy99LibrarySyncEngine() : juce::Thread ("Sy99LibrarySyncEngine") {}

    juce::WaitableEvent wakeEvent;

    void endSyncCapture() noexcept
    {
        Sy99MidiTransport::instance().clearReceiveBuffer();
        requestSysex = false;
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            if (! sy99AnyLibrarySyncActive())
            {
                wakeEvent.wait (100);
                continue;
            }

            tryProcessCompleteRx();
            flushPendingTx();
            trySyncTimeoutSkip();

            const int waitMs = sy99AnyLibrarySyncActive() ? 8 : 50;
            wakeEvent.wait (waitMs);
        }
    }

    void tryProcessCompleteRx() noexcept
    {
        juce::Array<juce::MidiMessage> batch;

        if (! Sy99MidiTransport::instance().tryTakeCompleteFrame (batch))
            return;

        if (sy99FullLibrarySyncSession().active)
            processFullLibrarySlot (batch, true);
        else if (sy99AutoSync64Session().active)
            processAuto64Slot (batch, true);
    }

    void trySyncTimeoutSkip() noexcept
    {
        if (! sy99AnyLibrarySyncActive())
            return;

        const auto now = sy99Sym7NowMs();
        const auto last = Sy99MidiTransport::instance().lastReceiveAtMs();

        if (last == 0 || now - last < 2000)
            return;

        if (! Sy99MidiTransport::instance().receiveBufferEmpty())
            return;

        if (sy99FullLibrarySyncSession().active)
            processFullLibrarySlot ({}, false);
        else if (sy99AutoSync64Session().active)
            processAuto64Slot ({}, false);
    }

    void flushPendingTx() noexcept
    {
        if (sy99AutoSync64Session().active && sy99AutoSync64Session().pacing.pendingSend)
        {
            const int wait = sy99Sym7MsUntilSend (sy99AutoSync64Session().pacing);

            if (wait > 0)
            {
                juce::Thread::sleep (juce::jmin (wait, 32));
                return;
            }

            sy99Sym7ClearPendingSend (sy99AutoSync64Session().pacing);

            if (sy99AutoSync64Session().subStep == Sy99AutoSync64SubStep::wait0040)
                sy99LibrarySyncSendAuto640040Request();
            else
                sy99LibrarySyncSendAuto64Request();

            return;
        }

        if (sy99FullLibrarySyncSession().active && sy99FullLibrarySyncSession().pacing.pendingSend)
        {
            const int wait = sy99Sym7MsUntilSend (sy99FullLibrarySyncSession().pacing);

            if (wait > 0)
            {
                juce::Thread::sleep (juce::jmin (wait, 32));
                return;
            }

            sy99Sym7ClearPendingSend (sy99FullLibrarySyncSession().pacing);
            sy99LibrarySyncSendFullRequest();
        }
    }

    void finishAuto64CaptureOnUi (const juce::File& saved) noexcept
    {
        endSyncCapture();

        juce::MessageManager::callAsync ([saved]()
        {
            if (auto& host = sy99LibrarySyncHost(); host.finishAuto64OnUi != nullptr)
                host.finishAuto64OnUi (saved);
        });
    }

    void advanceAuto64AfterSlotPair (Sy99AutoSync64Session& s) noexcept
    {
        if (s.currentMm >= kSy99AutoSyncInternalVoiceCount - 1)
        {
            const File saved = saveAutoSync64CombinedCapture (s.accumulator);
            finishAuto64CaptureOnUi (saved);
            return;
        }

        ++s.currentMm;
        s.subStep = Sy99AutoSync64SubStep::wait8101;
        s.retry0040Pending = false;
        sy99Sym7ScheduleGapFromLastTx (s.pacing, kSym7VcInterRequestDelayMs);
        sy99LibrarySyncLogTiming (s.pacing,
                                  sy99AutoSync64SlotLabel (s.currentMm - 1) + " -> "
                                      + sy99AutoSync64SlotLabel (s.currentMm));
    }

    void processAuto64Slot (const juce::Array<juce::MidiMessage>& batch, bool receivedOk) noexcept
    {
        auto& s = sy99AutoSync64Session();

        if (s.handlingRxSlot)
            return;

        struct RxSlotGuard
        {
            bool& flag;
            RxSlotGuard (bool& f) : flag (f) { flag = true; }
            ~RxSlotGuard() { flag = false; }
        };

        RxSlotGuard guard (s.handlingRxSlot);

        const int mm = s.currentMm;

        if (! receivedOk)
        {
            if (s.subStep == Sy99AutoSync64SubStep::wait8101)
            {
                juce::Logger::writeToLog ("[AutoSync64] WARN timeout 8101 "
                                          + sy99AutoSync64SlotLabel (mm) + " — skip slot");
                advanceAuto64AfterSlotPair (s);
                sy99LibrarySyncRequestUiRefresh();
                return;
            }

            if (! s.retry0040Pending)
            {
                s.retry0040Pending = true;
                juce::Logger::writeToLog ("[AutoSync64] WARN timeout 0040 "
                                          + sy99AutoSync64SlotLabel (mm) + " — retry");
                sy99Sym7ScheduleGapFromLastTx (s.pacing, kSym7IntraSlot0040DelayMs);
                sy99LibrarySyncRequestUiRefresh();
                return;
            }

            juce::Logger::writeToLog ("[AutoSync64] WARN 0040 missing "
                                      + sy99AutoSync64SlotLabel (mm) + " — 8101-only");
            advanceAuto64AfterSlotPair (s);
            sy99LibrarySyncRequestUiRefresh();
            return;
        }

        sy99Sym7RecordRxLatency (s.pacing);

        if (s.subStep == Sy99AutoSync64SubStep::wait8101)
        {
            if (! sy99BatchContainsLmTag (batch, "8101VC"))
            {
                juce::Logger::writeToLog ("[AutoSync64] WARN unexpected RX (expected 8101VC) mm="
                                          + juce::String::toHexString (mm));
                return;
            }

            for (const auto& m : batch)
                s.accumulator.add (m);

            sy99LibrarySyncWritePreviewSlot (Sy99LibraryContentPage::internalVoices,
                                             mm,
                                             sy99PreviewNameFromSysexMessages (batch));
            s.stepsCompleted += (int) batch.size();
            s.loadedCount = s.accumulator.size();
            s.subStep = Sy99AutoSync64SubStep::wait0040;
            s.retry0040Pending = false;
            sy99Sym7ScheduleGapFromLastTx (s.pacing, kSym7IntraSlot0040DelayMs);
            sy99LibrarySyncLogTiming (s.pacing,
                                      sy99AutoSync64SlotLabel (mm) + " 8101 -> 0040");
            sy99LibrarySyncRequestUiRefresh();
            return;
        }

        if (! sy99BatchContainsLmTag (batch, "0040VC"))
        {
            juce::Logger::writeToLog ("[AutoSync64] WARN unexpected RX (expected 0040VC) mm="
                                      + juce::String::toHexString (mm));
            return;
        }

        for (const auto& m : batch)
            s.accumulator.add (m);

        s.stepsCompleted += (int) batch.size();
        s.loadedCount = s.accumulator.size();
        ++s.paired0040Count;

        YamahaLmVoiceDump::Lm0040VcMinimal p0040;

        if (YamahaLmVoiceDump::parseLm0040VcMinimalFromSysexMessages (batch, p0040)
            && p0040.found0040)
        {
            juce::Logger::writeToLog ("[AutoSync64] mm="
                                      + juce::String::toHexString (mm).paddedLeft ('0', 2)
                                      + " paired 0040 len="
                                      + juce::String (batch[0].getRawDataSize())
                                      + " "
                                      + YamahaLmVoiceDump::formatLm0040VcMinimalLogLine (p0040));
        }

        sy99LibrarySyncRequestUiRefresh();
        advanceAuto64AfterSlotPair (s);
    }

    void processFullLibrarySlot (const juce::Array<juce::MidiMessage>& batch, bool receivedOk) noexcept
    {
        auto& s = sy99FullLibrarySyncSession();

        if (s.handlingRxSlot)
            return;

        struct RxSlotGuard
        {
            bool& flag;
            RxSlotGuard (bool& f) : flag (f) { flag = true; }
            ~RxSlotGuard() { flag = false; }
        };

        RxSlotGuard guard (s.handlingRxSlot);

        if (receivedOk)
        {
            const int receivedMm = s.currentMm;

            sy99Sym7RecordRxLatency (s.pacing);

            for (const auto& m : batch)
                s.phaseAccumulator.add (m);

            const auto& phase = sy99FullLibrarySyncPhaseAt (s.phaseIndex);

            if (phase.updatesVoicePreview)
            {
                const auto page = libraryContentPageFromId (phase.libraryPage);
                sy99LibrarySyncWritePreviewSlot (page,
                                                 receivedMm,
                                                 sy99PreviewNameFromSysexMessages (batch));
            }
        }

        ++s.requestsCompleted;
        sy99LibrarySyncRequestUiRefresh();

        const auto& phase = sy99FullLibrarySyncPhaseAt (s.phaseIndex);

        if (s.currentMm < phase.mmCount - 1)
        {
            ++s.currentMm;
            sy99Sym7ScheduleGapFromLastTx (s.pacing, phase.interRequestDelayMs);
            sy99LibrarySyncLogTiming (s.pacing,
                                      sy99FullLibrarySyncPhaseLabel (s.phaseIndex, s.currentMm),
                                      ! receivedOk);
            return;
        }

        if (receivedOk && ! s.phaseAccumulator.isEmpty())
        {
            const auto& phase = sy99FullLibrarySyncPhaseAt (s.phaseIndex);

            if (phase.updatesVoicePreview)
            {
                sy99FillLibraryPageNamesFromMessages (
                    libraryContentPageFromId (phase.libraryPage),
                    s.phaseAccumulator,
                    phase.mmCount);
                sy99LibrarySyncRequestUiRefresh();
            }

            sy99LibrarySyncSaveFinishedPhase();
        }

        s.phaseAccumulator.clear();

        const int nextPhaseIndex = sy99LibrarySyncNextPhaseIndex (s.phaseIndex,
                                                                  s.includeExtendedPhases);

        if (nextPhaseIndex < 0)
        {
            endSyncCapture();

            juce::MessageManager::callAsync ([]
            {
                if (auto& host = sy99LibrarySyncHost(); host.finishFullSyncOnUi != nullptr)
                    host.finishFullSyncOnUi();
            });
            return;
        }

        const int finishedPhase = s.phaseIndex;
        s.phaseIndex = nextPhaseIndex;
        s.currentMm = 0;
        const auto& nextPhase = sy99FullLibrarySyncPhaseAt (s.phaseIndex);

        if (nextPhase.updatesVoicePreview)
        {
            const juce::ScopedLock sl (sy99LibrarySyncPreviewLock());
            beginLibraryPagePreview (libraryContentPageFromId (nextPhase.libraryPage),
                                     nextPhase.mmCount);
        }

        const int phaseToPrepare = s.phaseIndex;
        juce::MessageManager::callAsync ([phaseToPrepare]()
        {
            if (auto& host = sy99LibrarySyncHost(); host.preparePhaseOnUi != nullptr)
                host.preparePhaseOnUi (phaseToPrepare);
        });

        sy99Sym7ResetBreatheState (s.pacing);
        sy99Sym7ScheduleGapFromLastTx (s.pacing,
                                       nextPhase.interRequestDelayMs + kSym7PhaseSettleMs);
        sy99LibrarySyncLogTiming (s.pacing,
                                  sy99FullLibrarySyncPhaseLabel (s.phaseIndex, 0),
                                  ! receivedOk);

        juce::ignoreUnused (finishedPhase);
    }
};

inline void sy99LibrarySyncEngineNotifySysex() noexcept
{
    Sy99LibrarySyncEngine::instance().ensureStarted();
    Sy99LibrarySyncEngine::instance().notifySysexReceived();
}

inline void sy99ScheduleLibrarySyncAdvanceOnReceive() noexcept
{
    sy99LibrarySyncEngineNotifySysex();
}

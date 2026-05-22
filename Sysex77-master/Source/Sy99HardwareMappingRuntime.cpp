/*
  ==============================================================================

    Sy99HardwareMappingRuntime.cpp

  ==============================================================================
*/

#include "Sy99HardwareMappingRuntime.h"

#include "LiveSynthState.h"
#include "Sy99ControllerTemplateStore.h"
#include "Sy99HardwareMappingStore.h"
#include "Sy99ParamRegistry.h"

#include <cmath>

namespace
{
    struct ResolvedBinding
    {
        juce::String feedbackPortMatch;
        int channel = 1;
        int cc = 0;
        juce::String paramId;
        int elementIndex = 0;
    };

    juce::CriticalSection gLock;
    juce::Array<ResolvedBinding> gFeedbackBindings;
    Sy99HardwareMappingRuntime::SendSysExFn gSendSysEx;
    Sy99HardwareMappingRuntime::SendCcFn gSendCc;
    std::atomic<juce::uint32> gSuppressFeedbackUntilMs { 0 };

    bool portMatches (const juce::String& portName, const juce::String& pattern)
    {
        if (pattern.trim().isEmpty())
            return true;

        return portName.containsIgnoreCase (pattern.trim());
    }

    bool resolveTemplateSlot (const juce::var& templateVar, int page, int slotIndex,
                              juce::String& paramId, int& elementIndex)
    {
        const juce::var pagesVar = templateVar.getProperty ("pages", {});

        if (! pagesVar.isArray() || page < 0 || page >= pagesVar.getArray()->size())
            return false;

        const juce::var slotsVar = pagesVar.getArray()->getReference (page).getProperty ("slots", {});

        if (! slotsVar.isArray() || slotIndex < 0 || slotIndex >= slotsVar.getArray()->size())
            return false;

        const juce::var slotVar = slotsVar.getArray()->getReference (slotIndex);

        paramId = slotVar.getProperty ("paramId", {}).toString().trim();
        elementIndex = (int) slotVar.getProperty ("elementIndex", 0);

        return paramId.isNotEmpty();
    }

    int ccFromRaw (const Sy99ParamRegistry::Meta& meta, int raw)
    {
        const int span = meta.rawMax - meta.rawMin;

        if (span <= 0)
            return juce::jlimit (0, 127, raw);

        const float norm = (float) (raw - meta.rawMin) / (float) span;
        return juce::jlimit (0, 127, (int) std::lround (norm * 127.0f));
    }

    int rawFromCc (const Sy99ParamRegistry::Meta& meta, int ccValue)
    {
        const int span = meta.rawMax - meta.rawMin;

        if (span <= 0)
            return juce::jlimit (meta.rawMin, meta.rawMax, ccValue);

        const float norm = juce::jlimit (0.0f, 1.0f, (float) ccValue / 127.0f);
        return juce::jlimit (meta.rawMin, meta.rawMax,
                             meta.rawMin + (int) std::lround (norm * (float) span));
    }

    void rebuildFeedbackIndexUnlocked()
    {
        gFeedbackBindings.clear();

        const juce::var allMappings = Sy99HardwareMappings::allToJsonVar();

        if (! allMappings.isArray())
            return;

        for (const auto& mappingVar : *allMappings.getArray())
        {
            const juce::String templateId = mappingVar.getProperty ("templateId", {}).toString();
            const juce::String feedbackPortMatch = mappingVar.getProperty ("feedbackPortMatch", {}).toString();
            const juce::var templateVar = Sy99ControllerTemplates::getById (templateId);
            const juce::var bindingsVar = mappingVar.getProperty ("bindings", {});

            if (templateVar.isVoid() || ! bindingsVar.isArray())
                continue;

            for (const auto& bindingVar : *bindingsVar.getArray())
            {
                ResolvedBinding binding;
                binding.feedbackPortMatch = feedbackPortMatch;
                binding.channel = (int) bindingVar.getProperty ("channel", 0);
                binding.cc = (int) bindingVar.getProperty ("cc", 0);

                if (! resolveTemplateSlot (templateVar,
                                           (int) bindingVar.getProperty ("page", 0),
                                           (int) bindingVar.getProperty ("slotIndex", 0),
                                           binding.paramId, binding.elementIndex))
                    continue;

                gFeedbackBindings.add (binding);
            }
        }
    }

    bool lookupInboundBinding (const juce::String& inputPortName, int channel, int cc,
                               ResolvedBinding& outBinding)
    {
        const juce::var allMappings = Sy99HardwareMappings::allToJsonVar();

        if (! allMappings.isArray())
            return false;

        for (const auto& mappingVar : *allMappings.getArray())
        {
            if (! portMatches (inputPortName, mappingVar.getProperty ("inputPortMatch", {}).toString()))
                continue;

            const juce::var templateVar = Sy99ControllerTemplates::getById (
                mappingVar.getProperty ("templateId", {}).toString());

            const juce::var bindingsVar = mappingVar.getProperty ("bindings", {});

            if (templateVar.isVoid() || ! bindingsVar.isArray())
                continue;

            for (const auto& bindingVar : *bindingsVar.getArray())
            {
                if ((int) bindingVar.getProperty ("channel", 0) != channel)
                    continue;

                if ((int) bindingVar.getProperty ("cc", 0) != cc)
                    continue;

                outBinding.feedbackPortMatch = mappingVar.getProperty ("feedbackPortMatch", {}).toString();
                outBinding.channel = channel;
                outBinding.cc = cc;

                return resolveTemplateSlot (templateVar,
                                            (int) bindingVar.getProperty ("page", 0),
                                            (int) bindingVar.getProperty ("slotIndex", 0),
                                            outBinding.paramId, outBinding.elementIndex);
            }
        }

        return false;
    }
}

namespace Sy99HardwareMappingRuntime
{
    void initializeAtStartup()
    {
        const juce::ScopedLock lock (gLock);
        rebuildFeedbackIndexUnlocked();
    }

    void setHandlers (SendSysExFn sendSysEx, SendCcFn sendCc)
    {
        const juce::ScopedLock lock (gLock);
        gSendSysEx = std::move (sendSysEx);
        gSendCc = std::move (sendCc);
    }

    void reloadFromStore()
    {
        const juce::ScopedLock lock (gLock);
        rebuildFeedbackIndexUnlocked();
    }

    void handleIncomingCc (const juce::String& inputPortName,
                           int channel, int cc, int value, uint8_t sysexDeviceByte)
    {
        if (channel == 1 && (cc == 0 || cc == 32))
            return;

        ResolvedBinding binding;

        {
            const juce::ScopedLock lock (gLock);

            if (! lookupInboundBinding (inputPortName, channel, cc, binding))
                return;
        }

        const Sy99ParamRegistry::Id paramId = Sy99ParamRegistry::idFromString (binding.paramId);

        if (paramId >= Sy99ParamRegistry::Id::Count)
            return;

        const Sy99ParamRegistry::Meta* meta = Sy99ParamRegistry::metaFor (paramId);

        if (meta == nullptr)
            return;

        const int raw = rawFromCc (*meta, value);
        uint8_t frame[9] = {};

        if (! Sy99ParamRegistry::buildLiveParameterSysexFrame (paramId, binding.elementIndex,
                                                               raw, frame, sysexDeviceByte))
            return;

        SendSysExFn sendSysEx;

        {
            const juce::ScopedLock lock (gLock);
            sendSysEx = gSendSysEx;
        }

        if (sendSysEx)
        {
            gSuppressFeedbackUntilMs.store (juce::Time::getMillisecondCounter() + 100);
            sendSysEx (frame);
        }

        Sy99ParamRegistry::mirrorBaselineValue (getLiveSynthState(), paramId,
                                                binding.elementIndex, raw);
    }

    bool sendParameterSysexFrame (const uint8 frame[9]) noexcept
    {
        const juce::ScopedLock lock (gLock);
        SendSysExFn send = gSendSysEx;

        if (send == nullptr || frame == nullptr)
            return false;

        send (frame);
        return true;
    }

    void onLiveParameterSysex (uint8 b3, uint8 b4, uint8 b5, uint8 b6, uint8 b8)
    {
        if (juce::Time::getMillisecondCounter() < gSuppressFeedbackUntilMs.load())
            return;

        int elementIndex = 0;
        const Sy99ParamRegistry::Meta* meta = Sy99ParamRegistry::findByLiveAddress (b3, b4, b5, b6,
                                                                                    elementIndex);

        if (meta == nullptr)
            return;

        const juce::String paramId = Sy99ParamRegistry::metaIdToString (meta->id);
        SendCcFn sendCc;
        juce::Array<ResolvedBinding> feedbackCopy;

        {
            const juce::ScopedLock lock (gLock);

            for (const auto& binding : gFeedbackBindings)
                if (binding.paramId.equalsIgnoreCase (paramId)
                    && binding.elementIndex == elementIndex)
                    feedbackCopy.add (binding);

            sendCc = gSendCc;
        }

        if (! sendCc || feedbackCopy.isEmpty())
            return;

        const int ccValue = ccFromRaw (*meta, (int) b8);

        for (const auto& binding : feedbackCopy)
            sendCc (binding.channel, binding.cc, ccValue, binding.feedbackPortMatch);
    }
}

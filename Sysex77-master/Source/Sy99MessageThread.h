/*
  ==============================================================================

    Sy99MessageThread.h
    Run JUCE UI / gLiveSynthState mutations on the message thread from Param API.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <memory>

template <typename Fn>
auto sy99RunOnMessageThread (Fn&& fn, int timeoutMs = 5000)
{
    using Result = decltype (fn());

    struct Shared
    {
        Result result {};
        bool done = false;
        juce::WaitableEvent event;
    };

    auto shared = std::make_shared<Shared>();

    juce::MessageManager::callAsync ([shared, fn = std::forward<Fn> (fn)]() mutable
    {
        shared->result = fn();
        shared->done = true;
        shared->event.signal();
    });

    if (! shared->event.wait (timeoutMs))
    {
        juce::Logger::writeToLog ("[ParamAPI] sy99RunOnMessageThread timeout after "
                                  + juce::String (timeoutMs) + " ms");
    }

    return shared->result;
}

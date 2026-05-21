/*
  ==============================================================================

    Sy99ControllerTemplateStore.h
    JSON persistence for MIDI controller parameter templates (React CMS).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace Sy99ControllerTemplates
{
    void initializeAtStartup();

    juce::File templatesJsonFile();

    /** Full template objects. */
    juce::var allToJsonVar();

    /** Single template or void if missing. */
    juce::var getById (const juce::String& id);

    bool createTemplate (const juce::String& id, const juce::var& body, juce::String& errorOut);

    bool upsertTemplate (const juce::String& id, const juce::var& body, juce::String& errorOut);
}

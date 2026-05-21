/*
  ==============================================================================

    Sy99HardwareMappingStore.h
    JSON persistence for HardwareMapping (MIDI CC ↔ ControllerTemplate slots).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace Sy99HardwareMappings
{
    void initializeAtStartup();

    juce::File mappingsJsonFile();

    juce::var allToJsonVar();

    juce::var getByDeviceId (const juce::String& deviceId);

    bool createMapping (const juce::String& deviceId, const juce::var& body, juce::String& errorOut);

    bool upsertMapping (const juce::String& deviceId, const juce::var& body, juce::String& errorOut);
}

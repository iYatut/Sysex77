/*
  ==============================================================================

    Sy99MasterCatalog.h
    Master parameter catalog from manual (sy99_master_catalog.json).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace Sy99MasterCatalog
{
    struct Entry
    {
        juce::String id;
        juce::String tag;
        juce::String uiLabel;
        juce::String group;
        int          sysexGroup = 0;
        int          nn = 0;
        bool         perElement = false;
        int          elementCount = 1;
        int          sy99LcdPage = 0;
        juce::String addressHex;
        juce::String codec;
        juce::String registryId;
    };

    struct BulkReadSpec
    {
        juce::String frame;
        juce::String field;
        bool         perElement = false;
        /** Inclusive max element index; -1 = no limit (up to 3). */
        int          maxElementIndex = -1;
    };

    struct Binding
    {
        juce::String id;
        juce::String bindStatus;
        juce::String registryId;
        juce::String bulkSource;
        juce::String notes;
        BulkReadSpec bulkRead;
    };

    void ensureLoaded();
    const juce::Array<Entry>& entries();
    juce::String bindStatusFor (const juce::String& paramId);
    bool tryGetBinding (const juce::String& paramId, Binding& out);
    juce::String formatAddressForElement (const Entry& entry, int elementIndex);

    struct Stats
    {
        int totalRows = 0;
        int catalogParams = 0;
        int inDump = 0;
        int confirmed = 0;
        int manualOnly = 0;
        int candidate = 0;
    };

    juce::var statsToJsonVar (const Stats& stats);
} // namespace Sy99MasterCatalog

/*
  ==============================================================================

    Sy99LibraryApi.h
    REST JSON builders for React Library browser (SY-99 captures on disk).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace Sy99LibraryApi
{
    juce::var libraryTreeToJsonVar();
    juce::var libraryPageVoicesFromSlug (const juce::String& pageSlug);
    juce::var libraryVoiceDetailFromSlug (const juce::String& pageSlug, int mm, juce::String& errorOut);

    /** Drop cached voice detail JSON (after sync save or capture rewrite). */
    void invalidateVoiceDetailCache() noexcept;

    struct LibraryRouteParts
    {
        bool valid = false;
        juce::String slug;
        int mm = -1;
        bool isVoiceDetail = false;
    };

    LibraryRouteParts parseLibraryPagesPath (const juce::String& path) noexcept;
}

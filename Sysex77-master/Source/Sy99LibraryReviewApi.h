#pragma once

#include <JuceHeader.h>

namespace Sy99LibraryReviewApi
{
    juce::File libraryReviewsDirPath() noexcept;
    juce::File libraryReviewsMirrorDirPath() noexcept;

    juce::var createReviewFromJsonBody (const juce::var& body, juce::String& errorOut);
    juce::var reviewToJsonVar (const juce::String& reviewId);
    juce::var listReviewsToJsonVar();
    bool deleteReview (const juce::String& reviewId, juce::String& errorOut);

    juce::String extractReviewIdFromPath (const juce::String& path) noexcept;
} // namespace Sy99LibraryReviewApi

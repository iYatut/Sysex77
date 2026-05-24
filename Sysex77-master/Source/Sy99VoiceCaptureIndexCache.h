/*
  ==============================================================================

    Sy99VoiceCaptureIndexCache.h
    Disk manifest (.syx.idx) + RAM LRU for library voice listing (L1) and offsets (L2).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "YamahaLmVoiceDump.h"

struct Sy99CaptureManifestSlot
{
    int mm = 0;
    juce::String name;
    int elmodeRaw = -1;
    int offset = 0;
    int length = 0;
    juce::String tag { "8101VC" };
};

struct Sy99CaptureManifest
{
    juce::int64 sourceModTimeMs = 0;
    juce::String capturePath;
    juce::Array<Sy99CaptureManifestSlot> slots;
    bool built = false;

    int slotCount() const noexcept { return slots.size(); }

    const Sy99CaptureManifestSlot* slotAt (int mm) const noexcept
    {
        for (const auto& s : slots)
            if (s.mm == mm)
                return &s;

        return nullptr;
    }
};

/** Legacy index shape used by JUCE library UI. */
struct Sy99VoiceCaptureIndex
{
    juce::Array<int> offsets;
    juce::Array<int> lengths;
    juce::StringArray names;
    juce::Array<int> elmodeRaws;
    juce::int64 sourceModTimeMs = 0;
    bool built = false;
};

enum class Sy99LibraryStartupState : std::uint8_t
{
    notStarted,
    restoring,
    ready
};

inline Sy99LibraryStartupState& sy99LibraryStartupState() noexcept
{
    static Sy99LibraryStartupState state = Sy99LibraryStartupState::notStarted;
    return state;
}

juce::File sy99CaptureManifestSidecarPath (const juce::File& captureFile) noexcept;

bool sy99WriteCaptureManifestToDisk (const juce::File& captureFile,
                                     const Sy99CaptureManifest& manifest) noexcept;

bool sy99LoadCaptureManifestFromDisk (const juce::File& captureFile,
                                      Sy99CaptureManifest& manifestOut) noexcept;

bool sy99WriteCaptureManifestFromMessages (const juce::Array<juce::MidiMessage>& messages,
                                           const juce::File& captureFile) noexcept;

bool sy99WriteCaptureManifestFromScan (const juce::File& captureFile) noexcept;

/** Repair legacy captures: one slot per mm (last F0…F7 at max offset). expectedMmCount 0 = infer. */
bool sy99WriteCaptureManifestFromScanDeduped (const juce::File& captureFile,
                                              int expectedMmCount = 0) noexcept;

void sy99CopyManifestToVoiceIndex (const Sy99CaptureManifest& manifest,
                                   Sy99VoiceCaptureIndex& indexOut) noexcept;

const Sy99CaptureManifest* sy99GetCaptureManifest (const juce::File& captureFile,
                                                   bool allowRepairScan = true) noexcept;

void sy99InvalidateCaptureManifest (const juce::File& captureFile) noexcept;

void sy99InvalidateAllCanonicalCaptureManifests() noexcept;

void sy99LoadManifestSlotNamesIntoArray (const juce::File& captureFile,
                                         juce::StringArray& namesOut,
                                         int slotCount) noexcept;

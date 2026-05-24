/*
  Shared BankClick tail: ingest from .syx + UI/MIDI side effects.
  Separate header to avoid VoicesTableModel <-> Sy99Lazy0040Fetch include cycle.
*/

#pragma once

#include "Sy99Lazy0040Fetch.h"
#include "VoicesTableModel.h"

inline void sy99FinishBankVoiceSlotOpen (Sy99LibraryContentPage page,
                                         int mm,
                                         const juce::String& notifyName,
                                         bool parsed) noexcept
{
    if (parsed || notifyName.isNotEmpty())
    {
        bankSelectedVoiceName = notifyName;
        bankSelectedVoiceNameValue.setValue (juce::var (bankSelectedVoiceName));
    }

    if (! sy99ShouldDeferBankClickEditorApply())
        bankVoiceSlotApplyTrigger.setValue (++bankVoiceSlotApplyNonce);

    if (auto& highlight = libraryVoiceHighlightCallback(); highlight != nullptr)
    {
        if (page == Sy99LibraryContentPage::internalVoices)
            highlight (mm);
        else
            selectLibraryPageSlotExclusive (mm / 16, mm % 16);
    }

    if (auto& opened = libraryVoiceOpenedCallback(); opened != nullptr)
        opened (mm, notifyName);

    if (! libraryVoiceSuppressProgramChangeSend())
    {
        if (auto& programChange = libraryVoiceProgramChangeCallback(); programChange != nullptr)
            programChange (mm);
    }

    sy99MaybeRequestBankClick0040Fetch (mm,
                                        sy99VoiceBulkMemoryTypeForContentPage (page));

    scheduleEditorSlotApplyGraceClear();
}

inline void sy99OpenVoiceSlotFromCapture (Sy99LibraryContentPage page,
                                          int mm,
                                          const juce::File& capture,
                                          const juce::Array<int>& offsets,
                                          const juce::Array<int>& lengths,
                                          const juce::StringArray* manifestNames) noexcept
{
    ++sy99BankClick0040FetchGeneration();

    const bool parsed = ingestLm8101FromBankVoiceSlotAndLog (mm, capture, offsets, lengths);

    if (parsed)
        getLiveSynthState().clearLiveParam9Overrides();

    juce::String notifyName;

    if (parsed && getLiveSynthState().lmVoiceName[0] != '\0')
        notifyName = juce::String (getLiveSynthState().lmVoiceName).trimEnd();
    else if (manifestNames != nullptr && mm < manifestNames->size())
        notifyName = manifestNames->getReference (mm).trimEnd();
    else
        notifyName = libraryPageSlotCode (mm);

    sy99FinishBankVoiceSlotOpen (page, mm, notifyName, parsed);
}

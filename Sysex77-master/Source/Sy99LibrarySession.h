/*
  ==============================================================================

    Sy99LibrarySession.h
    Virtual "SY-99" library entry — one UI bank backed by canonical AUTOSYNC captures.

  ==============================================================================
*/

#pragma once

#include "LiveSynthState.h"
#include "Sy99BulkLibraryCapture.h"
#include "VoicesTableModel.h"
#include "YamahaLmVoiceDump.h"

inline constexpr const char* kSy99LibrarySessionBankLabel = "SY-99";

/** Canonical stable capture prefixes (no timestamp suffix). */
inline const char* const kSy99CanonicalCapturePrefixes[] =
{
    "AUTOSYNC-VC-INT",
    "AUTOSYNC-VC-P2",
    "AUTOSYNC-VC-P3",
    "AUTOSYNC-MU-INT",
    "AUTOSYNC-MU-P2",
    "AUTOSYNC-VOICE64"
};

inline bool sy99IsCanonicalAutoloadCaptureFileName (const juce::String& fileName) noexcept
{
    if (! fileName.endsWithIgnoreCase (".syx"))
        return false;

    const juce::String base = fileName.upToLastOccurrenceOf (".", false, false);

    for (const char* prefix : kSy99CanonicalCapturePrefixes)
    {
        if (base.equalsIgnoreCase (prefix))
            return true;
    }

    return false;
}

inline bool sy99IsSy99LibrarySessionBankLabel (const juce::String& bankLabel) noexcept
{
    return bankLabel.equalsIgnoreCase (kSy99LibrarySessionBankLabel);
}

/** Persisted UI restore (loaded/saved from Config settings). */
inline int& libraryPersistedContentPageId() noexcept
{
    static int pageId = 0;
    return pageId;
}

inline int& libraryPersistedSlotMm() noexcept
{
    static int mm = -1;
    return mm;
}

inline int64 sy99NewestCanonicalCaptureMtimeMs() noexcept
{
    int64 newest = 0;

    for (const char* prefix : kSy99CanonicalCapturePrefixes)
    {
        const juce::File f = sy99StableSyncCapturePath (prefix);

        if (f.existsAsFile())
            newest = juce::jmax (newest, f.getLastModificationTime().toMilliseconds());
    }

    return newest;
}

inline void libraryLoadPersistedSelection (juce::PropertiesFile* settings) noexcept
{
    if (settings == nullptr)
        return;

    libraryPersistedContentPageId() = settings->getIntValue ("libraryLastPage", 0);
    libraryPersistedSlotMm() = settings->getIntValue ("libraryLastSlotMm", -1);
}

inline void librarySavePersistedSelection (juce::PropertiesFile* settings) noexcept
{
    if (settings == nullptr)
        return;

    int mm = -1;

    if (libraryPageIsMulti (libraryContentPage()))
    {
        mm = bankSelectedVoiceIndex;
    }
    else if (bankSelectedVoiceIndex >= 0)
    {
        mm = bankSelectedVoiceIndex;
    }

    settings->setValue ("libraryLastPage", (int) libraryContentPage());
    settings->setValue ("libraryLastSlotMm", mm);

    const int64 syncMtime = sy99NewestCanonicalCaptureMtimeMs();

    if (syncMtime > 0)
        settings->setValue ("libraryLastSyncTime", syncMtime);
}

inline juce::PropertiesFile* sy99UserSettingsFile() noexcept
{
    static juce::ApplicationProperties props;
    static bool initialised = false;

    if (! initialised)
    {
        juce::PropertiesFile::Options options;
        options.applicationName = ProjectInfo::projectName;
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName = juce::File::getSpecialLocation (
            juce::File::userApplicationDataDirectory)
            .getChildFile ("SYSEX77").getFullPathName();
        options.storageFormat = juce::PropertiesFile::storeAsXML;
        props.setStorageParameters (options);
        initialised = true;
    }

    return props.getUserSettings();
}

inline void libraryLoadPersistedSelectionFromDisk() noexcept
{
    libraryLoadPersistedSelection (sy99UserSettingsFile());
}

inline void librarySavePersistedSelectionToDisk() noexcept
{
    librarySavePersistedSelection (sy99UserSettingsFile());
}

struct Sy99VoiceCaptureIndex
{
    juce::Array<int> offsets;
    juce::Array<int> lengths;
    juce::StringArray names;
    bool built = false;
};

struct Sy99LibrarySession
{
    bool active = false;
    juce::File voiceInternal;
    juce::File voicePreset1;
    juce::File voicePreset2;
    juce::File multiInternal;
    juce::File multiPreset;
    juce::Array<juce::File> allCaptureFiles;
    Sy99VoiceCaptureIndex indexInternal;
    Sy99VoiceCaptureIndex indexPreset1;
    Sy99VoiceCaptureIndex indexPreset2;
};

inline Sy99LibrarySession& sy99LibrarySession() noexcept
{
    static Sy99LibrarySession session;
    return session;
}

inline juce::String sy99CaptureRelativePath (const juce::File& file) noexcept
{
    if (! file.existsAsFile())
        return {};

    return file.getRelativePathFrom (appDirPath).replaceCharacter ('\\', '/');
}

inline juce::Array<juce::File> sy99DiscoverCanonicalSyncCapturesOnDisk() noexcept
{
    juce::Array<juce::File> found;

    for (const char* prefix : kSy99CanonicalCapturePrefixes)
    {
        const juce::File f = sy99StableSyncCapturePath (prefix);

        if (f.existsAsFile())
            found.add (f);
    }

    return found;
}

inline bool sy99BuildVoiceCaptureIndex (const juce::File& bankFile,
                                        Sy99VoiceCaptureIndex& index) noexcept
{
    index.offsets.clear();
    index.lengths.clear();
    index.names.clear();
    index.built = false;

    if (! bankFile.existsAsFile())
        return false;

    juce::MemoryBlock mb;

    if (! bankFile.loadFileAsData (mb))
        return false;

    const auto* const data = static_cast<const juce::uint8*> (mb.getData());
    const size_t total = mb.getSize();

    if (total == 0)
        return false;

    size_t i = 0;

    while (i < total)
    {
        if (data[i] != 0xF0)
        {
            ++i;
            continue;
        }

        const size_t frameStart = i;
        size_t j = i + 1;

        while (j < total && data[j] != 0xF7)
            ++j;

        if (j >= total)
            break;

        const int frameLen = (int) (j - frameStart + 1);

        if (! YamahaLmVoiceDump::frameContainsLmTag (data + frameStart, frameLen, "8101VC"))
        {
            i = j + 1;
            continue;
        }

        juce::String str;
        YamahaLmVoiceDump::Lm8101VcMinimal parsed;

        if (YamahaLmVoiceDump::parseLm8101VcMinimal (data + frameStart, frameLen, parsed)
            && parsed.name[0] != '\0')
        {
            str = juce::String (parsed.name).trimEnd();
        }
        else if (frameStart + 43 <= total)
        {
            for (int a = 0; a < 10; ++a)
            {
                const juce::uint8 c = data[frameStart + 33 + a];
                const char ch = (c >= 0x20 && c < 0x7F) ? (char) c : ' ';
                str += juce::String::charToString ((juce_wchar) ch);
            }

            str = str.trimEnd();
        }

        index.names.add (str);
        index.offsets.add ((int) frameStart);
        index.lengths.add (frameLen);
        i = j + 1;
    }

    index.built = index.offsets.size() > 0;
    return index.built;
}

inline void sy99InvalidateSy99LibrarySessionCaches() noexcept
{
    auto& s = sy99LibrarySession();
    s.indexInternal.built = false;
    s.indexPreset1.built = false;
    s.indexPreset2.built = false;
}

inline void sy99RefreshSy99LibrarySessionFromDisk() noexcept
{
    auto& s = sy99LibrarySession();
    s.allCaptureFiles = sy99DiscoverCanonicalSyncCapturesOnDisk();

    s.voiceInternal = sy99StableSyncCapturePath ("AUTOSYNC-VC-INT");
    s.voicePreset1  = sy99StableSyncCapturePath ("AUTOSYNC-VC-P2");
    s.voicePreset2  = sy99StableSyncCapturePath ("AUTOSYNC-VC-P3");
    s.multiInternal = sy99StableSyncCapturePath ("AUTOSYNC-MU-INT");
    s.multiPreset   = sy99StableSyncCapturePath ("AUTOSYNC-MU-P2");

    if (! s.voiceInternal.existsAsFile())
        s.voiceInternal = sy99StableSyncCapturePath ("AUTOSYNC-VOICE64");

    s.active = s.voiceInternal.existsAsFile()
               || s.voicePreset1.existsAsFile()
               || s.voicePreset2.existsAsFile()
               || s.multiInternal.existsAsFile()
               || s.multiPreset.existsAsFile();

    sy99InvalidateSy99LibrarySessionCaches();
}

inline juce::File sy99CaptureFileForLibraryPage (Sy99LibraryContentPage page) noexcept
{
    const auto& s = sy99LibrarySession();

    switch (page)
    {
        case Sy99LibraryContentPage::preset1Voices:  return s.voicePreset1;
        case Sy99LibraryContentPage::preset2Voices:  return s.voicePreset2;
        case Sy99LibraryContentPage::multiInternal:  return s.multiInternal;
        case Sy99LibraryContentPage::multiPreset:   return s.multiPreset;
        case Sy99LibraryContentPage::internalVoices:
        default:                                     return s.voiceInternal;
    }
}

inline Sy99VoiceCaptureIndex* sy99VoiceCaptureIndexForPage (Sy99LibraryContentPage page) noexcept
{
    auto& s = sy99LibrarySession();

    switch (page)
    {
        case Sy99LibraryContentPage::preset1Voices: return &s.indexPreset1;
        case Sy99LibraryContentPage::preset2Voices: return &s.indexPreset2;
        case Sy99LibraryContentPage::internalVoices:
        default:                                    return &s.indexInternal;
    }
}

inline Sy99VoiceCaptureIndex* sy99EnsureVoiceCaptureIndex (Sy99LibraryContentPage page) noexcept
{
    if (libraryPageIsMulti (page))
        return nullptr;

    const juce::File capture = sy99CaptureFileForLibraryPage (page);

    if (! capture.existsAsFile())
        return nullptr;

    auto* index = sy99VoiceCaptureIndexForPage (page);

    if (index == nullptr)
        return nullptr;

    if (! index->built)
        sy99BuildVoiceCaptureIndex (capture, *index);

    return index->built ? index : nullptr;
}

inline void sy99RefreshSy99LibraryNamesFromSession (bool extended = true) noexcept
{
    sy99RefreshLibraryNamesFromSyncCaptures (sy99LibrarySession().allCaptureFiles, extended);
}

inline void sy99CopyVoiceIndexToInternalArrays (const Sy99VoiceCaptureIndex& index) noexcept
{
    arrayListVoices.clear();
    voiceSysexFileOffsets.clear();
    voiceSysexFileLengths.clear();

    for (int i = 0; i < index.names.size(); ++i)
        arrayListVoices.add (index.names[i]);

    voiceSysexFileOffsets.addArray (index.offsets);
    voiceSysexFileLengths.addArray (index.lengths);
}

/** Fill A–D grid slot names for the active library page from canonical captures. */
inline void sy99EnsureLibraryPageSlotNames (Sy99LibraryContentPage page) noexcept
{
    if (! sy99LibrarySession().active)
        return;

    if (libraryPageIsMulti (page))
    {
        const juce::File capture = sy99CaptureFileForLibraryPage (page);

        if (capture.existsAsFile())
            sy99LoadLibraryPageNamesFromCaptureFile (capture, page, kSy99LibraryMultiSlotCount);

        return;
    }

    auto* index = sy99EnsureVoiceCaptureIndex (page);

    if (index == nullptr)
        return;

    if (page == Sy99LibraryContentPage::internalVoices)
    {
        sy99CopyVoiceIndexToInternalArrays (*index);
        return;
    }

    auto& slots = libraryPageSlotNames (page);
    slots.clear();

    for (int i = 0; i < index->names.size(); ++i)
        slots.add (index->names[i]);

    while (slots.size() < kSy99LibraryVoiceSlotCount)
        slots.add (juce::String());
}

/** Open editor from SY-99 capture for VOICE/PRE; Multi = highlight + recall only. */
inline void selectLibrarySlotWithEditor (Sy99LibraryContentPage page, int mm) noexcept
{
    if (mm < 0)
        return;

    if (libraryPageIsMulti (page))
    {
        selectLibraryPageSlotExclusive (0, mm);

        if (! libraryVoiceSuppressProgramChangeSend())
        {
            if (auto& programChange = libraryVoiceProgramChangeCallback(); programChange != nullptr)
                programChange (mm);
        }

        return;
    }

    if (! sy99LibrarySession().active)
        return;

    auto* index = sy99EnsureVoiceCaptureIndex (page);

    if (index == nullptr || mm >= index->offsets.size())
        return;

    const juce::File capture = sy99CaptureFileForLibraryPage (page);

    if (page == Sy99LibraryContentPage::internalVoices)
    {
        sy99CopyVoiceIndexToInternalArrays (*index);
        bankSelected = kSy99LibrarySessionBankLabel;
    }

    bankSelectedVoiceIndex = mm;
    clearEditorPatchDirty();

    const bool parsed = ingestLm8101FromBankVoiceSlotAndLog (mm, capture,
                                                             index->offsets, index->lengths);

    if (parsed)
        getLiveSynthState().clearLiveParam9Overrides();

    juce::String notifyName;

    if (parsed && getLiveSynthState().lmVoiceName[0] != '\0')
        notifyName = juce::String (getLiveSynthState().lmVoiceName).trimEnd();
    else if (mm < index->names.size())
        notifyName = index->names[mm].trimEnd();
    else
        notifyName = libraryPageSlotCode (mm);

    if (parsed || notifyName.isNotEmpty())
    {
        bankSelectedVoiceName = notifyName;
        bankSelectedVoiceNameValue.setValue (juce::var (bankSelectedVoiceName));
    }

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

    // Voice/PRE browsing is editor-only — no outbound CC0/CC32/PC (see Librairie WRITE SY99 tooltip).

    librarySavePersistedSelectionToDisk();
}

inline bool sy99ShouldUseSy99SlotEditor() noexcept
{
    return sy99LibrarySession().active
           && sy99IsSy99LibrarySessionBankLabel (bankSelected);
}

inline void sy99RestoreLibrarySlotFromPersistence() noexcept
{
    if (! sy99LibrarySession().active)
        return;

    const int pageId = libraryPersistedContentPageId();
    const int mm = libraryPersistedSlotMm();
    const auto page = libraryContentPageFromId ((juce::uint8) juce::jlimit (0, 4, pageId));

    libraryContentPage() = page;

    if (mm < 0)
        return;

    if (libraryPageIsMulti (page))
    {
        if (mm < kSy99LibraryMultiSlotCount)
            selectLibrarySlotWithEditor (page, mm);
    }
    else if (mm < kSy99LibraryVoiceSlotCount)
    {
        selectLibrarySlotWithEditor (page, mm);
    }
}

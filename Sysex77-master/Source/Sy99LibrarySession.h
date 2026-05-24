/*
  ==============================================================================

    Sy99LibrarySession.h
    Virtual "SY-99" library entry — one UI bank backed by canonical AUTOSYNC captures.

  ==============================================================================
*/

#pragma once

#include "LiveSynthState.h"
#include "Sy99BulkLibraryCapture.h"
#include "Sy99VoiceCaptureIndexCache.h"
#include "VoicesTableModel.h"
#include "Sy99BankClickSlot.h"
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

/** Full-sync AUTOSYNC captures — kept on disk for audit, hidden from BANK list. */
inline bool sy99IsHiddenSyncCaptureFileName (const juce::String& fileName) noexcept
{
    if (! fileName.endsWithIgnoreCase (".syx"))
        return false;

    const juce::String base = fileName.upToLastOccurrenceOf (".", false, false);
    return base.startsWithIgnoreCase ("AUTOSYNC-");
}

/** True when captures/ has .syx files but every one is a hidden AUTOSYNC sync artifact. */
inline bool sy99CapturesDirHasOnlyHiddenSyncCaptures() noexcept
{
    const juce::File cap = sy99EnsureLibraryCapturesDir();

    if (! cap.isDirectory())
        return false;

    juce::Array<juce::File> capFiles;
    cap.findChildFiles (capFiles, juce::File::findFiles, false, "*.syx");

    if (capFiles.isEmpty())
        return false;

    for (const auto& f : capFiles)
    {
        if (! sy99IsHiddenSyncCaptureFileName (f.getFileName()))
            return false;
    }

    return true;
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

inline void midiPersistSaveOpenDevices (const juce::StringArray& inputIdentifiers,
                                        const juce::StringArray& outputIdentifiers) noexcept
{
    if (auto* settings = sy99UserSettingsFile())
    {
        settings->setValue ("midiOpenInputIds", inputIdentifiers.joinIntoString ("|"));
        settings->setValue ("midiOpenOutputIds", outputIdentifiers.joinIntoString ("|"));
        settings->saveIfNeeded();
    }
}

inline void midiPersistLoadOpenDeviceIds (juce::StringArray& inputIdentifiers,
                                          juce::StringArray& outputIdentifiers) noexcept
{
    inputIdentifiers.clear();
    outputIdentifiers.clear();

    if (auto* settings = sy99UserSettingsFile())
    {
        inputIdentifiers.addTokens (settings->getValue ("midiOpenInputIds", ""), "|", "");
        outputIdentifiers.addTokens (settings->getValue ("midiOpenOutputIds", ""), "|", "");
        inputIdentifiers.removeEmptyStrings (true);
        outputIdentifiers.removeEmptyStrings (true);
    }
}

inline void midiPersistSaveMonitorOptions (bool showSysExOnly) noexcept
{
    if (auto* settings = sy99UserSettingsFile())
    {
        settings->setValue ("midiMonitorSysExOnly", showSysExOnly ? 1 : 0);
        settings->saveIfNeeded();
    }
}

inline void midiPersistLoadMonitorOptions (bool& showSysExOnly) noexcept
{
    showSysExOnly = false;

    if (auto* settings = sy99UserSettingsFile())
        showSysExOnly = settings->getIntValue ("midiMonitorSysExOnly", 0) != 0;
}

inline void libraryLoadPersistedSelectionFromDisk() noexcept
{
    libraryLoadPersistedSelection (sy99UserSettingsFile());
}

inline void librarySavePersistedSelectionToDisk() noexcept
{
    librarySavePersistedSelection (sy99UserSettingsFile());
}

inline bool sy99BuildVoiceCaptureIndex (const juce::File& bankFile,
                                        Sy99VoiceCaptureIndex& index) noexcept
{
    index = {};

    if (const Sy99CaptureManifest* manifest = sy99GetCaptureManifest (bankFile, true))
    {
        sy99CopyManifestToVoiceIndex (*manifest, index);
        return index.built;
    }

    return false;
}

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
        bankSelectedVoiceIndex = mm;

        if (! libraryVoiceSuppressProgramChangeSend())
        {
            if (auto& programChange = libraryVoiceProgramChangeCallback(); programChange != nullptr)
                programChange (mm);
        }

        librarySavePersistedSelectionToDisk();
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

    sy99OpenVoiceSlotFromCapture (page, mm, capture, index->offsets, index->lengths, &index->names);

    librarySavePersistedSelectionToDisk();
}

inline bool sy99ShouldUseSy99SlotEditor() noexcept
{
    return sy99LibrarySession().active
           && sy99IsSy99LibrarySessionBankLabel (bankSelected);
}

inline bool& libraryPersistedRestoreCompleted() noexcept
{
    static bool done = false;
    return done;
}

inline void sy99RestoreLibrarySlotFromPersistence() noexcept
{
    if (! sy99LibrarySession().active)
        return;

    const int pageId = libraryPersistedContentPageId();
    const int mm = libraryPersistedSlotMm();
    const auto page = libraryContentPageFromId ((juce::uint8) juce::jlimit (0, 4, pageId));

    if (auto& restore = libraryContentPageRestoreCallback(); restore != nullptr)
        restore (page);
    else
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

/** Once per app launch — safe when BankTableModel restore races Librairie ctor. */
inline void sy99TryRestoreLibraryFromPersistenceOnce() noexcept
{
    if (libraryPersistedRestoreCompleted())
        return;

    if (! sy99LibrarySession().active)
        return;

    libraryPersistedRestoreCompleted() = true;
    sy99RestoreLibrarySlotFromPersistence();
}

/*
  ==============================================================================

    VoicesTableModel.h
    Created: 17 Nov 2018 5:25:44pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once

#include <functional>

#include "../JuceLibraryCode/JuceHeader.h"
#include "LiveSynthState.h"
#include "Sy99Lazy0040Fetch.h"
#include "Sy99ParamRegistry.h"

extern juce::StringArray arrayListVoices;
extern juce::StringArray arrayListPreset1Voices;
extern juce::StringArray arrayListPreset2Voices;
extern juce::StringArray arrayListMultiIntVoices;
extern juce::StringArray arrayListMultiPresetVoices;

enum class Sy99LibraryContentPage : uint8
{
    internalVoices = 0,
    preset1Voices,
    preset2Voices,
    multiInternal,
    multiPreset
};

inline Sy99LibraryContentPage& libraryContentPage() noexcept
{
    static Sy99LibraryContentPage page = Sy99LibraryContentPage::internalVoices;
    return page;
}

inline bool libraryPageIsMulti (Sy99LibraryContentPage page) noexcept
{
    return page == Sy99LibraryContentPage::multiInternal
        || page == Sy99LibraryContentPage::multiPreset;
}

/** SY99 internal voice grid: banks A–D × 16 programs (global index 0…63). */
inline constexpr int kSy99LibraryVoiceSlotCount = 64;

/** Multi: 16 performances M1…M16 — one list (not voice-style banks A–D). */
inline constexpr int kSy99LibraryMultiSlotCount = 16;

/** Multi MIDI: CC32=16 internal / 18 preset, CC0=0, PC 64…79 = M1…M16. */
inline constexpr int kSy99MultiPcBase = 64;
inline constexpr int kSy99Cc32MultiInternal = 16;
inline constexpr int kSy99Cc32MultiPreset   = 18;

inline int libraryPageRowsPerBank() noexcept
{
    return libraryPageIsMulti (libraryContentPage()) ? kSy99LibraryMultiSlotCount : 16;
}

inline int sy99PageRowsPerBank (Sy99LibraryContentPage page) noexcept
{
    return libraryPageIsMulti (page) ? kSy99LibraryMultiSlotCount : 16;
}

inline juce::StringArray& libraryPageSlotNames (Sy99LibraryContentPage page) noexcept
{
    switch (page)
    {
        case Sy99LibraryContentPage::preset1Voices:     return arrayListPreset1Voices;
        case Sy99LibraryContentPage::preset2Voices:     return arrayListPreset2Voices;
        case Sy99LibraryContentPage::multiInternal:     return arrayListMultiIntVoices;
        case Sy99LibraryContentPage::multiPreset:       return arrayListMultiPresetVoices;
        case Sy99LibraryContentPage::internalVoices:
        default:                                        return arrayListVoices;
    }
}

inline juce::StringArray& libraryActiveSlotNames() noexcept
{
    return libraryPageSlotNames (libraryContentPage());
}

/** Display label "A1" … "D16" for a global slot (independent of .syx frame count). */
inline String libraryVoiceSy99SlotCode (int globalIdx) noexcept
{
    if (globalIdx < 0 || globalIdx >= kSy99LibraryVoiceSlotCount)
        return {};

    static const char banks[] = "ABCD";
    const int bankIdx = globalIdx / 16;
    const int voiceNo = (globalIdx % 16) + 1;
    return String::charToString (banks[bankIdx]) + String (voiceNo);
}

inline int libraryBankBaseOffset (int bankIndex) noexcept
{
    return bankIndex * libraryPageRowsPerBank();
}

inline int libraryGlobalIdxForBankRow (int bankIndex, int rowInBank) noexcept
{
    return libraryBankBaseOffset (bankIndex) + rowInBank;
}

inline juce::String libraryPageSlotCode (int globalIdx) noexcept
{
    if (libraryPageIsMulti (libraryContentPage()))
        return "M" + juce::String (globalIdx + 1);

    return libraryVoiceSy99SlotCode (globalIdx);
}

inline juce::String libraryListCellTextForBankRow (int bankIndex, int rowInBank) noexcept
{
    if (libraryPageIsMulti (libraryContentPage()) && bankIndex != 0)
        return {};

    const int globalIdx = libraryPageIsMulti (libraryContentPage())
                              ? rowInBank
                              : libraryGlobalIdxForBankRow (bankIndex, rowInBank);
    const juce::String code = libraryPageSlotCode (globalIdx);
    auto& slots = libraryActiveSlotNames();
    juce::String name;

    if (globalIdx >= 0 && globalIdx < slots.size())
        name = slots[globalIdx].trimEnd();

    if (code.isEmpty())
        return name;

    if (name.isEmpty())
        return code;

    return code + "  " + name;
}

/** Shared list-row paint: slot code + name, left-aligned with ellipsis. */
inline void paintLibraryBankListRow (juce::Graphics& g,
                                     int bankIndex,
                                     int rowNumber,
                                     int width,
                                     int height,
                                     bool rowIsSelected,
                                     juce::Colour selectedColour,
                                     juce::Colour altColour) noexcept
{
    if (rowIsSelected)
        g.fillAll (selectedColour);
    else if (rowNumber % 2)
        g.fillAll (altColour);

    g.setColour (juce::LookAndFeel::getDefaultLookAndFeel().findColour (juce::Label::textColourId));
    g.setFont (height * 0.7f);

    const juce::String label = libraryListCellTextForBankRow (bankIndex, rowNumber);

    if (label.isNotEmpty())
        g.drawText (label, 4, 0, width - 8, height, juce::Justification::centredLeft, true);
}

inline juce::String libraryBankHeaderTitle (int bankIndex) noexcept
{
    if (libraryPageIsMulti (libraryContentPage()))
    {
        if (bankIndex != 0)
            return {};

        return libraryContentPage() == Sy99LibraryContentPage::multiPreset
                   ? juce::String ("Multi Preset")
                   : juce::String ("Multi");
    }

    static const char banks[] = "ABCD";
    const char bank = banks[juce::jlimit (0, 3, bankIndex)];

    switch (libraryContentPage())
    {
        case Sy99LibraryContentPage::preset1Voices:
            return juce::String ("Preset1 ") + bank;
        case Sy99LibraryContentPage::preset2Voices:
            return juce::String ("Preset2 ") + bank;
        case Sy99LibraryContentPage::internalVoices:
        default:
            return juce::String ("Bank ") + bank;
    }
}

inline Sy99LibraryContentPage libraryContentPageFromId (uint8 pageId) noexcept
{
    switch (pageId)
    {
        case 1: return Sy99LibraryContentPage::preset1Voices;
        case 2: return Sy99LibraryContentPage::preset2Voices;
        case 3: return Sy99LibraryContentPage::multiInternal;
        case 4: return Sy99LibraryContentPage::multiPreset;
        default: return Sy99LibraryContentPage::internalVoices;
    }
}

inline uint8 libraryContentPageId (Sy99LibraryContentPage page) noexcept
{
    return (uint8) page;
}

//==============================================================================
// [LIBSYNC] slotInBank 0..15, bankOffset 0|16|32|48 → global index 0..63
static inline String libraryVoiceSlotLabel (int globalIdx) noexcept
{
    auto& slots = libraryActiveSlotNames();

    if (globalIdx < 0 || globalIdx >= slots.size())
        return {};

    return slots[globalIdx].trimEnd();
}

inline std::function<void (int)>& libraryVoiceHighlightCallback() noexcept
{
    static std::function<void (int)> cb;
    return cb;
}

/** Single selection across all four bank columns (voices, presets, multi). */
inline void selectLibraryPageSlotExclusive (int bankIndex, int rowInBank) noexcept
{
    const int globalIdx = libraryGlobalIdxForBankRow (bankIndex, rowInBank);

    if (auto& highlight = libraryVoiceHighlightCallback(); highlight != nullptr)
        highlight (globalIdx);
}

inline uint8 sy99VoiceBulkMemoryTypeForContentPage (Sy99LibraryContentPage page) noexcept
{
    switch (page)
    {
        case Sy99LibraryContentPage::preset1Voices: return 0x02;
        case Sy99LibraryContentPage::preset2Voices: return 0x03;
        default:                                    return 0x00;
    }
}

inline uint8 sy99VoiceBulkMemoryTypeForLibraryPage() noexcept
{
    return sy99VoiceBulkMemoryTypeForContentPage (libraryContentPage());
}

/** UI feedback when a library slot is opened in the editor (no MIDI out). */
inline std::function<void (int globalIdx, const juce::String& voiceName)>& libraryVoiceOpenedCallback() noexcept
{
    static std::function<void (int, const juce::String&)> cb;
    return cb;
}

/** SY99 panel → Librairie tab + slot (8101VC/MU bulk dump with byte28 + mm). */
inline std::function<void (Sy99LibraryContentPage page, int mm)>& librarySynthPanelNavigateCallback() noexcept
{
    static std::function<void (Sy99LibraryContentPage, int)> cb;
    return cb;
}

/** Librairie grid click → open editor / recall (set from LibrairiePage). */
inline std::function<void (Sy99LibraryContentPage page, int mm)>& librarySlotWithEditorCallback() noexcept
{
    static std::function<void (Sy99LibraryContentPage, int)> cb;
    return cb;
}

/** Last Bank Select MSB (CC0) seen on channel 1 — used with inbound PC to map slot 0…15 → bank A–D. */
inline int& inboundLibraryBankMsb() noexcept
{
    static int msb = 0;
    return msb;
}

/** Last Bank Select LSB (CC32) seen on channel 1 — SY99 memory type (Internal/PRE1/PRE2/Multi). */
inline int& inboundLibraryBankLsb() noexcept
{
    static int lsb = 0;
    return lsb;
}

/** SY99 CC32 (Bank Select fine) → Librairie page — hardware capture 2026-05-23. */
inline bool sy99LibraryContentPageFromBankLsb (int bankLsb,
                                               Sy99LibraryContentPage& pageOut) noexcept
{
    switch (bankLsb)
    {
        case 0:  pageOut = Sy99LibraryContentPage::internalVoices; return true;
        case 2:  pageOut = Sy99LibraryContentPage::preset1Voices; return true;
        case 5:  pageOut = Sy99LibraryContentPage::preset2Voices; return true;
        case kSy99Cc32MultiInternal:
            pageOut = Sy99LibraryContentPage::multiInternal;
            return true;
        case kSy99Cc32MultiPreset:
            pageOut = Sy99LibraryContentPage::multiPreset;
            return true;
        default: return false;
    }
}

inline int sy99BankLsbForLibraryContentPage (Sy99LibraryContentPage page) noexcept
{
    switch (page)
    {
        case Sy99LibraryContentPage::preset1Voices: return 2;
        case Sy99LibraryContentPage::preset2Voices: return 5;
        case Sy99LibraryContentPage::multiInternal: return kSy99Cc32MultiInternal;
        case Sy99LibraryContentPage::multiPreset:   return kSy99Cc32MultiPreset;
        default: return 0;
    }
}

#include "Sy99LibraryNavigation.h"

/** Last Program Change slot 0…15 on channel 1 (paired with inboundLibraryBankMsb). -1 = never seen. */
inline int& inboundLibraryProgramSlot() noexcept
{
    static int slot = -1;
    return slot;
}

/** Previous inbound PC slot/bank — for bank-wrap when SY99 sends PC=0 without CC0 at A16→B1, etc. */
inline int& inboundLibraryPreviousProgramSlot() noexcept
{
    static int slot = -1;
    return slot;
}

inline int& inboundLibraryPreviousBankMsb() noexcept
{
    static int msb = 0;
    return msb;
}

/** Multi: PC 64…79 → M1…M16 (CC32=18); voice: linear mm from committed state + batch CC0. */
inline int sy99InboundMmFromProgramChange (Sy99LibraryContentPage page,
                                           int programNumber) noexcept
{
    const juce::uint32 nowMs = juce::Time::getMillisecondCounter();
    const auto& batch = libraryInboundNavCcBatch();
    const int cc0Batch = batch.cc0Fresh (nowMs) ? batch.cc0 : -1;
    return sy99InboundVoiceMmFromPc (page, programNumber, cc0Batch, libraryInboundCommittedState());
}

inline int sy99OutboundProgramForLibrarySlot (Sy99LibraryContentPage page, int globalMm) noexcept
{
    if (libraryPageIsMulti (page))
        return juce::jlimit (0, 127, kSy99MultiPcBase + globalMm);

    // B..D: PC 16..63 is direct voice index. A: PC 0..15 is slot (pair with CC0=0).
    if (globalMm >= 16)
        return globalMm;

    return juce::jlimit (0, 15, globalMm);
}

/** Client edit context: bankSelectedVoiceIndex when a bank is loaded and index is in range. */
inline int resolveCurrentEditorVoiceGlobalIdx() noexcept
{
    const int idx = bankSelectedVoiceIndex;

    if (idx < 0)
        return -1;

    if (bankSelected.isEmpty() || arrayListVoices.isEmpty())
        return -1;

    if (idx >= arrayListVoices.size())
        return -1;

    return idx;
}

inline void clearAllLibraryDisplayState() noexcept
{
    arrayListVoices.clear();
    arrayListPreset1Voices.clear();
    arrayListPreset2Voices.clear();
    arrayListMultiIntVoices.clear();
    arrayListMultiPresetVoices.clear();
    libraryContentPage() = Sy99LibraryContentPage::internalVoices;
    resetLibraryRecallContext();
    libraryInboundCommittedState() = {};
}

/** true → send CC0+CC32+PC; false → PC only (same page + mm/16 context). */
inline bool libraryRecallNeedsFullTriple (int globalIdx) noexcept
{
    return libraryOutboundNeedsFullTriple (libraryContentPage(), globalIdx);
}

/** Recall on SY99 (mode chosen in MidiDemo::sendLibraryVoiceRecallToSynth). */
inline std::function<void (int globalIdx)>& libraryVoiceProgramChangeCallback() noexcept
{
    static std::function<void (int)> cb;
    return cb;
}

/** True when recall was queued because MIDI Out was not open yet (startup). */
inline bool& libraryVoiceRecallDeferredToMidiOpen() noexcept
{
    static bool deferred = false;
    return deferred;
}

/** Tab switch: CC0=0 + CC32 without PC (MidiDemo::sendLibraryPageBankSelectToSynth). */
inline std::function<void (Sy99LibraryContentPage page)>& libraryPageBankSelectCallback() noexcept
{
    static std::function<void (Sy99LibraryContentPage)> cb;
    return cb;
}

/** Startup / persist restore: Librairie tab UI + bank select (switchLibraryContentPage). */
inline std::function<void (Sy99LibraryContentPage page)>& libraryContentPageRestoreCallback() noexcept
{
    static std::function<void (Sy99LibraryContentPage)> cb;
    return cb;
}

/** Common tab → Voice tab: full CC0+CC32+PC recall for current editor voice (MidiDemo::sendVoiceTabRecallFromCommonEdit). */
inline std::function<void()>& voiceTabRecallFromCommonCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

/** Skip outbound PC when the selection came from an incoming SY99 Program Change. */
inline bool& libraryVoiceSuppressProgramChangeSend() noexcept
{
    static bool suppress = false;
    return suppress;
}

/** Skip auto slot open after bank reload (sync finish). */
inline bool& libraryVoiceSuppressAutoSlotOpen() noexcept
{
    static bool suppress = false;
    return suppress;
}

/** Delegate SY-99 bank clicks to session path when wired (MidiDemo). */
inline std::function<void (int globalMm)>& librarySy99SessionSlotSelectCallback() noexcept
{
    static std::function<void (int)> cb;
    return cb;
}

inline void sy99OpenVoiceSlotFromCapture (Sy99LibraryContentPage page,
                                          int mm,
                                          const juce::File& capture,
                                          const juce::Array<int>& offsets,
                                          const juce::Array<int>& lengths,
                                          const juce::StringArray* manifestNames = nullptr) noexcept;

/** Ignore inbound PC echo right after we sent recall (CC0+CC32+PC). */
struct LibraryVoiceProgramChangeEchoGuard
{
    int lastGlobalIdx = -1;
    int lastBankMsb = -1;
    int lastBankLsb = -1;
    Sy99LibraryContentPage lastPage = Sy99LibraryContentPage::internalVoices;
    juce::uint32 sentAtMs = 0;

    static constexpr juce::uint32 kWindowMs = 300;
};

inline LibraryVoiceProgramChangeEchoGuard& libraryVoiceProgramChangeEchoGuard() noexcept
{
    static LibraryVoiceProgramChangeEchoGuard g;
    return g;
}

// --- Editor patch dirty / exit gate (phase 1) ---

inline bool& editorPatchDirty() noexcept
{
    static bool dirty = false;
    return dirty;
}

inline std::function<void()>& pendingEditorExitAction() noexcept
{
    static std::function<void()> action;
    return action;
}

/** Blocks tryLeaveEditorContext and tab gate while completing deferred navigation. */
inline bool& suppressEditorExitPrompt() noexcept
{
    static bool suppress = false;
    return suppress;
}

/** Blocks markEditorPatchDirty during applyLiveSynthState / inbound refresh. */
inline bool& suppressEditorPatchDirtyMark() noexcept
{
    static bool suppress = false;
    return suppress;
}

/** After Stop sync: skip exit gate while async slider notifications drain. */
inline bool& editorSyncBaselineGracePeriod() noexcept
{
    static bool grace = false;
    return grace;
}

/** Wired from VoiceCommonPage — refresh OUTSEL / EFSEND toggle buttons after apply. */
inline std::function<void()>& refreshMixerBoundControlsCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

/** Wired from VoiceCommonPage — relayout mixer strip count/width after ELMODE sync. */
inline std::function<void()>& syncCommonMixerLayoutCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

inline void editorDirtyAuditLog (const char* event, const char* reason = "") noexcept
{
    juce::String msg = "[EditorDirty] " + juce::String (event);

    if (reason != nullptr && reason[0] != '\0')
        msg << " reason=" << reason;

    msg << " dirty=" << (int) editorPatchDirty();
    juce::Logger::writeToLog (msg);
}

inline void markEditorPatchDirty() noexcept
{
    if (suppressEditorPatchDirtyMark())
        return;

    editorPatchDirty() = true;
    editorDirtyAuditLog ("mark", "user-edit");
}

inline void markEditorPatchDirtyFromSynth() noexcept
{
    if (suppressEditorPatchDirtyMark())
        return;

    editorPatchDirty() = true;
    editorDirtyAuditLog ("mark", "synth-ingest");
}

/** Pre-fill slots during library sync so banks show live progress. */
inline void beginLibraryPagePreview (Sy99LibraryContentPage page, int slotCount) noexcept
{
    auto& slots = libraryPageSlotNames (page);
    slots.clear();

    for (int i = 0; i < slotCount; ++i)
        slots.add (String());
}

inline void beginAutoSyncVoicePreview() noexcept
{
    beginLibraryPagePreview (Sy99LibraryContentPage::internalVoices, 64);
}

inline void updateLibraryPagePreviewSlot (Sy99LibraryContentPage page,
                                            int mm,
                                            const String& voiceName) noexcept
{
    if (mm < 0)
        return;

    auto& slots = libraryPageSlotNames (page);

    while (slots.size() <= mm)
        slots.add (String());

    slots.set (mm, voiceName.trimEnd());
}

/** Set voice name for slot mm while auto-sync is running (internal voices page). */
inline void updateAutoSyncVoicePreviewSlot (int mm, const String& voiceName) noexcept
{
    updateLibraryPagePreviewSlot (Sy99LibraryContentPage::internalVoices, mm, voiceName);
}

/** List row: slot code + voice name from active library page. */
inline String libraryVoiceListCellText (int globalIdx) noexcept
{
    const String code = libraryPageSlotCode (globalIdx);
    const String name = libraryVoiceSlotLabel (globalIdx);

    if (code.isEmpty())
        return name;

    if (name.isEmpty())
        return code;

    return code + "  " + name;
}

inline void clearEditorPatchDirty() noexcept
{
    if (! editorPatchDirty())
        return;

    editorPatchDirty() = false;
    editorDirtyAuditLog ("clear");
}

/** Wired from MidiDemo to VoicePage::commitEditorSessionToLiveSynthBaseline(). */
inline std::function<void()>& commitEditorSessionToLiveSynthBaselineCallback() noexcept
{
    static std::function<void()> cb;
    return cb;
}

inline bool executeEditorExitActionOnce (const std::function<void()>& action) noexcept
{
    if (! action)
        return true;

    suppressEditorExitPrompt() = true;
    bool completed = false;

    try
    {
        action();
        completed = true;
    }
    catch (...) {}

    suppressEditorExitPrompt() = false;

    if (completed)
        clearEditorPatchDirty();

    return completed;
}

inline void runPendingEditorExitActionOnce() noexcept
{
    auto action = pendingEditorExitAction();
    pendingEditorExitAction() = nullptr;
    executeEditorExitActionOnce (action);
}

inline void scheduleEditorSlotApplyGraceClear() noexcept
{
    editorSyncBaselineGracePeriod() = true;

    juce::MessageManager::callAsync ([]
    {
        suppressEditorPatchDirtyMark() = true;
        clearEditorPatchDirty();
        suppressEditorPatchDirtyMark() = false;
        editorSyncBaselineGracePeriod() = false;
    });
}

inline bool shouldPromptEditorExitGate (int targetGlobalMm = -1) noexcept
{
    if (suppressEditorExitPrompt() || editorSyncBaselineGracePeriod())
        return false;

    if (! editorPatchDirty())
        return false;

    const int current = resolveCurrentEditorVoiceGlobalIdx();

    if (current < 0)
    {
        editorDirtyAuditLog ("gate-skip", "no-open-patch");
        return false;
    }

    if (targetGlobalMm >= 0 && targetGlobalMm == current)
    {
        editorDirtyAuditLog ("gate-skip", "same-slot");
        return false;
    }

    return true;
}

inline bool tryLeaveEditorContext (std::function<void()> onProceed, int targetGlobalMm = -1)
{
    if (! onProceed)
        return true;

    if (! shouldPromptEditorExitGate (targetGlobalMm))
        return executeEditorExitActionOnce (onProceed);

    editorDirtyAuditLog ("gate", "unsaved-patch-or-quit");
    juce::Logger::writeToLog ("[Editor exit gate] dirty=1 — Save re-reads UI into baseline; Apply/Cancel discard navigation or skip baseline");

    pendingEditorExitAction() = std::move (onProceed);

    auto* alert = new AlertWindow (TRANS ("Unsaved editor changes"),
                                    TRANS ("Common/Mixer edits are on the SY99 but the editor baseline was not updated.\n"
                                           "Save updates the app baseline only (no MIDI, no .syx file)."),
                                    AlertWindow::WarningIcon);

    alert->addButton (TRANS ("Save"),  1, KeyPress (KeyPress::returnKey));
    alert->addButton (TRANS ("Apply"), 2);
    alert->addButton (TRANS ("Cancel"), 0, KeyPress (KeyPress::escapeKey));

    alert->enterModalState (true,
        ModalCallbackFunction::create ([alert] (int result)
        {
            std::unique_ptr<AlertWindow> deleter (alert);

            if (result == 0)
            {
                pendingEditorExitAction() = nullptr;
                return;
            }

            if (result == 1)
            {
                if (auto& commit = commitEditorSessionToLiveSynthBaselineCallback(); commit != nullptr)
                    commit();
            }

            runPendingEditorExitActionOnce();
        }),
        true);

    return false;
}

/** Open library voice in editor + optional SY99 recall MIDI (no bulk write). */
static inline void selectLibraryVoiceSlotProceed (int slotInBank, int bankOffset)
{
    const int globalIdx = bankOffset + slotInBank;

    if (bankSelected.equalsIgnoreCase ("SY-99"))
    {
        if (auto& sessionSelect = librarySy99SessionSlotSelectCallback(); sessionSelect != nullptr)
        {
            sessionSelect (globalIdx);
            return;
        }
    }

    bankSelectedVoiceIndex = globalIdx;
    clearEditorPatchDirty();

    juce::File bankFile = libraryBankFileFromName (bankSelected);

    if (bankSelected.equalsIgnoreCase ("SY-99"))
    {
        bankFile = libraryCapturesDirPath().getChildFile ("AUTOSYNC-VC-INT.syx");

        if (! bankFile.existsAsFile())
            bankFile = libraryCapturesDirPath().getChildFile ("AUTOSYNC-VOICE64.syx");
    }

    sy99OpenVoiceSlotFromCapture (Sy99LibraryContentPage::internalVoices,
                                  globalIdx,
                                  bankFile,
                                  voiceSysexFileOffsets,
                                  voiceSysexFileLengths);
}

static inline void selectLibraryVoiceSlot (int slotInBank, int bankOffset)
{
    if (libraryVoiceSuppressProgramChangeSend())
    {
        selectLibraryVoiceSlotProceed (slotInBank, bankOffset);
        return;
    }

    tryLeaveEditorContext ([=]
    {
        selectLibraryVoiceSlotProceed (slotInBank, bankOffset);
    },
                           bankOffset + slotInBank);
}

/** UI thread: Librairie tab + slot after inbound PC (mm already committed on MIDI thread). */
static inline void handleIncomingLibraryProgramChangeUi (Sy99LibraryContentPage page, int globalMm) noexcept
{
    if (auto& nav = librarySynthPanelNavigateCallback(); nav != nullptr)
        nav (page, globalMm);

    if (bankSelected.equalsIgnoreCase ("SY-99"))
        return;

    if (page != Sy99LibraryContentPage::internalVoices)
        return;

    if (bankSelected.isEmpty() || arrayListVoices.isEmpty())
        return;

    if (globalMm >= arrayListVoices.size())
        return;

    if (globalMm == bankSelectedVoiceIndex)
        return;

    const int bankOffset = (globalMm / 16) * 16;
    const int slotInBank = globalMm % 16;

    libraryVoiceSuppressProgramChangeSend() = true;
    selectLibraryVoiceSlot (slotInBank, bankOffset);
    libraryVoiceSuppressProgramChangeSend() = false;
}

/** Returns true when inbound PC matches a recall we just sent (suppress echo). */
inline bool libraryInboundPcIsEcho (Sy99LibraryContentPage page, int mm, int pc) noexcept
{
    auto& echo = libraryVoiceProgramChangeEchoGuard();
    const juce::uint32 now = juce::Time::getMillisecondCounter();

    if (echo.lastGlobalIdx < 0 || now - echo.sentAtMs >= LibraryVoiceProgramChangeEchoGuard::kWindowMs)
        return false;

    if (page == echo.lastPage && mm == echo.lastGlobalIdx)
        return true;

    if (libraryPageIsMulti (page) && page == echo.lastPage
        && pc == kSy99MultiPcBase + echo.lastGlobalIdx)
        return true;

    if (! libraryPageIsMulti (page) && page == echo.lastPage && pc == echo.lastGlobalIdx)
        return true;

    return false;
}

/** MIDI thread: decode mm, commit state, queue UI. */
inline void processInboundLibraryProgramChangeOnMidiThread (int pc, int midiChannel) noexcept
{
    if (midiChannel != 1 || pc < 0)
        return;

    const juce::uint32 nowMs = juce::Time::getMillisecondCounter();
    auto& batch = libraryInboundNavCcBatch();
    auto& committed = libraryInboundCommittedState();

    int cc32 = inboundLibraryBankLsb();

    if (batch.cc32 >= 0)
        cc32 = batch.cc32;

    Sy99LibraryContentPage page = Sy99LibraryContentPage::internalVoices;

    if (! sy99LibraryContentPageFromBankLsb (cc32, page))
    {
        if (cc32 != 0)
            return;

        page = Sy99LibraryContentPage::internalVoices;
    }

    const int cc0Batch = batch.cc0;
    const int prevMm = committed.mm;
    const int mm = sy99InboundVoiceMmFromPc (page, pc, cc0Batch, committed);

    committed.page = page;
    committed.mm = mm;
    libraryRecallContextAfterSend (page, mm);

    if (libraryPageIsMulti (page))
    {
        inboundLibraryPreviousProgramSlot() = prevMm >= 0 ? prevMm : -1;
        inboundLibraryPreviousBankMsb() = 0;
        inboundLibraryBankMsb() = 0;
        inboundLibraryProgramSlot() = mm;
    }
    else
    {
        inboundLibraryPreviousProgramSlot() = prevMm >= 0 ? (prevMm % 16) : -1;
        inboundLibraryPreviousBankMsb() = prevMm >= 0 ? (prevMm / 16) : 0;
        inboundLibraryBankMsb() = mm / 16;
        inboundLibraryProgramSlot() = mm % 16;
    }

    inboundLibraryBankLsb() = cc32;
    batch.cc0 = -1;
    batch.cc32 = -1;

    libraryNavAuditLog ("RX", page, cc0Batch, cc32, pc, prevMm, mm, false, false);

    if (libraryInboundPcIsEcho (page, mm, pc))
    {
        sy99FlushPendingHwRefreshAfterRecall();
        return;
    }

    sy99OnInboundSynthNavigation (mm);

    juce::MessageManager::callAsync ([page, mm]
    {
        handleIncomingLibraryProgramChangeUi (page, mm);
    });
}

/** Legacy entry — prefer processInboundLibraryProgramChangeOnMidiThread. */
static inline void handleIncomingLibraryProgramChange (int programNumber, int midiChannel) noexcept
{
    processInboundLibraryProgramChangeOnMidiThread (programNumber, midiChannel);
}

/** Step through voices in the loaded bank file (0…63). Used from Voice tab ◀ ▶ / arrow keys. */
static inline void stepLibraryVoiceSlot (int delta) noexcept
{
    if (arrayListVoices.isEmpty() || bankSelected.isEmpty() || delta == 0)
        return;

    int idx = bankSelectedVoiceIndex;

    if (idx < 0)
        idx = (delta > 0) ? 0 : (kSy99LibraryVoiceSlotCount - 1);
    else
        idx = (idx + delta + kSy99LibraryVoiceSlotCount) % kSy99LibraryVoiceSlotCount;

    if (bankSelected.equalsIgnoreCase ("SY-99"))
    {
        if (auto& slotOpen = librarySlotWithEditorCallback(); slotOpen != nullptr)
        {
            tryLeaveEditorContext ([=]
            {
                slotOpen (libraryContentPage(), idx);
            },
                                   idx);
            return;
        }
    }

    const int slotInBank = idx % 16;
    const int bankOffset = (idx / 16) * 16;

    tryLeaveEditorContext ([=]
    {
        selectLibraryVoiceSlotProceed (slotInBank, bankOffset);
    },
                           idx);
}

/** Human label for where a 05:64 bulk frame lands on SY99 internal memory. */
static inline String libraryVoiceSy99InternalTarget (int globalIdx) noexcept
{
    if (globalIdx < 0)
        return {};

    static const char banks[] = "ABCD";
    const int bankIdx = jlimit (0, 3, globalIdx / 16);
    const int voiceNo = (globalIdx % 16) + 1;
    return "Bank " + String::charToString (banks[bankIdx]) + " voice " + String (voiceNo);
}

//==============================================================================

class BankATableModel    : public Component

{

public:
    BankATableModel()
    {
        setName ("Bank A");
      //  loadBank();
        sourceListBox.setModel (&sourceModel);
        sourceListBox.setHeaderComponent(std::make_unique<Header>(*this));
        addAndMakeVisible (sourceListBox);
    }

    void resized() override
    {
        sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) / juce::jmax (1, libraryPageRowsPerBank());
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
    };

private:
    //==============================================================================
    struct SourceItemListboxContents  : public ListBoxModel
    {
        int getNumRows() override
        {
            return libraryPageRowsPerBank();
        }
        void listBoxItemDoubleClicked	(int 	row,const MouseEvent &)	override
        {
               Logger::writeToLog("table A double clic");
        }
        void listBoxItemClicked	(int 	row, const MouseEvent &)	 override
        {
            if (auto& slotOpen = librarySlotWithEditorCallback(); slotOpen != nullptr)
            {
                slotOpen (libraryContentPage(), row);
                return;
            }

            if (libraryContentPage() == Sy99LibraryContentPage::internalVoices)
                selectLibraryVoiceSlot (row, 0);
            else if (libraryPageIsMulti (libraryContentPage()))
            {
                selectLibraryPageSlotExclusive (0, row);

                if (! libraryVoiceSuppressProgramChangeSend())
                {
                    if (auto& programChange = libraryVoiceProgramChangeCallback(); programChange != nullptr)
                        programChange (row);
                }
            }
            else
                selectLibraryPageSlotExclusive (0, row);
        }
        
        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            paintLibraryBankListRow (g, 0, rowNumber, width, height, rowIsSelected,
                                     SYColSelected, SYColAlt);
        }
        
        
        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            // for our drag description, we'll just make a comma-separated list of the selected row
            // numbers - this will be picked up by the drag target and displayed in its box.
            StringArray rows;
            
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
                
                return rows.joinIntoString (", ");
                }
    };
    
    struct Header    : public Component
    {
        Header (BankATableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (findColour (Label::textColourId));
            g.drawFittedText (libraryBankHeaderTitle (0), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent&) override
        {
            Logger::writeToLog("VoicesBankA Mouse event");
        }
        
        BankATableModel& owner;
    };
    
    
    //==============================================================================
    
public: void loadBank()
    {
        sourceListBox.updateContent();
        sourceListBox.repaint();
        repaint();
/*
        if(!appDirPath.exists())
        {
            appDirPath.createDirectory();
            
            Logger::writeToLog("Creation du dossier de presets");
            Logger::writeToLog(appDirPath.getFullPathName());
            
        }
        
        //  appDirPath.findChildFiles(BankFiles, TypeOfFileToFind::findFiles, true, "someName");
        
        appDirPath.findChildFiles(BankFiles, File::TypesOfFileToFind::findFiles
                                  , true,"*.syx");
        numRows=BankFiles.size();
        
        //Verifier si il s'agit bien d'une BANK YAMAHA SY
        
        
        // Construction du XML
        XmlElement bankList ("TABLE_DATA");
        
        //Construction du Headers
        // create an inner element..
        XmlElement* chien = new XmlElement ("HEADERS");
        
        // create an inner element..
        XmlElement* poule = new XmlElement ("COLUMN");
        
        poule->setAttribute ("columnId", 1);
        poule->setAttribute ("name", "ID");
        poule->setAttribute ("width", 120);
        
        chien->addChildElement (poule);
        // create an inner element..
        XmlElement* mpoule = new XmlElement ("COLUMN");
        
        mpoule->setAttribute ("columnId", 2);
        mpoule->setAttribute ("name", "BANK");
        mpoule->setAttribute ("width", 120);
        
        chien->addChildElement (mpoule);
        bankList.addChildElement (chien);
        
        
        //Construction des DATA
        XmlElement* xData = new XmlElement("DATA");
        
        for (int i = 0; i < numRows  ; ++i)
        {
            XmlElement* giraffe = new XmlElement ("ITEM");
            giraffe->setAttribute ("ID", i);
            giraffe->setAttribute ("BANK", BankFiles[i].getFileName());
            arrayBank.add( BankFiles[i].getFileName());
            xData->addChildElement (giraffe);
        }
        bankList.addChildElement(xData);
        // dataList = &BankList;
        
        bankList.writeToFile(appDirPath.getFullPathName() + "/Bank.xml", "");
        
  */
    }
    
    
    
    
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    int numRows;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankATableModel)
};

//==============================================================================

//==============================================================================

class BankBTableModel    : public Component

{
    
public:
    BankBTableModel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        setName ("Bank B");
     //   loadBank();
        sourceListBox.setModel (&sourceModel);
        //sourceListBox.setMultipleSelectionEnabled (true);
        sourceListBox.setHeaderComponent(std::make_unique<Header>(*this));
        
        addAndMakeVisible (sourceListBox);
        //     addAndMakeVisible (target);
        
    }
    
    ~BankBTableModel()
    {
    }
    
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        //        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        //        g.setColour (Colours::grey);
        //        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component
        
        //        g.setColour (Colours::white);
        //        g.setFont (14.0f);
        //        g.drawText ("BankTableModel", getLocalBounds(),
        //                    Justification::centred, true);   // draw some placeholder text
    }
    
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        //  auto r = getLocalBounds().reduced (8);
        sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) / juce::jmax (1, libraryPageRowsPerBank());
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
    };
private:
    //==============================================================================
    struct SourceItemListboxContents  : public ListBoxModel
    {
        int getNumRows() override
        {
            if (libraryPageIsMulti (libraryContentPage()))
                return 0;

            return libraryPageRowsPerBank();
        }

        void listBoxItemClicked (int row, const MouseEvent &) override
        {
            if (auto& slotOpen = librarySlotWithEditorCallback(); slotOpen != nullptr)
            {
                slotOpen (libraryContentPage(), row + 16);
                return;
            }

            if (libraryContentPage() == Sy99LibraryContentPage::internalVoices)
                selectLibraryVoiceSlot (row, 16);
            else
                selectLibraryPageSlotExclusive (1, row);
        }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            paintLibraryBankListRow (g, 1, rowNumber, width, height, rowIsSelected,
                                     SYColSelected, SYColAlt);
        }
        
        
        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            // for our drag description, we'll just make a comma-separated list of the selected row
            // numbers - this will be picked up by the drag target and displayed in its box.
            StringArray rows;
            
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
                
                return rows.joinIntoString (", ");
                }
    };
    
    struct Header    : public Component
    {
        Header (BankBTableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (findColour (Label::textColourId));
            g.drawFittedText (libraryBankHeaderTitle (1), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent&) override
        {
            
        }
        
        BankBTableModel& owner;
    };
    
    
    //==============================================================================
    
public:    void loadBank()
    {
        sourceListBox.updateContent();
        sourceListBox.repaint();
        repaint();
        
    }
    
    
    
    
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    int numRows;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankBTableModel)
};

//==============================================================================


//==============================================================================

class BankCTableModel    : public Component

{
    
public:
    BankCTableModel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        setName ("Bank C");
    //    loadBank();
        sourceListBox.setModel (&sourceModel);
        //sourceListBox.setMultipleSelectionEnabled (true);
        sourceListBox.setHeaderComponent(std::make_unique<Header>(*this));
        
        addAndMakeVisible (sourceListBox);
        //     addAndMakeVisible (target);
        
    }
    
    ~BankCTableModel()
    {
    }
    
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        //        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        //        g.setColour (Colours::grey);
        //        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component
        
        //        g.setColour (Colours::white);
        //        g.setFont (14.0f);
        //        g.drawText ("BankTableModel", getLocalBounds(),
        //                    Justification::centred, true);   // draw some placeholder text
    }
    
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        //  auto r = getLocalBounds().reduced (8);
        sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) / juce::jmax (1, libraryPageRowsPerBank());
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
    };
private:
    //==============================================================================
    struct SourceItemListboxContents  : public ListBoxModel
    {
        int getNumRows() override
        {
            if (libraryPageIsMulti (libraryContentPage()))
                return 0;

            return libraryPageRowsPerBank();
        }

        void listBoxItemClicked (int row, const MouseEvent &) override
        {
            if (auto& slotOpen = librarySlotWithEditorCallback(); slotOpen != nullptr)
            {
                slotOpen (libraryContentPage(), row + 32);
                return;
            }

            if (libraryContentPage() == Sy99LibraryContentPage::internalVoices)
                selectLibraryVoiceSlot (row, 32);
            else
                selectLibraryPageSlotExclusive (2, row);
        }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            paintLibraryBankListRow (g, 2, rowNumber, width, height, rowIsSelected,
                                     SYColSelected, SYColAlt);
        }
        
        
        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            // for our drag description, we'll just make a comma-separated list of the selected row
            // numbers - this will be picked up by the drag target and displayed in its box.
            StringArray rows;
            
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
                
                return rows.joinIntoString (", ");
                }
    };
    
    struct Header    : public Component
    {
        Header (BankCTableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (findColour (Label::textColourId));
            g.drawFittedText (libraryBankHeaderTitle (2), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent&) override
        {
            
        }
        
        BankCTableModel& owner;
    };
    
    
    //==============================================================================
    
public:    void loadBank()
    {
        sourceListBox.updateContent();
        sourceListBox.repaint();
        repaint();
    }
    
    
    
    
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    int numRows;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankCTableModel)
};

//==============================================================================

//==============================================================================

class BankDTableModel    : public Component

{
    
public:
    BankDTableModel()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        setName ("Bank D");
//        loadBank();
        sourceListBox.setModel (&sourceModel);
        //sourceListBox.setMultipleSelectionEnabled (true);
        sourceListBox.setHeaderComponent(std::make_unique<Header>(*this));
        
        addAndMakeVisible (sourceListBox);
        //     addAndMakeVisible (target);
        
    }
    
    ~BankDTableModel()
    {
    }
    
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        //        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        //        g.setColour (Colours::grey);
        //        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component
        
        //        g.setColour (Colours::white);
        //        g.setFont (14.0f);
        //        g.drawText ("BankTableModel", getLocalBounds(),
        //                    Justification::centred, true);   // draw some placeholder text
    }
    
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        //  auto r = getLocalBounds().reduced (8);
        sourceListBox.setBoundsInset (BorderSize<int> (0));
        int rowHeight =(sourceListBox.getHeight() - 24) / juce::jmax (1, libraryPageRowsPerBank());
        if (rowHeight < 24)
            rowHeight =24;
        sourceListBox.setRowHeight(rowHeight);
    };
private:
    //==============================================================================
    struct SourceItemListboxContents  : public ListBoxModel
    {
        int getNumRows() override
        {
            if (libraryPageIsMulti (libraryContentPage()))
                return 0;

            return libraryPageRowsPerBank();
        }

        void listBoxItemClicked (int row, const MouseEvent &) override
        {
            if (auto& slotOpen = librarySlotWithEditorCallback(); slotOpen != nullptr)
            {
                slotOpen (libraryContentPage(), row + 48);
                return;
            }

            if (libraryContentPage() == Sy99LibraryContentPage::internalVoices)
                selectLibraryVoiceSlot (row, 48);
            else
                selectLibraryPageSlotExclusive (3, row);
        }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            paintLibraryBankListRow (g, 3, rowNumber, width, height, rowIsSelected,
                                     SYColSelected, SYColAlt);
        }
        
        
        var getDragSourceDescription (const SparseSet<int>& selectedRows) override
        {
            // for our drag description, we'll just make a comma-separated list of the selected row
            // numbers - this will be picked up by the drag target and displayed in its box.
            StringArray rows;
            
            for (int i = 0; i < selectedRows.size(); ++i)
                rows.add (String (selectedRows[i] + 1));
                
                return rows.joinIntoString (", ");
                }
    };
    
    struct Header    : public Component
    {
        Header (BankDTableModel& o)
        : owner (o)
        {
            setSize (0, 30);
        }
        
        void paint (Graphics& g) override
        {
            g.setColour(SYColLabel);
            g.fillAll();
            g.setColour (findColour (Label::textColourId));
            
            g.drawFittedText (libraryBankHeaderTitle (3), getLocalBounds().reduced (20, 0), Justification::centred, 1);
        }
        
        void mouseDown (const MouseEvent&) override
        {
            
        }
        
        BankDTableModel& owner;
    };
    
    
    //==============================================================================
    
public:    void loadBank()
    {
        sourceListBox.updateContent();
        sourceListBox.repaint();
        repaint();
    }
    
    
    
    
    ListBox sourceListBox  { "D+D source", nullptr };
    SourceItemListboxContents sourceModel;
    int numRows;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BankDTableModel)
};

//==============================================================================

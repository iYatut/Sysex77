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

//==============================================================================
// [LIBSYNC] slotInBank 0..15, bankOffset 0|16|32|48 → global index 0..63
static inline String libraryVoiceSlotLabel (int globalIdx) noexcept
{
    if (globalIdx < 0 || globalIdx >= arrayListVoices.size())
        return {};

    return arrayListVoices[globalIdx].trimEnd();
}

inline std::function<void (int)>& libraryVoiceHighlightCallback() noexcept
{
    static std::function<void (int)> cb;
    return cb;
}

/** UI feedback when a library slot is opened in the editor (no MIDI out). */
inline std::function<void (int globalIdx, const juce::String& voiceName)>& libraryVoiceOpenedCallback() noexcept
{
    static std::function<void (int, const juce::String&)> cb;
    return cb;
}

/** Last Bank Select MSB (CC0) seen on channel 1 — used with inbound PC to map slot 0…15 → bank A–D. */
inline int& inboundLibraryBankMsb() noexcept
{
    static int msb = 0;
    return msb;
}

/** Last Bank Select LSB (CC32) seen on channel 1 (logged; SY99 internal recall uses MSB + PC). */
inline int& inboundLibraryBankLsb() noexcept
{
    static int lsb = 0;
    return lsb;
}

/** Last Program Change slot 0…15 on channel 1 (paired with inboundLibraryBankMsb). -1 = never seen. */
inline int& inboundLibraryProgramSlot() noexcept
{
    static int slot = -1;
    return slot;
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

/** Internal bank (MSB 0…3) for which a full CC0+CC32+PC recall was last done from the app.
    -1 = unknown / external — next recall sends the full triple. Not SY99 edit state (not observable in MIDI). */
inline int& libraryRecallContextBankMsb() noexcept
{
    static int msb = -1;
    return msb;
}

inline void resetLibraryRecallContext() noexcept
{
    libraryRecallContextBankMsb() = -1;
}

/** true → send CC0+CC32+PC; false → PC only (same MSB as established context). */
inline bool libraryRecallNeedsFullTriple (int globalIdx) noexcept
{
    if (globalIdx < 0 || globalIdx > 63)
        return true;

    const int ctx = libraryRecallContextBankMsb();
    return ctx < 0 || ctx != (globalIdx / 16);
}

/** Recall on SY99 (mode chosen in MidiDemo::sendLibraryVoiceRecallToSynth). */
inline std::function<void (int globalIdx)>& libraryVoiceProgramChangeCallback() noexcept
{
    static std::function<void (int)> cb;
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

/** Ignore inbound PC echo right after we sent recall (CC0+CC32+PC). */
struct LibraryVoiceProgramChangeEchoGuard
{
    int lastGlobalIdx = -1;
    juce::uint32 sentAtMs = 0;
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

inline void markEditorPatchDirty() noexcept
{
    if (! suppressEditorPatchDirtyMark())
        editorPatchDirty() = true;
}

/** SY99 internal voice grid: banks A–D × 16 programs (global index 0…63). */
inline constexpr int kSy99LibraryVoiceSlotCount = 64;

/** Display label "A1" … "D16" for a global slot (independent of .syx frame count). */
inline String libraryVoiceSy99SlotCode (int globalIdx) noexcept
{
    if (globalIdx < 0 || globalIdx >= kSy99LibraryVoiceSlotCount)
        return {};

    static const char banks[] = "ABCD";
    const int bankIdx = globalIdx / 16;
    const int voiceNo = (globalIdx % 16) + 1;
    return String (banks[bankIdx]) + String (voiceNo);
}

/** List row: slot code + voice name from .syx when present. */
inline String libraryVoiceListCellText (int globalIdx) noexcept
{
    const String code = libraryVoiceSy99SlotCode (globalIdx);
    const String name = libraryVoiceSlotLabel (globalIdx);

    if (code.isEmpty())
        return name;

    if (name.isEmpty())
        return code;

    return code + "  " + name;
}

inline void clearEditorPatchDirty() noexcept
{
    editorPatchDirty() = false;
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

inline bool tryLeaveEditorContext (std::function<void()> onProceed)
{
    if (! onProceed)
        return true;

    if (! editorPatchDirty() || suppressEditorExitPrompt())
        return executeEditorExitActionOnce (onProceed);

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
    bankSelectedVoiceIndex = globalIdx;
    clearEditorPatchDirty();

    const bool parsed = ingestLm8101FromBankVoiceSlotAndLog (globalIdx, bankSelected,
                                         voiceSysexFileOffsets, voiceSysexFileLengths);

    String notifyName;

    if (parsed && getLiveSynthState().lmVoiceName[0] != '\0')
        notifyName = String (getLiveSynthState().lmVoiceName).trimEnd();
    else
        notifyName = libraryVoiceSlotLabel (globalIdx);

    if (parsed || notifyName.isNotEmpty())
    {
        bankSelectedVoiceName = notifyName;
        bankSelectedVoiceNameValue.setValue (juce::var (bankSelectedVoiceName));
    }

    bankVoiceSlotApplyTrigger.setValue (++bankVoiceSlotApplyNonce);

    if (auto& highlight = libraryVoiceHighlightCallback(); highlight != nullptr)
        highlight (globalIdx);

    if (auto& opened = libraryVoiceOpenedCallback(); opened != nullptr)
        opened (globalIdx, notifyName);

    if (! libraryVoiceSuppressProgramChangeSend())
    {
        if (auto& programChange = libraryVoiceProgramChangeCallback(); programChange != nullptr)
            programChange (globalIdx);
    }
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
    });
}

/** SY99 → PC (+ prior CC0 on ch1): recall the matching library slot A–D. */
static inline void handleIncomingLibraryProgramChange (int programNumber, int midiChannel) noexcept
{
    if (midiChannel != 1 || programNumber < 0)
        return;

    if (bankSelected.isEmpty() || arrayListVoices.isEmpty())
        return;

    const int slotInBank = programNumber % 16;
    const int bankOffset = inboundLibraryBankMsb() * 16;
    const int globalFromInbound = bankOffset + slotInBank;

    if (globalFromInbound >= arrayListVoices.size())
        return;

    auto& echo = libraryVoiceProgramChangeEchoGuard();
    const juce::uint32 now = juce::Time::getMillisecondCounter();

    if (echo.lastGlobalIdx >= 0 && now - echo.sentAtMs < 120
        && slotInBank == (echo.lastGlobalIdx % 16)
        && bankOffset == (echo.lastGlobalIdx / 16) * 16)
        return;

    if (globalFromInbound == bankSelectedVoiceIndex)
        return;

    libraryVoiceSuppressProgramChangeSend() = true;
    selectLibraryVoiceSlot (slotInBank, bankOffset);
    libraryVoiceSuppressProgramChangeSend() = false;
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

    const int slotInBank = idx % 16;
    const int bankOffset = (idx / 16) * 16;

    tryLeaveEditorContext ([=]
    {
        selectLibraryVoiceSlotProceed (slotInBank, bankOffset);
    });
}

/** Human label for where a 05:64 bulk frame lands on SY99 internal memory. */
static inline String libraryVoiceSy99InternalTarget (int globalIdx) noexcept
{
    if (globalIdx < 0)
        return {};

    static const char banks[] = "ABCD";
    const int bankIdx = jlimit (0, 3, globalIdx / 16);
    const int voiceNo = (globalIdx % 16) + 1;
    return "Bank " + String (banks[bankIdx]) + " voice " + String (voiceNo);
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
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
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
            
            return 16;
        }
        void listBoxItemDoubleClicked	(int 	row,const MouseEvent &)	override
        {
               Logger::writeToLog("table A double clic");
        }
        void listBoxItemClicked	(int 	row, const MouseEvent &)	 override
        {
            selectLibraryVoiceSlot (row, 0);
        }
        
        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll (SYColSelected);
                else if (rowNumber % 2)
                    g.fillAll (SYColAlt);
                    
                    
                    g.setColour (LookAndFeel::getDefaultLookAndFeel().findColour (Label::textColourId));
                    g.setFont (height * 0.7f);
                    const String label = libraryVoiceListCellText (rowNumber);

                    if (label.isNotEmpty())
                        g.drawFittedText (label, 0, 0, width, height, Justification::centred, 1);
                        
                        
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
            g.drawFittedText (TRANS("Bank A"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
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
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
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
            
            return 16;
        }

        void listBoxItemClicked (int row, const MouseEvent &) override
        {
            selectLibraryVoiceSlot (row, 16);
        }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll (SYColSelected);
                else if (rowNumber % 2)
                    g.fillAll (SYColAlt);
                    
                    
                    g.setColour (LookAndFeel::getDefaultLookAndFeel().findColour (Label::textColourId));
                    g.setFont (height * 0.7f);
                    
                    //    auto text = arrayBank[rowNumber] ;
                    const String label = libraryVoiceListCellText (rowNumber + 16);

                    if (label.isNotEmpty())
                        g.drawFittedText (label, 0, 0, width, height, Justification::centred, 1);
                 //   g.drawFittedText(arrayBank[rowNumber], 0, 0, width, height, Justification::centred, 1);
                /*
                 g.drawText ("Aucunes Banques" + String (rowNumber + 1),
                 5, 0, width, height,
                 Justification::centred, true);
                 */
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
            g.drawFittedText (TRANS("Bank B"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
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
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
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
            
            return 16;
        }

        void listBoxItemClicked (int row, const MouseEvent &) override
        {
            selectLibraryVoiceSlot (row, 32);
        }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll (SYColSelected);
                else if (rowNumber % 2)
                    g.fillAll (SYColAlt);
                    
                    
                    g.setColour (LookAndFeel::getDefaultLookAndFeel().findColour (Label::textColourId));
                    g.setFont (height * 0.7f);
                    
                    const String label = libraryVoiceListCellText (rowNumber + 32);

                    if (label.isNotEmpty())
                        g.drawFittedText (label, 0, 0, width, height, Justification::centred, 1);
                        
                /*
                 g.drawText ("Aucunes Banques" + String (rowNumber + 1),
                 5, 0, width, height,
                 Justification::centred, true);
                 */
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
            g.drawFittedText (TRANS("Bank C"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
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
        int rowHeight =(sourceListBox.getHeight() - 24) /16;
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
            
            return 16;
        }

        void listBoxItemClicked (int row, const MouseEvent &) override
        {
            selectLibraryVoiceSlot (row, 48);
        }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            if (rowIsSelected)
                g.fillAll (SYColSelected);
                else if (rowNumber % 2)
                    g.fillAll (SYColAlt);
                    
                    
                    g.setColour (LookAndFeel::getDefaultLookAndFeel().findColour (Label::textColourId));
                    g.setFont (height * 0.7f);
                    
                    const String label = libraryVoiceListCellText (rowNumber + 48);

                    if (label.isNotEmpty())
                        g.drawFittedText (label, 0, 0, width, height, Justification::centred, 1);
                        
                /*
                 g.drawText ("Aucunes Banques" + String (rowNumber + 1),
                 5, 0, width, height,
                 Justification::centred, true);
                 */
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
            
            g.drawFittedText (TRANS("Bank D"), getLocalBounds().reduced (20, 0), Justification::centred, 1);
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
    //==============================================================================

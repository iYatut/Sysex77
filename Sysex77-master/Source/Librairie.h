/*
 ==============================================================================
 
 Librairie.h
 Created: 13 Nov 2018 11:23:35pm
 Author:  Sébastien Portrait
 
 Notes:
 BANK limité à 64 PATCH pour pouvoir transferer la banque
 
 Objectifs:
 Se rapprocher de la Librairie a la "massive"
 [VOICES/MULTI/SEQ] [DUMP]
 [BANK] [PATCHES]
 
 *Lecture/Sauvegarde des fichiers SYX
 *Lecture/Sauvegarde des fichier MID
 
 Eventualitées
 *Ajout de TAG
 
 ==============================================================================
 */

#pragma once
#include <memory>

#include "BankTableModel.h"
#include "VoicesTableModel.h"
#include "Sy99BulkLibraryCapture.h"


struct LibrairiePage   : public Component,public Button::Listener, private Timer,public ChangeListener, public ChangeBroadcaster
{
    LibrairiePage()
    {
        setOpaque(false);
        addBtAndMakeStyle(btVoice);
        addBtAndMakeStyle(btMulti);
        btMulti.setEnabled(false);
        addBtAndMakeStyle(btSeq);
        btSeq.setEnabled(false);
        addAndMakeVisible(labelInfoLine);
        
        //    labelInfoLine.setJustificationType(Justification::centred);
        addAndMakeVisible(btSend);
        btSend.addListener(this);
        addAndMakeVisible(btReceive);
        addAndMakeVisible(btStop);
        btStop.setVisible(false);
        btStop.setColour(TextButton::ColourIds::buttonColourId, Colours::red);
        btStop.addListener(this);
        //    btReceive.setEnabled(false);
        btReceive.addListener(this);

        // --- Library skeleton (Prompt #12): IMPORT / EXPORT / SEND VOICE / REQUEST VOICE ---
        addAndMakeVisible (btImport);
        btImport.addListener (this);
        addAndMakeVisible (btExport);
        btExport.addListener (this);
        addAndMakeVisible (btSendVoice);
        btSendVoice.setTooltip ("Writes to SY99 internal memory. Clicking voices / Prev / Next only opens in the editor.");
        btSendVoice.addListener (this);
        addAndMakeVisible (btRequestVoice);
        btRequestVoice.addListener (this);

        addAndMakeVisible(bankList);
        bankList.addChangeListener(this);
        
        addAndMakeVisible(voicesListA);
        addAndMakeVisible(voicesListB);
        addAndMakeVisible(voicesListC);
        addAndMakeVisible(voicesListD);
        
        libraryVoiceHighlightCallback() = [this] (int globalIdx)
        {
            highlightLibraryVoiceSlot (globalIdx);
        };

        libraryVoiceOpenedCallback() = [this] (int globalIdx, const String& voiceName)
        {
            labelInfoLine.setText ("Synced: \"" + voiceName.trimEnd()
                                   + "\" (" + libraryVoiceSy99SlotCode (globalIdx) + ")",
                                   dontSendNotification);
        };

        btVoice.setToggleState(true, NotificationType::dontSendNotification);
        
        
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
    }
    ~LibrairiePage()
    {
        btSend.removeListener(this);
        btStop.removeListener(this);
        btReceive.removeListener(this);
        btImport.removeListener(this);
        btExport.removeListener(this);
        btSendVoice.removeListener(this);
        btRequestVoice.removeListener(this);
        bankList.removeChangeListener(this);
        stopTimer();
        
    }
    void mouseDown (const MouseEvent& e) override
    {
        Logger::writeToLog("Librairie mouse event");
    }
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        Logger::writeToLog("Librairie: changeListener");
        voicesListA.loadBank();
        voicesListB.loadBank();
        voicesListC.loadBank();
        voicesListD.loadBank();
        bankSelectedVoiceIndex = -1;
        bankSelectedVoiceName.clear();
    }
    void resized() override
    {
        // const int bankWidth = 140; // Largeur de la table BANK
        int tableWidth;
        tableWidth = (getWidth()-  64)/5;
        
        btVoice.setBounds (tableWidth + 20,10,64,24);
        btMulti.setBounds (tableWidth + 94, 10,64,24);
        btSeq.setBounds (tableWidth + 168, 10,64,24);

        btSendVoice.setBounds   (tableWidth + 244, 10, 96, 24);
        btRequestVoice.setBounds(tableWidth + 344, 10, 96, 24);
        btImport.setBounds      (tableWidth + 444, 10, 64, 24);
        btExport.setBounds      (tableWidth + 512, 10, 64, 24);

        btSend.setBounds (getWidth()-74,10,64,24);
        btReceive.setBounds(getWidth()-152, 10, 74, 24);
        btStop.setBounds(btReceive.getBounds());
        voicesListA.setBounds(tableWidth + 16, 44, tableWidth, getHeight()-44);
        voicesListB.setBounds(tableWidth + 26 + tableWidth, 44, tableWidth, getHeight()-44);
        voicesListC.setBounds(tableWidth + 36 + tableWidth+ tableWidth, 44, tableWidth, getHeight()-44);
        voicesListD.setBounds(tableWidth + 46 + tableWidth+ tableWidth + tableWidth, 44, tableWidth, getHeight()-44);
        
        bankList.setBounds(8, 10, tableWidth, getHeight()-20);
        labelInfoLine.setBounds (8, getHeight() - 22, getWidth() - 16, 18);
        
    }
    //==============================================================================
    void timerCallback() override
    {
        ++timeOut;

        if (requestSysex && ! arraySysex.isEmpty())
        {
            Sy99BulkDumpAnalysis preview;
            preview = analyzeCapturedSysexMessages (arraySysex);

            labelInfoLine.setText ("Receiving… " + preview.makeSummaryLine()
                                   + " (STOP when SY99 shows Completed)",
                                   dontSendNotification);
        }

        const int threshold = bulkCaptureIdleThresholdTicks (arraySysex.size());

        if (timeOut <= threshold)
            return;

        if (arraySysex.isEmpty())
        {
            if (bulkSessionMode == Sy99BulkCaptureSessionMode::continuous)
            {
                timeOut = 0;
                return;
            }

            endBulkCaptureSession (false);
            labelInfoLine.setText ("Dump: no SysEx received — check MIDI in + Bulk Protect",
                                   dontSendNotification);
            return;
        }

        commitBufferedDumpToLibrary();

        if (bulkSessionMode == Sy99BulkCaptureSessionMode::continuous)
        {
            timeOut = 0;
            labelInfoLine.setText ("Saved " + String (bulkSessionSaveCount) + " file(s). "
                                   + bulkCaptureInstructionForKind (bulkCaptureKind),
                                   dontSendNotification);
            return;
        }

        endBulkCaptureSession (true);
    }

    void beginBulkCapture (Sy99BulkCaptureKind kind,
                           Sy99BulkCaptureSessionMode mode) noexcept
    {
        bulkCaptureKind = kind;
        bulkSessionMode = mode;
        bulkSessionSaveCount = 0;

        arraySysex.clear();
        newMessage = false;
        requestSysex = true;
        timeOut = 0;

        btReceive.setEnabled (false);
        btRequestVoice.setEnabled (false);
        btSend.setEnabled (false);
        btStop.setVisible (true);
        startTimer (500);

        labelInfoLine.setText (bulkCaptureInstructionForKind (kind), dontSendNotification);
        Logger::writeToLog ("[BulkRecv] Started kind="
                            + String ((int) kind)
                            + " mode=" + String ((int) mode));
    }

    bool commitBufferedDumpToLibrary() noexcept
    {
        if (arraySysex.isEmpty() || ! arraySysex[0].isSysEx())
            return false;

        Sy99BulkDumpAnalysis analysis;
        const auto preview = analyzeCapturedSysexMessages (arraySysex);
        const String prefix = (bulkCaptureKind == Sy99BulkCaptureKind::voiceSingle)
                                  ? String ("VOICE1")
                                  : preview.suggestFileBaseName();

        const File saved = saveCapturedSysexToLibrary (arraySysex, prefix, &analysis);
        arraySysex.clear();

        if (! saved.existsAsFile())
            return false;

        ++bulkSessionSaveCount;
        bankList.reloadAfterLibraryFileAdded();

        labelInfoLine.setText ("Saved: " + saved.getFileName()
                               + " (" + analysis.makeSummaryLine() + ")",
                               dontSendNotification);
        return true;
    }

    void endBulkCaptureSession (bool hadData) noexcept
    {
        stopTimer();
        requestSysex = false;
        newMessage = false;
        timeOut = 0;
        arraySysex.clear();

        btReceive.setEnabled (true);
        btRequestVoice.setEnabled (true);
        btSend.setEnabled (true);
        btStop.setVisible (false);

        if (bulkSessionMode == Sy99BulkCaptureSessionMode::continuous && bulkSessionSaveCount > 0)
        {
            labelInfoLine.setText ("Receive finished: " + String (bulkSessionSaveCount)
                                   + " file(s) in library",
                                   dontSendNotification);
        }
        else if (! hadData && bulkSessionSaveCount == 0)
        {
            juce::ignoreUnused (hadData);
        }

        Logger::writeToLog ("[BulkRecv] Session ended, saved="
                            + String (bulkSessionSaveCount));
    }

    void showReceiveFromSy99Menu()
    {
        PopupMenu menu;
        menu.addItem (1, "64 Voice Bank  (SY99 menu 05:64 Voice)");
        menu.addItem (2, "Single Voice   (SY99 menu 07:1 Voice)");
        menu.addSeparator();
        menu.addItem (3, "All dumps — continuous (each dump → .syx, STOP when done)");

        menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&btReceive),
                            [this] (int result)
        {
            if (result == 1)
                beginBulkCapture (Sy99BulkCaptureKind::voiceBank64,
                                  Sy99BulkCaptureSessionMode::singleAutoClose);
            else if (result == 2)
                beginBulkCapture (Sy99BulkCaptureKind::voiceSingle,
                                  Sy99BulkCaptureSessionMode::singleAutoClose);
            else if (result == 3)
                beginBulkCapture (Sy99BulkCaptureKind::receiveAll,
                                  Sy99BulkCaptureSessionMode::continuous);
        });
    }
    void buttonClicked (Button* button) override
    {
        Logger::writeToLog("Librairie -> ButtonClicked");
        if(button == &btSend)
        {
            String  bankName =appDirPath.getFullPathName()+"/";
            if (bankSelected.isNotEmpty())
            {
                bankName = bankName + bankSelected;
                Logger::writeToLog(bankName);
                if (! sender.send (adresseOscSendBank, bankSelected)) // [5]
                    Logger::writeToLog ("OSC erreur");;
                labelInfoLine.setText ("Bank send -> " + bankSelected, dontSendNotification);
            }
            else
            {
                labelInfoLine.setText ("No bank selected", dontSendNotification);
            }
        }
        else if (button == &btReceive)
        {
            showReceiveFromSy99Menu();
        }
        else if (button == &btStop)
        {
            const bool hadBuffered = arraySysex.size() > 0;

            if (hadBuffered)
                commitBufferedDumpToLibrary();
            else
                labelInfoLine.setText ("No SysEx captured — check MIDI In port + Bulk Protect OFF",
                                       dontSendNotification);

            endBulkCaptureSession (hadBuffered || bulkSessionSaveCount > 0);
        }
        else if (button == &btImport)
        {
            chooser = std::make_unique<FileChooser> (
                "Import SY99 SysEx file",
                File::getSpecialLocation (File::userDocumentsDirectory),
                "*.syx;*.SYX");
            chooser->launchAsync (FileBrowserComponent::openMode
                                  | FileBrowserComponent::canSelectFiles,
                                  [this] (const FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == File()) return;
                if (! appDirPath.exists()) appDirPath.createDirectory();
                File dest (appDirPath.getFullPathName() + "/" + file.getFileName());
                if (dest.existsAsFile()) dest = dest.getNonexistentSibling();
                if (file.copyFileTo (dest))
                {
                    labelInfoLine.setText ("Imported: " + dest.getFileName(),
                                           dontSendNotification);
                    bankList.reloadAfterLibraryFileAdded();
                }
                else
                {
                    labelInfoLine.setText ("Import failed: " + file.getFileName(),
                                           dontSendNotification);
                }
            });
        }
        else if (button == &btExport)
        {
            if (bankSelected.isEmpty())
            {
                labelInfoLine.setText ("No bank selected to export", dontSendNotification);
                return;
            }
            File src (appDirPath.getFullPathName() + "/" + bankSelected);
            if (! src.existsAsFile())
            {
                labelInfoLine.setText ("Source missing: " + bankSelected,
                                       dontSendNotification);
                return;
            }
            chooser = std::make_unique<FileChooser> (
                "Export bank file",
                File::getSpecialLocation (File::userDocumentsDirectory).getChildFile (bankSelected),
                "*.syx");
            chooser->launchAsync (FileBrowserComponent::saveMode
                                  | FileBrowserComponent::canSelectFiles
                                  | FileBrowserComponent::warnAboutOverwriting,
                                  [this, src] (const FileChooser& fc)
            {
                auto dest = fc.getResult();
                if (dest == File()) return;
                if (src.copyFileTo (dest))
                    labelInfoLine.setText ("Exported -> " + dest.getFullPathName(),
                                           dontSendNotification);
                else
                    labelInfoLine.setText ("Export failed", dontSendNotification);
            });
        }
        else if (button == &btSendVoice)
        {
            if (bankSelected.isEmpty())
            {
                labelInfoLine.setText ("Select a bank file first", dontSendNotification);
                return;
            }

            if (bankSelectedVoiceIndex < 0)
            {
                labelInfoLine.setText ("Click a voice slot (A–D) to open it", dontSendNotification);
                return;
            }

            const String target = libraryVoiceSy99InternalTarget (bankSelectedVoiceIndex);
            const String voiceLabel = libraryVoiceSlotLabel (bankSelectedVoiceIndex);

            AlertWindow::showOkCancelBox (
                AlertWindow::WarningIcon,
                "Write voice to SY99 internal memory?",
                "This overwrites " + target + " on the synthesizer (\"" + voiceLabel + "\").\n\n"
                "Browsing / Prev / Next only opens the voice in the editor and does not send MIDI.\n\n"
                "Continue?",
                "Write to SY99",
                "Cancel",
                this,
                ModalCallbackFunction::create ([this, target, voiceLabel] (int result)
                {
                    if (result != 1)
                    {
                        labelInfoLine.setText ("Write cancelled — \"" + voiceLabel
                                               + "\" still open in editor only",
                                               dontSendNotification);
                        return;
                    }

                    if (! sender.send (OSCAddressPattern (adresseOscSendVoice),
                                       bankSelected, (int32) bankSelectedVoiceIndex))
                    {
                        labelInfoLine.setText ("Write to SY99 failed (OSC)", dontSendNotification);
                        return;
                    }

                    labelInfoLine.setText ("Written to SY99 " + target
                                           + " — select that voice on the synth to play",
                                           dontSendNotification);
                }));
        }
        else if (button == &btRequestVoice)
        {
            beginBulkCapture (Sy99BulkCaptureKind::voiceSingle,
                              Sy99BulkCaptureSessionMode::singleAutoClose);
        }
    }
    
private:
    //       LibrairiePage& owner;
    ToggleButton toggleButton;
    
    
    
    
    // FIN DES XML FONCTIONS
    void addBtAndMakeStyle (TextButton& textButton)
    {
        textButton.setClickingTogglesState(true);
        textButton.setRadioGroupId(77);
        textButton.setColour(TextButton::ColourIds::buttonOnColourId, Colours::red);
        addAndMakeVisible (textButton);
    }

    void highlightLibraryVoiceSlot (int globalIdx) noexcept
    {
        voicesListA.sourceListBox.deselectAllRows();
        voicesListB.sourceListBox.deselectAllRows();
        voicesListC.sourceListBox.deselectAllRows();
        voicesListD.sourceListBox.deselectAllRows();

        if (globalIdx < 0 || globalIdx >= arrayListVoices.size())
            return;

        const int row = globalIdx % 16;

        switch (globalIdx / 16)
        {
            case 0: voicesListA.sourceListBox.selectRow (row); break;
            case 1: voicesListB.sourceListBox.selectRow (row); break;
            case 2: voicesListC.sourceListBox.selectRow (row); break;
            case 3: voicesListD.sourceListBox.selectRow (row); break;
            default: break;
        }
    }
    Label labelInfoLine {"","Test Info Line -- -- -- -- -- -- -- -- -- -- --"};
    TextButton btSend {TRANS("SEND BANK->")};
    TextButton btReceive {TRANS("FROM SY99 <-")};
    TextButton btStop {"STOP"};

    TextButton btImport     {TRANS("IMPORT")};
    TextButton btExport     {TRANS("EXPORT")};
    TextButton btSendVoice  { TRANS ("WRITE SY99") };
    TextButton btRequestVoice {TRANS("REQUEST VOICE")};

    std::unique_ptr<FileChooser> chooser;

    TextButton btVoice {"VOICE"};
    TextButton btMulti {"MULTI"};
    TextButton btSeq{"SEQ"};
    
    //    TableListBox table { {}, this };;
    //   TableListBox tableBank { {}, this };;
    BankATableModel voicesListA;
    BankBTableModel voicesListB;
    BankCTableModel voicesListC;
    BankDTableModel voicesListD;
    BankTableModel bankList;
    
    int rowSelected;
    
    
    int oldSize;
    
    int numRows;
    OSCSender sender;  // [2]

    Sy99BulkCaptureKind bulkCaptureKind { Sy99BulkCaptureKind::voiceBank64 };
    Sy99BulkCaptureSessionMode bulkSessionMode { Sy99BulkCaptureSessionMode::singleAutoClose };
    int bulkSessionSaveCount = 0;
    
};
//==============================================================================

//==============================================================================
/**
 This class shows how to implement a TableListBoxModel to show in a TableListBox.
 */
//==============================================================================

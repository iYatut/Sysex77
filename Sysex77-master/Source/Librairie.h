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
        btSendVoice.addListener (this);
        addAndMakeVisible (btRequestVoice);
        btRequestVoice.addListener (this);

        addAndMakeVisible(bankList);
        bankList.addChangeListener(this);
        
        addAndMakeVisible(voicesListA);
        addAndMakeVisible(voicesListB);
        addAndMakeVisible(voicesListC);
        addAndMakeVisible(voicesListD);
        
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

        btSendVoice.setBounds   (tableWidth + 244, 10, 84, 24);
        btRequestVoice.setBounds(tableWidth + 332, 10, 96, 24);
        btImport.setBounds      (tableWidth + 432, 10, 64, 24);
        btExport.setBounds      (tableWidth + 500, 10, 64, 24);

        btSend.setBounds (getWidth()-74,10,64,24);
        btReceive.setBounds(getWidth()-140, 10, 64, 24);
        btStop.setBounds(btReceive.getBounds());
        voicesListA.setBounds(tableWidth + 16, 44, tableWidth, getHeight()-44);
        voicesListB.setBounds(tableWidth + 26 + tableWidth, 44, tableWidth, getHeight()-44);
        voicesListC.setBounds(tableWidth + 36 + tableWidth+ tableWidth, 44, tableWidth, getHeight()-44);
        voicesListD.setBounds(tableWidth + 46 + tableWidth+ tableWidth + tableWidth, 44, tableWidth, getHeight()-44);
        
        bankList.setBounds(8, 10, tableWidth, getHeight()-20);
        
    }
    //==============================================================================
    void timerCallback() override   // Timer of Sysex DUMP (receive sysex)
    {
        Logger::writeToLog("Timer : " + String(timeOut));
        timeOut ++;
        if(timeOut >20)
        {
            saveDump();
        }
    }
    void saveDump()  //When Sysex is dumped
    {
        stopTimer();
        btReceive.setEnabled(true);
        btSend.setEnabled(true);
        newMessage = false;
        requestSysex = false;
        btStop.setVisible(false);

        if (arraySysex.size() == 0 || ! arraySysex[0].isSysEx())
        {
            labelInfoLine.setText ("Dump: no SysEx received", dontSendNotification);
            return;
        }

        auto stamp = Time::getCurrentTime().formatted ("DUMP-%Y%m%d-%H%M%S");
        File myFile {(appDirPath.getFullPathName() + "/" + stamp + ".syx")};
        if (myFile.existsAsFile())
            myFile = myFile.getNonexistentSibling();

        FileOutputStream fos (myFile);
        for (auto& m : arraySysex)
        {
            Logger::writeToLog(m.getDescription());
            fos.write(m.getRawData(), m.getRawDataSize());
        }
        fos.flush();
        labelInfoLine.setText ("Dump saved: " + myFile.getFileName()
                               + " (" + String (arraySysex.size()) + " msg)",
                               dontSendNotification);
        arraySysex.clear();
        loadBankRequest = true; //Make a better code later!
        sendChangeMessage();
        repaint();
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
        else if(button == &btReceive)
        {
            btReceive.setEnabled(false);
            btSend.setEnabled(false);
            newMessage = false;
            requestSysex = true;
            timeOut=0;
            startTimer(500);
            btStop.setVisible(true);
            labelInfoLine.setText ("Receiving bulk dump…", dontSendNotification);
        }
        else if(button == &btStop)
        {
            saveDump(); //la fonction save dump vérifie la presence de sysex
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
                    loadBankRequest = true;
                    sendChangeMessage();
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
                Logger::writeToLog ("[LIBSYNC] SEND VOICE: no bank selected");
                labelInfoLine.setText ("SEND VOICE: select a bank first", dontSendNotification);
                return;
            }
            if (bankSelectedVoiceIndex < 0)
            {
                Logger::writeToLog ("[LIBSYNC] SEND VOICE: no voice slot selected");
                labelInfoLine.setText ("SEND VOICE: click a voice slot (A–D)", dontSendNotification);
                return;
            }
            if (! sender.send (OSCAddressPattern (adresseOscSendVoice),
                               bankSelected, (int32) bankSelectedVoiceIndex))
            {
                Logger::writeToLog ("[LIBSYNC] SEND VOICE: OSC send failed");
                labelInfoLine.setText ("SEND VOICE: OSC send failed", dontSendNotification);
            }
            else
            {
                labelInfoLine.setText ("SEND VOICE -> slot " + String (bankSelectedVoiceIndex)
                                       + " (" + bankSelected + ")", dontSendNotification);
            }
        }
        else if (button == &btRequestVoice)
        {
            btReceive.setEnabled(false);
            btSend.setEnabled(false);
            newMessage = false;
            requestSysex = true;
            timeOut = 0;
            startTimer (500);
            btStop.setVisible (true);
            labelInfoLine.setText ("Request voice: press DUMP OUT on SY99 → "
                                   "Edit Voice / Voice Common (1 dump)",
                                   dontSendNotification);
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
    Label labelInfoLine {"","Test Info Line -- -- -- -- -- -- -- -- -- -- --"};
    TextButton btSend {TRANS("SEND BANK->")};
    TextButton btReceive {TRANS("RECEIVE BANK <-")};
    TextButton btStop {"STOP"};

    TextButton btImport     {TRANS("IMPORT")};
    TextButton btExport     {TRANS("EXPORT")};
    TextButton btSendVoice  {TRANS("SEND VOICE")};
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
    
};
//==============================================================================

//==============================================================================
/**
 This class shows how to implement a TableListBoxModel to show in a TableListBox.
 */
//==============================================================================

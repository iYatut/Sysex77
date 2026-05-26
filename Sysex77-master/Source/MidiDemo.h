/*
 ==============================================================================
 
 This file is part of the JUCE examples.
 Copyright (c) 2017 - ROLI Ltd.
 
 The code included in this file is provided under the terms of the ISC license
 http://www.isc.org/downloads/software-support-policy/isc-license. Permission
 To use, copy, modify, and/or distribute this software for any purpose with or
 without fee is hereby granted provided that the above copyright notice and
 this permission notice appear in all copies.
 
 THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
 WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
 PURPOSE, ARE DISCLAIMED.
 
 ==============================================================================
 */

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.
 
 BEGIN_JUCE_PIP_METADATA
 
 name:             MidiDemo
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Handles incoming and outcoming midi messages.
 
 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
 juce_audio_processors, juce_audio_utils, juce_core,
 juce_data_structures, juce_events, juce_graphics,
 juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make, xcode_iphone, androidstudio
 
 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1
 
 type:             Component
 mainClass:        MidiDemo
 
 useLocalCopy:     1
 
 END_JUCE_PIP_METADATA
 
 
 *******************************************************************************/
#include "Values.h"
#include "LiveSynthState.h"
#include "EditorPatchDirtyGate.h"
#include "Sy99HardwareMappingRuntime.h"
#include "MidiStreamLogger.h"

// Set to 1 for temporary logs: queue sizes per async drain + max consecutive identical SysEx in batch.
#ifndef SYSEX77_MIDI_MONITOR_DIAGNOSTICS
#define SYSEX77_MIDI_MONITOR_DIAGNOSTICS 0
#endif

static const String adresseOscFoot = "/77Foot";
static const String adresseOscMod = "/77Mod";
static const String adresseOpMode = "/77OpMode";
static const String adresseOscSendBank ="/77SendBank";
static const String adresseOscSendVoice = "/77SendVoice"; // [LIBSYNC] per-voice frame from parsed .syx
static const String adresseOscRepaint = "/77Repaint";
static const String adresseOscParseVoices = "/77ParseVoices";
static const String oscTotalVoiceVolume = "/77TotalVoiceVolume";


static const String oscVoicePan1 = "/77VoicePan1";
static const String oscVoicePan2 = "/77VoicePan2";
static const String oscVoicePan3 = "/77VoicePan3";
static const String oscVoicePan4 = "/77VoicePan4";
static const String oscVoiceGrp1 = "/77VoiceGroupe1";
static const String oscVoiceGrp2 = "/77VoiceGroupe2";
static const String oscVoiceGrp3 = "/77VoiceGroupe3";
static const String oscVoiceGrp4 = "/77VoiceGroupe4";
static const String oscVoicePitch1 = "/77VoicePitch1";
static const String oscVoicePitch2 = "/77VoicePitch2";
static const String oscVoicePitch3 = "/77VoicePitch3";
static const String oscVoicePitch4 = "/77VoicePitch4";
static const String oscVoiceFine1 = "/77VoiceFine1";
static const String oscVoiceFine2 = "/77VoiceFine2";
static const String oscVoiceFine3 = "/77VoiceFine3";
static const String oscVoiceFine4 = "/77VoiceFine4";
static const String oscVoiceFixe1 = "/77VoiceFixe1";
static const String oscVoiceFixe2 = "/77VoiceFixe2";
static const String oscVoiceFixe3 = "/77VoiceFixe3";
static const String oscVoiceFixe4 = "/77VoiceFixe4";
static Array<MidiMessage>    oscMidiMessage;
static const String oscSendMidiMessage = "/77MidiMessage";

static   StringArray  arrayBank;    //la liste des banques
static  StringArray arrayListVoices; // liste des voices
static  StringArray arrayListPreset1Voices;
static  StringArray arrayListPreset2Voices;
static  StringArray arrayListMultiIntVoices;
static  StringArray arrayListMultiPresetVoices;
// [LIBSYNC] Parsed per-voice SysEx bulk frames inside the selected .syx (F0 … F7 each).
static Array<int> voiceSysexFileOffsets;
static Array<int> voiceSysexFileLengths;
static int    bankSelectedVoiceIndex = -1;
static String bankSelectedVoiceName;
static Value  bankSelectedVoiceNameValue;
/** Incremented on each library voice-slot click so Voice tab applies even if name unchanged. */
static Value  bankVoiceSlotApplyTrigger;
static int    bankVoiceSlotApplyNonce = 0;
static const int maxFiles = 512;

static        Array<File> BankFiles; //les fichiers des banques
Array<MidiMessage> arraySysex;
int SYModel =1;
int CommonFoot;
int CommonMod;

bool newMessage = false;
bool requestSysex = false;
static bool doubleClickBank = false;
static    bool bankDeleteKey = false;
static int rowSelectedBank;
bool loadBankRequest = false;
int timeOut = 0;
static String bankSelected;

#include "Sy99Lazy0040Fetch.h"
#include "Sy99InvokeBulkLoad.h"

static Value valueSysexIn; //values from sysex midi in
static bool boolStopReceive; //to shunt the midi in when sending

static int sysexModel;
/** Yamaha device byte in outgoing `43 nn 34 …` payloads. Matches stable ELMODE route in `MidiSysex.h`
    (literal `0x10`) until the user selects another ID in Config. Zero was wrong for SY99 default. */
static uint8 sysexEngine { 0x10 };
static     float fAngle = -90 * (float_Pi / 180.0); //Radiant to draw at 90°
File appDirPath = File::getSpecialLocation(File::userApplicationDataDirectory ).getChildFile("Application Support/Sysex77");

static Path pathFilter1;
static Path pathFilter2;
static Path pathFilter3;
static Path pathFilter4;
int intTabIndex;
static constexpr int kTabVoice  = 2;
static constexpr int kTabCommon = 3;
// ValueTree for the Voice
//==============================================================================





//==============================================================================


#pragma once
#include "DeviceModel.h"
#include "LookAndFeel.h"
#include "Controller.h"
#include "Config.h"
#include "Voice.h"
#include "Librairie.h"
#include "Sy99LibrarySession.h"
#include "MidiObjects.h"

//==============================================================================
struct MidiDeviceListEntry : ReferenceCountedObject
{
    MidiDeviceListEntry (const String& deviceName, const String& deviceIdentifier = {})
        : name (deviceName), identifier (deviceIdentifier) {}
    
    String name;
    String identifier;
    std::unique_ptr<MidiInput> inDevice;
    std::unique_ptr<MidiOutput> outDevice;
    
    using Ptr = ReferenceCountedObjectPtr<MidiDeviceListEntry>;
};

//==============================================================================
struct DemoTabbedComponent  : public TabbedComponent
{
    DemoTabbedComponent (bool isRunningComponenTransformsDemo)
    : TabbedComponent (TabbedButtonBar::TabsAtTop)
    {
        auto colour = findColour (ResizableWindow::backgroundColourId);
        addTab (TRANS("Setting"),     colour, new ConfigPage (), true);
        addTab (TRANS("Librairie"),     colour, new LibrairiePage (), true);
       // addTab (TRANS("Midi"),     colour, new ControllerPage (), true);
        auto* voicePage = new VoicePage();
        addTab (TRANS("Voice"), colour, voicePage, true);
        addTab (TRANS("Common"), colour, new VoiceCommonPage (*voicePage), true);
        addTab (TRANS("Midi Setting"),colour,nullptr,false);
   
        
        
        
        //        getTabbedButtonBar().getTabButton (5)->setExtraComponent (new CustomTabButton (isRunningComponenTransformsDemo),
        //                                                                  TabBarButton::afterText);
    }
    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) override
    {
        const int prevTab = intTabIndex;

        if (suppressEditorExitPrompt())
        {
            if (newCurrentTabIndex < 4)
                intTabIndex = newCurrentTabIndex;
            return;
        }

        const auto isEditContextTab = [] (int tab) noexcept
        {
            return tab == kTabVoice || tab == kTabCommon;
        };

        const bool wasEdit = isEditContextTab (prevTab);
        const bool isEdit = isEditContextTab (newCurrentTabIndex);
        const bool commonToVoice = (prevTab == kTabCommon && newCurrentTabIndex == kTabVoice);
        const bool leavingEdit = wasEdit && (! isEdit || commonToVoice);

        if (! leavingEdit)
        {
            if (newCurrentTabIndex < 4)
                intTabIndex = newCurrentTabIndex;
            return;
        }

        const int targetTab = newCurrentTabIndex;

        auto proceed = [this, targetTab, commonToVoice]()
        {
            setCurrentTabIndex (targetTab);

            if (targetTab < 4)
                intTabIndex = targetTab;

            if (commonToVoice)
                if (auto& cb = voiceTabRecallFromCommonCallback(); cb != nullptr)
                    cb();
        };

        proceed();
    }
    void popupMenuClickOnTab (int tabIndex, const String& tabName)override
    {
        Logger::writeToLog("TabbedComponent: popupMenuClick");
    }
    // This is a small star button that is put inside one of the tabs. You can
    // use this technique to create things like "close tab" buttons, etc.
    class CustomTabButton  : public Component
    {
    public:
        CustomTabButton (bool isRunningComponenTransformsDemo)
        : runningComponenTransformsDemo (isRunningComponenTransformsDemo)
        {
            setSize (20, 20);
        }
        
        void paint (Graphics& g) override
        {
            Path star;
            star.addStar ({}, 7, 1.0f, 2.0f);
            
            g.setColour (Colours::green);
            g.fillPath (star, star.getTransformToScaleToFit (getLocalBounds().reduced (2).toFloat(), true));
        }
        void mouseEnter (const MouseEvent& mouseEvent) override
        {
            
        }
        void mouseDown (const MouseEvent&) override
        {
            
        }
    private:
        bool runningComponenTransformsDemo;
        //  std::unique_ptr<BubbleMessageComponent> bubbleMessage;
    };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoTabbedComponent)
};

//==============================================================================


//==============================================================================
class MidiDemo  : public Component,
private Timer,
private MidiKeyboardStateListener,
private MidiInputCallback,
private AsyncUpdater,
private TextButton::Listener,
 public ComboBox::Listener,
 public Value::Listener,
private OSCReceiver,                                                          // [1]
private OSCReceiver::ListenerWithOSCAddress<OSCReceiver::MessageLoopCallback> // [2]
{
public:
    //==============================================================================
    MidiDemo (bool isRunningComponenTransformsDemo = false)
    :  midiKeyboard (keyboardState, MidiKeyboardComponent::horizontalKeyboard)
    {
        // Defer ListBox/tabs construction until MidiDemo subobjects (midiInputs/midiOutputs)
        // are fully initialised; MidiDeviceListBox must not pass `this` as model in the
        // ListBox base initializer (JUCE calls getNumRows during setModel).
        midiInputSelector.reset (new MidiDeviceListBox ("Midi Input Selector", *this, true));
        midiOutputSelector.reset (new MidiDeviceListBox ("Midi Output Selector", *this, false));
        tabs = std::make_unique<DemoTabbedComponent> (isRunningComponenTransformsDemo);

        // Init to light the Paint call

        imgBack = ImageCache::getFromMemory(BinaryData::Sysex77_png, BinaryData::Sysex77_pngSize);
        if(SYModel==3)
            imgBack = ImageCache::getFromMemory(BinaryData::Sysex99_png, BinaryData::Sysex99_pngSize);
        

        
        addAndMakeVisible (*tabs);
  
        addLabelAndSetStyle (midiInputLabel);
        addLabelAndSetStyle (midiOutputLabel);
        addLabelAndSetStyle (incomingMidiLabel);
        
        addAndMakeVisible(btBulk);
        btBulk.setClickingTogglesState(true);
        btBulk.setColour(TextButton::ColourIds::buttonOnColourId, Colours::darkorange);
        btBulk.addListener(this);

        // "Show realtime" — default OFF: hides F8/FA/FB/FC/FF clock spam
        addAndMakeVisible(btShowRealtime);
        btShowRealtime.setClickingTogglesState(true);
        btShowRealtime.setColour(TextButton::buttonOnColourId, Colours::darkorange);
        btShowRealtime.onClick = [this] { showRealtimeMidi = btShowRealtime.getToggleState(); };

        // "SysEx only" — default OFF: shows all messages; ON isolates parameter SysEx
        addAndMakeVisible (btShowSysExOnly);
        btShowSysExOnly.setColour (ToggleButton::tickColourId, Colours::darkorange);
        btShowSysExOnly.setColour (ToggleButton::tickDisabledColourId, Colours::darkorange);
        btShowSysExOnly.onClick = [this]
        {
            showSysExOnly = btShowSysExOnly.getToggleState();
            syncMonitorSysExOnlyButtonLook();
            midiPersistSaveMonitorOptions (showSysExOnly);
        };

        incomingMidiLabel.setInterceptsMouseClicks (false, false);

        midiPersistLoadMonitorOptions (showSysExOnly);
        applyMonitorSysExOnlyToggleUi();

        addAndMakeVisible(btClearMonitor);
        btClearMonitor.onClick = [this] {
            midiMonitor.clear();
            sysExMsgCounter    = 0;
            midiMonitorCharCount = 0;
        };

        // Monitor has no keyboard focus & no caret — Ctrl+C/extra menu won't work;
        // one-click clipboard for logs.
        addAndMakeVisible (btCopyMonitor);
        btCopyMonitor.onClick = [this]
        {
            SystemClipboard::copyTextToClipboard (midiMonitor.getText());
        };

        
        midiKeyboard.setName ("MIDI Keyboard");
        addAndMakeVisible (midiKeyboard);
        
        midiMonitor.setMultiLine (true);
        midiMonitor.setReturnKeyStartsNewLine (false);
        midiMonitor.setReadOnly (true);
        midiMonitor.setScrollbarsShown (true);
        midiMonitor.setCaretVisible (false);
        midiMonitor.setPopupMenuEnabled (false);
        midiMonitor.setText ({});
        midiMonitor.setWantsKeyboardFocus (false); // logs must not steal keys from Voice name field
        midiMonitor.setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (midiMonitor);
        
        if (! BluetoothMidiDevicePairingDialogue::isAvailable())
            pairButton.setEnabled (false);
        
        addAndMakeVisible (pairButton);
        pairButton.onClick = []
        {
            RuntimePermissions::request (RuntimePermissions::bluetoothMidi,
                                         [] (bool wasGranted)
                                         {
                                             if (wasGranted)
                                                 BluetoothMidiDevicePairingDialogue::open();
                                         });
        };
        keyboardState.addListener (this);
        
        addAndMakeVisible (midiInputSelector .get());
        addAndMakeVisible (midiOutputSelector.get());
        
        
        loadData();
        Logger::writeToLog("XML Num" + String(numRows));
        addAndMakeVisible(labelFoot);
        addAndMakeVisible(labelMod);
        
        addAndMakeVisible (comboFoot);
        
        comboFoot.setEditableText (false);
        comboFoot.setJustificationType (Justification::left);
        comboFoot.addListener(this);
        

        addAndMakeVisible (comboMod);
        
        comboMod.setEditableText (false);
        comboMod.setJustificationType (Justification::left);
        comboMod.addListener(this);

        
        for (int i = 1; i < numRows; ++i)
            //  comboBox.addItem ("combo box item " + String (i), i);
            //   forEachXmlChildElement (*dataList, "name")
        {
            if (auto* rowElement = dataList->getChildElement(i))
            {
                auto text = rowElement->getStringAttribute (getAttributeNameForColumnId (1));
                text = text + " " + rowElement->getStringAttribute (getAttributeNameForColumnId (2));
                comboFoot.addItem (text,i);
                comboMod.addItem (text,i);
            }
            
            
        }
                comboFoot.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::COMMONFOOT, nullptr));
                comboMod.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::COMMONMOD, nullptr));
        
// Si pas de selection on evite un champ vide en selectionnant le premier
        if(comboFoot.getSelectedItemIndex()<1)
            comboFoot.setSelectedId(1);
        if(comboMod.getSelectedItemIndex()<1)
            comboMod.setSelectedId(1);
// ----------------------------
        if (! connect (9001))                   // [3]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        // tell the component to listen for OSC messages matching this address:
        //       addListener (this, "/77Foot"); // [4]
        //       addListener (this, "/77Mod");
        
        if (! senderMidiIn.connect ("127.0.0.1", 9002)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        

        addOscListener();
        tabs->setVisible (false);
        tabs->setAlwaysOnTop (true);


        setSize (732, 520);
        tabs->setCurrentTabIndex (1);
        intTabIndex = 1;
        startTimer (500);

        // THROTTLE DEBOUNCE: wire the flush timer back to this MidiDemo.
        throttleTimer.owner = this;

        libraryVoiceProgramChangeCallback() = [this] (int globalIdx)
        {
            auto& echo = libraryVoiceProgramChangeEchoGuard();
            echo.lastGlobalIdx = globalIdx;
            echo.sentAtMs = juce::Time::getMillisecondCounter();
            sendLibraryVoiceRecallToSynth (globalIdx);
        };

        libraryPageBankSelectCallback() = [this] (Sy99LibraryContentPage page)
        {
            sendLibraryPageBankSelectToSynth (page);
        };

        sy99DumpRequestSendCallback() = [this] (const juce::MidiMessage& msg)
        {
            if (sy99AnyLibrarySyncActive())
                sendSysexHardware (msg);
            else
                sendRaw (msg.getRawData(), (long) msg.getRawDataSize());
        };

        sy99InvokeBulkLoadSendCallback() = [this] (const juce::MidiMessage& msg)
        {
            sendMidiMessagesToOutputsShunted ({ msg });
        };

        sy99BankClick0040ReapplyCallback() = [this]
        {
            const int targetMm = sy99BankClick0040FetchTargetMm();

            if (targetMm >= 0 && bankSelectedVoiceIndex >= 0 && targetMm != bankSelectedVoiceIndex)
            {
                juce::Logger::writeToLog ("[BankClick] fetch0040 reapply skipped stale mm="
                                          + juce::String::toHexString (targetMm).paddedLeft ('0', 2)
                                          + " selected="
                                          + juce::String::toHexString (bankSelectedVoiceIndex).paddedLeft ('0', 2));
                return;
            }

            const auto& s = getLiveSynthState();
            juce::Logger::writeToLog ("[BankClick] fetch0040 reapply has0040="
                                      + juce::String (s.hasParsedBulk0040 ? 1 : 0)
                                      + " EFMODE_bulk="
                                      + juce::String (s.lm0040EfmodeRaw)
                                      + " skip0040Blocks="
                                      + juce::String (sy99BankClickSkip0040DependentBlocks() ? 1 : 0));

            if (auto* voicePage = dynamic_cast<VoicePage*> (tabs->getTabContentComponent (kTabVoice)))
                voicePage->applyLiveSynthStateToEditor();
            else
                bankVoiceSlotApplyTrigger.setValue (++bankVoiceSlotApplyNonce);
        };

        librarySy99SessionSlotSelectCallback() = [] (int globalMm)
        {
            if (sy99LibrarySession().active)
                selectLibrarySlotWithEditor (Sy99LibraryContentPage::internalVoices, globalMm);
        };

        Sy99MidiTransport::instance().setSender ([this] (const juce::MidiMessage& msg)
        {
            sendSysexHardware (msg);
        });

        sy99MidiPortStatusCallback() = [this]() -> juce::String
        {
            bool inOpen = false;
            bool outOpen = false;

            for (auto& entry : midiInputs)
                if (entry->inDevice.get() != nullptr)
                    inOpen = true;

            for (auto& entry : midiOutputs)
                if (entry->outDevice.get() != nullptr)
                    outOpen = true;

            if (! inOpen && ! outOpen)
                return Sy99Ui::midiInOutNotOpen();

            if (! outOpen)
                return Sy99Ui::midiOutNotOpen();

            if (! inOpen)
                return Sy99Ui::midiInNotOpen();

            return {};
        };

        voiceTabRecallFromCommonCallback() = [this]
        {
            sendVoiceTabRecallFromCommonEdit();
        };

        if (auto* voicePage = dynamic_cast<VoicePage*> (tabs->getTabContentComponent (kTabVoice)))
        {
            commitEditorSessionToLiveSynthBaselineCallback() = [voicePage]()
            {
                voicePage->commitEditorSessionToLiveSynthBaseline();
            };

            editorPatchDirtyMarkCallback() = [] { markEditorPatchDirtyFromSynth(); };
            editorPatchDirtySuppressCallback() = [] { return suppressEditorPatchDirtyMark(); };
            editorRefreshFromLiveSynthCallback() = [voicePage]()
            {
                if (voicePage == nullptr)
                    return;

                suppressEditorPatchDirtyMark() = true;
                voicePage->applyLiveSynthStateToEditor (false);
                suppressEditorPatchDirtyMark() = false;
            };

            Sy99ParamRegistry::metaRegistryChangedCallback() = [voicePage]()
            {
                voicePage->refreshEnumCombosFromMetaRegistry();
            };
        }

        Sy99HardwareMappingRuntime::setHandlers (
            [this] (const uint8_t frame[9]) { sendParamSysexFrame (frame); },
            [this] (int channel, int cc, int value, const juce::String& portMatch)
            {
                sendFeedbackCc (channel, cc, value, portMatch);
            });

        juce::MessageManager::callAsync ([this]
        {
            updateDeviceList (true);
            updateDeviceList (false);
            restorePersistedMidiDevicesIfNeeded();
        });
    }
    
    ~MidiDemo()
    {
        stopTimer();
        throttleTimer.stopTimer();

        persistOpenMidiDevices();
        midiPersistSaveMonitorOptions (showSysExOnly);

        midiInputs .clear();
        midiOutputs.clear();
        keyboardState.removeListener (this);
        btBulk.removeListener(this);


        midiInputSelector .reset();
        midiOutputSelector.reset();
        comboFoot.removeListener(this);
        comboMod.removeListener(this);


    }
    

    void valueChanged (Value& value) override
    {
        Logger::writeToLog("midi demo value change");
    }
    
    void buttonClicked (Button* button) override
    {
        if(button == &btBulk)
        {
            Logger::writeToLog("Bulk Protect");
            uint8 sysexdata[9] = { 0x43, DeviceModel::getInstance().getSysExDeviceID(), 0x34, 0x0f, 0x00, 0x00, 0x34, 0x00, 0x00 };
            sysexdata[8] = btBulk.getToggleState();
            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
    }
    //==============================================================================
    void timerCallback() override
    {
        if (tabs->getCurrentTabIndex() == 4)
        {
            tabs->setCurrentTabIndex (intTabIndex);
           tabs->setVisible (false);
            
            //repaint();
        }
        updateDeviceList (true);
        updateDeviceList (false);
    }
    
    /** Send a burst of SysEx (e.g. VNAM ×10) with one MIDI-input shunt window. */
    void sendMidiMessagesToOutputsShunted (const Array<MidiMessage>& messages)
    {
        if (messages.isEmpty())
            return;

        boolStopReceive = true;

        for (auto midiOutput : midiOutputs)
            if (midiOutput->outDevice.get() != nullptr)
                for (const auto& m : messages)
                {
                    Logger::writeToLog ("OUT3 midi send");
                    midiOutput->outDevice->sendMessageNow (m);
                    MidiStreamLogger::logOutgoing (m, midiOutput->name);
                }

        boolStopReceive = false;
    }

    //==============================================================================

#include "MidiSysex.h"
    
    /*
     ==============================================================================
     
     Pour réduire le code et retrouver facilement la routine l'include externalise
     le code de la routine qui recupere l'osc et envoi le sysex en midi
     
     ==============================================================================
     */
    
    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    
    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        MidiMessage m (MidiMessage::noteOff (midiChannel, midiNoteNumber, velocity));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }
    
    void paint (Graphics& g) override
    {
        //Logger::writeToLog ("MidiDemo Paint");
        g.fillAll (SYColBackground);
        g.drawImageAt(imgBack,getLocalBounds().getWidth() - imgBack.getWidth() - 20, 10);
        
        
    }
    void mouseDown (const MouseEvent&) override
    {
        if (tabs->isVisible())
        {
            tabs->setVisible (false);
        }
        else
        {
            tabs->setVisible (true);
        }
        
    }
    void resized() override
    {
        auto margin = 10;
        
        
        
        midiInputLabel.setBounds (margin, margin + 34,
                                  (getWidth() / 2) - (2 * margin), 24);
        
        midiOutputLabel.setBounds ((getWidth() / 2) + margin, margin + 34,
                                   (getWidth() / 2) - (2 * margin), 24);
        
        midiInputSelector->setBounds (margin, (2 * margin) + 48,
                                      (getWidth() / 2) - (2 * margin),
                                      (getHeight() / 2) - ((4 * margin) + 64 ));
        
        midiOutputSelector->setBounds ((getWidth() / 2) + margin, (2 * margin) + 48,
                                       (getWidth() / 2) - (2 * margin),
                                       (getHeight() / 2) - ((4 * margin) + 64 ));
        
        pairButton.setBounds (margin, (getHeight() / 2) - (margin + 24),
                              getWidth() - (2 * margin), 24);
        
        const int filterRowY = getHeight() / 2;
        incomingMidiLabel.setBounds (margin, filterRowY - 26, getWidth() - (2 * margin), 22);

        midiMonitor.setBounds (margin, (filterRowY + 24 + margin), getWidth() - (2 * margin), getHeight()/2 - 128);
        // Filter buttons left of btBulk; label sits on the row above (no overlap)
        btShowRealtime .setBounds (margin,                filterRowY, getWidth() / 5, 24);
        btShowSysExOnly.setBounds (margin + getWidth()/5, filterRowY, getWidth() / 5, 24);
        btBulk         .setBounds (getWidth() / 2,        filterRowY, getWidth() / 4, 24);
        {
            const int yBtn = filterRowY;
            const int rightQuarterX = (getWidth() * 3) / 4;
            const int rightQuarterW = getWidth() / 4 - margin;
            constexpr int gap = 4;
            const int copyW = jmin (52, jmax (40, rightQuarterW / 3));
            btCopyMonitor .setBounds (rightQuarterX, yBtn, copyW, 24);
            btClearMonitor.setBounds (rightQuarterX + copyW + gap, yBtn, jmax (20, rightQuarterW - copyW - gap), 24);
        }
        
        tabs->setBounds (0, 10, getWidth(), getHeight() - 84);
        
        
        midiKeyboard.setBounds (margin +140, (getHeight() - 70 ), getWidth() - (2 * margin) - 140, 70);
        labelFoot.setBounds(margin,(getHeight() - 77 ), 134, 20);
        comboFoot.setBounds(margin,(getHeight() - 58 ), 134, 20);
        labelMod.setBounds(margin,(getHeight() - 40 ), 134, 20);
        comboMod.setBounds(margin,(getHeight() - 24 ), 134, 20);
    }
    
    void openDevice (bool isInput, int index)
    {
        if (isInput)
        {
            if (index < 0 || index >= midiInputs.size())
                return;

            if (midiInputs[index]->inDevice.get() != nullptr)
                return;

            midiInputs[index]->inDevice = MidiInput::openDevice (midiInputs[index]->identifier, this);
            
            if (midiInputs[index]->inDevice.get() == nullptr)
            {
                DBG ("MidiDemo::openDevice: open input device for index = " << index << " failed!");
                return;
            }
            
            midiInputs[index]->inDevice->start();
        }
        else
        {
            if (index < 0 || index >= midiOutputs.size())
                return;

            if (midiOutputs[index]->outDevice.get() != nullptr)
                return;

            midiOutputs[index]->outDevice = MidiOutput::openDevice (midiOutputs[index]->identifier);
            
            if (midiOutputs[index]->outDevice.get() == nullptr)
            {
                DBG ("MidiDemo::openDevice: open output device for index = " << index << " failed!");
            }
        }

        if (! isInput)
            flushPendingLibraryRecall();

        tryStartupEditBufferSyncFromMidiConnect();
    }
    
    void closeDevice (bool isInput, int index)
    {
        if (isInput)
        {
            if (index < 0 || index >= midiInputs.size())
                return;

            if (midiInputs[index]->inDevice.get() == nullptr)
                return;

            midiInputs[index]->inDevice->stop();
            midiInputs[index]->inDevice.reset();
        }
        else
        {
            if (index < 0 || index >= midiOutputs.size())
                return;

            if (midiOutputs[index]->outDevice.get() == nullptr)
                return;

            midiOutputs[index]->outDevice.reset();
        }
    }
    
    int getNumMidiInputs() const noexcept
    {
        return midiInputs.size();
    }
    
    int getNumMidiOutputs() const noexcept
    {
        return midiOutputs.size();
    }
    
    ReferenceCountedObjectPtr<MidiDeviceListEntry> getMidiDevice (int index, bool isInput) const noexcept
    {
        return isInput ? midiInputs[index] : midiOutputs[index];
    }
    
private:
    //==============================================================================
    struct MidiDeviceListBox : public ListBox,
    private ListBoxModel
    {
        MidiDeviceListBox (const String& name,
                           MidiDemo& contentComponent,
                           bool isInputDeviceList)
        : ListBox (name, nullptr),
          parent (contentComponent),
          isInput (isInputDeviceList)
        {
            setModel (this);
            setOutlineThickness (1);
            setMultipleSelectionEnabled (true);
            setClickingTogglesRowSelection (true);
        }
        
        //==============================================================================
        int getNumRows() override
        {
            return isInput ? parent.getNumMidiInputs()
            : parent.getNumMidiOutputs();
        }
        
        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            auto textColour = getLookAndFeel().findColour (ListBox::textColourId);
            
            if (rowIsSelected)
                g.fillAll (textColour.interpolatedWith (getLookAndFeel().findColour (ListBox::backgroundColourId), 0.5));
            
            
            g.setColour (textColour);
            g.setFont (height * 0.7f);
            
            if (isInput)
            {
                if (rowNumber < parent.getNumMidiInputs())
                    g.drawText (parent.getMidiDevice (rowNumber, true)->name,
                                5, 0, width, height,
                                Justification::centredLeft, true);
            }
            else
            {
                if (rowNumber < parent.getNumMidiOutputs())
                    g.drawText (parent.getMidiDevice (rowNumber, false)->name,
                                5, 0, width, height,
                                Justification::centredLeft, true);
            }
        }
        
        //==============================================================================
        void selectedRowsChanged (int) override
        {
            auto newSelectedItems = getSelectedRows();
            if (newSelectedItems != lastSelectedItems)
            {
                for (auto i = 0; i < lastSelectedItems.size(); ++i)
                {
                    if (! newSelectedItems.contains (lastSelectedItems[i]))
                        parent.closeDevice (isInput, lastSelectedItems[i]);
                }
                
                for (auto i = 0; i < newSelectedItems.size(); ++i)
                {
                    if (! lastSelectedItems.contains (newSelectedItems[i]))
                        parent.openDevice (isInput, newSelectedItems[i]);
                    
                }
                
                lastSelectedItems = newSelectedItems;
                parent.persistOpenMidiDevices();
            }
        }
        
        //==============================================================================
        void syncSelectedItemsWithDeviceList (const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices)
        {
            SparseSet<int> selectedRows;
            for (auto i = 0; i < midiDevices.size(); ++i)
                if (midiDevices[i]->inDevice.get() != nullptr || midiDevices[i]->outDevice.get() != nullptr)
                    selectedRows.addRange (Range<int> (i, i + 1));
            
            lastSelectedItems = selectedRows;
            updateContent();
            setSelectedRows (selectedRows, dontSendNotification);
        }
        
    private:
        //==============================================================================
        MidiDemo& parent;
        bool isInput;
        SparseSet<int> lastSelectedItems;
    };
    
    //==============================================================================
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override
    {
        // This is called on the MIDI thread
        const ScopedLock sl (midiMonitorLock);

        // Library sync: dedicated RX transport (worker thread). Bulk capture: legacy buffer.
        if (message.isSysEx() && sy99AnyLibrarySyncActive())
        {
            Sy99MidiTransport::instance().postReceive (message);
            sy99ScheduleLibrarySyncAdvanceOnReceive();
        }
        else if (requestSysex && message.isSysEx())
        {
            arraySysex.add (message);
            timeOut = 0;
        }

        sy99BankClick0040NoteIncoming (message);

        if (gRequestLiveVoiceRead && message.isSysEx())
        {
            notifyLiveVoiceReadActivity();
            gArrayLiveReadSysex.add (message);

            const int n = message.getSysExDataSize();
            const uint8* d = message.getSysExData();

            if (n == 9 && d[0] == 0x43 && d[2] == 0x34)
            {
                // Dump stream param9 for EFSDLV is unreliable (often 0 or stale 8101 tail e.g. 106).
                const bool skipEfsendlvlDuringDump = gRequestLiveVoiceRead
                    && YamahaLmVoiceDump::isEfsendlvlLiveParam9Frame (d[3], d[4], d[5], d[6]);

                if (! skipEfsendlvlDuringDump)
                {
                    gLiveSynthState.ingestParameterFrame (d[3], d[4], d[5], d[6], d[7], d[8]);
                    Sy99HardwareMappingRuntime::onLiveParameterSysex (d[3], d[4], d[5], d[6], d[8]);
                }
            }
            else
                gLiveSynthState.noteNonParameterMessage (n);
        }

        if (message.isSysEx())
        {
            const juce::String portName = source != nullptr ? source->getName() : juce::String();
            MidiStreamLogger::logIncoming (message, portName);
        }

        if (boolStopReceive || message.isActiveSense())
            return;

        if (message.isSysEx())
            handleIncomingLibrarySynthPanelSysex (message);

        if (message.isController())
        {
            const int ch = message.getChannel();
            const int cc = message.getControllerNumber();

            if (ch == 1)
            {
                if (cc == 0)
                {
                    inboundLibraryBankMsb() = message.getControllerValue();
                    libraryInboundNoteCc0 (ch, message.getControllerValue());
                }
                else if (cc == 32)
                {
                    inboundLibraryBankLsb() = message.getControllerValue();
                    libraryInboundNoteCc32 (ch, message.getControllerValue());
                }
            }

            const juce::String portName = source != nullptr ? source->getName() : juce::String();
            const int ccValue = message.getControllerValue();
            juce::MessageManager::callAsync ([portName, ch, cc, ccValue]()
            {
                Sy99HardwareMappingRuntime::handleIncomingCc (portName, ch, cc, ccValue, sysexEngine);
            });
        }

        if (message.isProgramChange())
        {
            const int pc = message.getProgramChangeNumber();
            const int ch = message.getChannel();

            if (ch == 1 && pc >= 0)
                processInboundLibraryProgramChangeOnMidiThread (pc, ch);
        }

        // Realtime single-byte messages: F8=Clock, FA=Start, FB=Continue,
        // FC=Stop, FD=ActiveSense(already caught above), FF=Reset.
        // Status byte >= 0xF8 identifies all realtime messages.
        // When showRealtimeMidi is OFF, skip adding to the display queue entirely
        // so triggerAsyncUpdate() is not called on every MIDI clock tick.
        const bool isRealtime = (message.getRawDataSize() == 1
                                 && message.getRawData()[0] >= 0xF8);
        if (isRealtime && !showRealtimeMidi)
            return;

        incomingMessages.add (message);
        triggerAsyncUpdate();
    }
    template<typename... Ts>
    inline var make_var_array(Ts... values)
    {
        return Array<var>(values...);
    }
    void handleAsyncUpdate() override
    {
        // Drain MIDI queue once per async tick: local batch avoids recycling an old buffer
        // back into incomingMessages via swapWith (which caused duplicate monitor lines).
        Array<MidiMessage> batch;

#if SYSEX77_MIDI_MONITOR_DIAGNOSTICS
        int incomingBefore = 0;
        int incomingAfterSwap = 0;
#endif

        {
            const ScopedLock sl (midiMonitorLock);
#if SYSEX77_MIDI_MONITOR_DIAGNOSTICS
            incomingBefore = incomingMessages.size();
#endif
            batch.swapWith (incomingMessages);
#if SYSEX77_MIDI_MONITOR_DIAGNOSTICS
            incomingAfterSwap = incomingMessages.size();
#endif
        }

#if SYSEX77_MIDI_MONITOR_DIAGNOSTICS
        const int batchSizeAfterSwap = batch.size();
        int maxRunEqualSysEx = 0;
        int currentRun = 0;
        MemoryBlock prevSysExBlock;
        bool havePrevSysEx = false;

        for (const auto& message : batch)
        {
            if (! message.isSysEx())
                continue;

            MemoryBlock block (message.getSysExData(),
                               static_cast<size_t> (message.getSysExDataSize()));

            if (havePrevSysEx && block == prevSysExBlock)
            {
                ++currentRun;
                maxRunEqualSysEx = jmax (maxRunEqualSysEx, currentRun);
            }
            else
            {
                prevSysExBlock = block;
                currentRun = 1;
                havePrevSysEx = true;
                maxRunEqualSysEx = jmax (maxRunEqualSysEx, 1);
            }
        }

        Logger::writeToLog ("[MidiMon] incoming_before=" + String (incomingBefore)
                            + " incoming_after_swap=" + String (incomingAfterSwap)
                            + " batch=" + String (batchSizeAfterSwap)
                            + " sysex_max_run_equal=" + String (maxRunEqualSysEx));
#endif

        String messageText;

        if (gLiveReadClearMonitorPending)
        {
            gLiveReadClearMonitorPending = false;
            midiMonitor.clear();
            sysExMsgCounter = 0;
            midiMonitorCharCount = 0;
            messageText << "[LiveRead] monitor cleared — waiting for single-voice dump (07:1 Voice)\n";
        }

        for (auto& message : batch)
        {
            // SysEx parameter extraction — always, independent of display filters
            if (! gRequestLiveVoiceRead && message.getSysExDataSize() == 9)
            {
                memcpy(&data, message.getSysExData(), message.getSysExDataSize());
                if (data[0] == 0x43 && data[2] == 0x34)
                {
                    // ECHO GUARD: ignore own reflections for 50ms after send
                    const uint8 incomingAddr[4] = { data[3], data[4], data[5], data[6] };
                    if (! isRecentEcho (incomingAddr))
                    {
                        const bool ingestedLive = gLiveSynthState.ingestParameterFrame (data[3], data[4], data[5],
                                                                                        data[6], data[7], data[8]);
                        valueSysexIn = make_var_array(data[3], data[4], data[5],
                                                      data[6], data[7], data[8]);
                        Sy99HardwareMappingRuntime::onLiveParameterSysex (data[3], data[4], data[5],
                                                                          data[6], data[8]);

                        if (ingestedLive)
                        {
                            if (auto& refresh = editorRefreshFromLiveSynthCallback(); refresh != nullptr)
                                refresh();
                        }
                    }
                }
            }

            // "SysEx only" display filter
            if (showSysExOnly && !message.isSysEx())
                continue;

            // Format: SysEx gets a sequential index and full hex bytes;
            // other messages use JUCE's compact getDescription()
            if (message.isSysEx())
            {
                const uint8* d   = message.getSysExData();
                const int    len = message.getSysExDataSize();
                const bool captureMode = gRequestLiveVoiceRead || requestSysex;
                // Single-voice LM blocks are typically 585+113 etc. (always < 2 KB).
                // Omit hex only for very large frames outside capture (e.g. casual 64-voice flood).
                const bool showFullHex = captureMode || len <= 2048;

                if (! showFullHex)
                {
                    messageText << "[#" << String (++sysExMsgCounter) << "] SysEx: "
                                << len << " bytes (hex omitted)\n";
                }
                else
                {
                    String hexStr = "F0";
                    for (int i = 0; i < len; ++i)
                        hexStr << " " << String::toHexString (d[i]).toUpperCase();
                    hexStr << " F7";
                    messageText << "[#" << String (++sysExMsgCounter) << "] SysEx: "
                                << hexStr << "\n";
                }
            }
            else
            {
                messageText << message.getDescription() << "\n";
            }
        }

        if (messageText.isNotEmpty())
        {
            midiMonitorCharCount += messageText.length();

            const int maxChars = (gRequestLiveVoiceRead || requestSysex)
                                     ? kMidiMonitorMaxCharsLiveRead
                                     : 12000;

            if (midiMonitorCharCount > maxChars)
            {
                midiMonitor.clear();
                sysExMsgCounter = 0;
                midiMonitorCharCount = messageText.length();
                messageText = "[monitor buffer full — older lines removed]\n" + messageText;
            }

            midiMonitor.insertTextAtCaret (messageText);
        }
    }
public:
    /** Common → Voice tab: always send full CC0+CC32+PC for bankSelectedVoiceIndex.
        Separate from library-click recall; ignores libraryRecallContextBankMsb. */
    void sendVoiceTabRecallFromCommonEdit()
    {
        constexpr int midiCh = 1;
        const int globalIdx = resolveCurrentEditorVoiceGlobalIdx();

        if (globalIdx < 0)
        {
            Logger::writeToLog ("[VOICE-TAB-RECALL] skipped: no valid editor context"
                                + (bankSelected.isEmpty() ? String (" (no bank loaded)")
                                                          : String())
                                + " bankSelectedVoiceIndex=" + String (bankSelectedVoiceIndex));
            return;
        }

        const int bankMsb = globalIdx / 16;
        const int bankLsb = sy99BankLsbForLibraryContentPage (libraryContentPage());
        const int program = sy99OutboundProgramForLibrarySlot (libraryContentPage(), globalIdx);

        auto& echo = libraryVoiceProgramChangeEchoGuard();
        echo.lastGlobalIdx = globalIdx;
        echo.lastBankMsb = bankMsb;
        echo.lastBankLsb = bankLsb;
        echo.lastPage = libraryContentPage();
        echo.sentAtMs = juce::Time::getMillisecondCounter();

        sendToOutputs (MidiMessage::controllerEvent (midiCh, 0, 0));
        sendToOutputs (MidiMessage::controllerEvent (midiCh, 32, bankLsb));

        MidiMessage pc = MidiMessage::programChange (midiCh, program);
        pc.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (pc);

        juce::String txLog = "[VOICE-TAB-RECALL] CC32=" + String (bankLsb)
                             + " CC0=0 PC=" + String (program)
                  + " source=Common->Voice currentEditIdx=" + String (globalIdx)
                  + " label=\"" + libraryVoiceSlotLabel (globalIdx) + "\"";
        Logger::writeToLog (txLog);
    }

    void sendLibraryPageBankSelectToSynth (Sy99LibraryContentPage page)
    {
        constexpr int midiCh = 1;

        if (! hasMidiOutputOpen())
        {
            Logger::writeToLog ("[LIBSYNC] Page bank select skipped (MIDI Out not open) page="
                                + String ((int) page));
            return;
        }

        const int bankLsb = sy99BankLsbForLibraryContentPage (page);
        bool sentOk = sendToOutputs (MidiMessage::controllerEvent (midiCh, 0, 0));

        if (sentOk)
            sentOk = sendToOutputs (MidiMessage::controllerEvent (midiCh, 32, bankLsb));

        if (sentOk)
            libraryRecallContextAfterPageBankSelect (page);

        libraryNavAuditLog ("TX page-select", page, 0, bankLsb, -1, -1, -1, true, false);

        Logger::writeToLog ("[LIBSYNC] Page bank select ch" + String (midiCh)
                            + " CC0=0 CC32=" + String (bankLsb)
                            + " page=" + String ((int) page)
                            + (sentOk ? "" : " (TX failed)"));
    }

    bool hasMidiOutputOpen() const noexcept
    {
        for (auto& entry : midiOutputs)
            if (entry->outDevice.get() != nullptr)
                return true;

        return false;
    }

    void flushPendingLibraryRecall()
    {
        if (! pendingLibraryRecallValid || pendingLibraryRecallIdx < 0)
            return;

        if (! hasMidiOutputOpen())
            return;

        const int mm = pendingLibraryRecallIdx;
        const bool wasDeferred = libraryVoiceRecallDeferredToMidiOpen();
        pendingLibraryRecallValid = false;
        pendingLibraryRecallIdx = -1;
        libraryVoiceRecallDeferredToMidiOpen() = false;

        if (wasDeferred)
        {
            sendLibraryPageBankSelectToSynth (libraryContentPage());
            libraryForceNextRecallFullTriple() = true;
        }

        sendLibraryVoiceRecallToSynth (mm);
        sy99BeginBankClickHwRefreshAfterRecallWait();
    }

    /** [LIBSYNC] Recall SY99 voice (outbound). Strong CC0+CC32+PC when not in sync;
        PC-only when sy99HostSynthNavInSync. Context committed only after successful TX. */
    void sendLibraryVoiceRecallToSynth (int globalIdx)
    {
        constexpr int midiCh = 1;

        const auto page = libraryContentPage();
        const bool isMulti = libraryPageIsMulti (page);
        const int maxIdx = isMulti ? 15 : 63;

        if (globalIdx < 0 || globalIdx > maxIdx)
        {
            Logger::writeToLog ("[LIBSYNC] Recall skipped: globalIdx " + String (globalIdx)
                                + " out of range 0…" + String (maxIdx));
            pendingLibraryRecallValid = false;
            return;
        }

        libraryVoiceRecallDeferredToMidiOpen() = false;

        if (! hasMidiOutputOpen())
        {
            pendingLibraryRecallIdx = globalIdx;
            pendingLibraryRecallValid = true;
            libraryVoiceRecallDeferredToMidiOpen() = true;
            Logger::writeToLog ("[LIBSYNC] Recall deferred (MIDI Out not open) mm="
                                + String (globalIdx)
                                + " page=" + String ((int) page));
            return;
        }

        pendingLibraryRecallValid = false;

        const int bankMsb = globalIdx / 16;
        const int bankLsb = sy99BankLsbForLibraryContentPage (page);
        const int program = sy99OutboundProgramForLibrarySlot (page, globalIdx);
        const bool fullTriple = libraryOutboundNeedsFullTriple (page, globalIdx);

        if (fullTriple && libraryForceNextRecallFullTriple())
            libraryForceNextRecallFullTriple() = false;

        const bool sendCc0 = sy99OutboundNeedsCc0ForRecall (page, globalIdx, fullTriple);
        const int cc0Value = fullTriple ? 0 : bankMsb;

        auto& echo = libraryVoiceProgramChangeEchoGuard();
        echo.lastGlobalIdx = globalIdx;
        echo.lastBankMsb = bankMsb;
        echo.lastBankLsb = bankLsb;
        echo.lastPage = page;
        echo.sentAtMs = juce::Time::getMillisecondCounter();

        bool sentOk = true;

        if (fullTriple)
        {
            if (sendCc0)
                if (! sendToOutputs (MidiMessage::controllerEvent (midiCh, 0, cc0Value)))
                    sentOk = false;

            if (sentOk)
                if (! sendToOutputs (MidiMessage::controllerEvent (midiCh, 32, bankLsb)))
                    sentOk = false;
        }
        else if (sendCc0)
        {
            if (! sendToOutputs (MidiMessage::controllerEvent (midiCh, 0, cc0Value)))
                sentOk = false;
        }

        if (sentOk)
        {
            MidiMessage pc = MidiMessage::programChange (midiCh, program);
            pc.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);

            if (! sendToOutputs (pc))
                sentOk = false;
        }

        if (sentOk)
            libraryRecallContextAfterSend (page, globalIdx);
        else
            Logger::writeToLog ("[LIBSYNC] Recall TX failed — context not committed mm="
                                + String (globalIdx));

        libraryNavAuditLog ("TX", page, sendCc0 ? cc0Value : -1, fullTriple ? bankLsb : -1,
                            program, -1, globalIdx, fullTriple, false);

        juce::String txLog = "[LIBSYNC] Recall ch" + String (midiCh);

        if (fullTriple)
            txLog += " CC32=" + String (bankLsb);

        if (sendCc0)
            txLog += " CC0=" + String (cc0Value);

        txLog += " PC=" + String (program) + " (globalIdx=" + String (globalIdx) + ")"
                 + " fullTriple=" + String (fullTriple ? 1 : 0);
        Logger::writeToLog (txLog);
    }

    void appendToMidiMonitor (const MidiMessage& message, const char* directionTag)
    {
        String messageText;

        if (message.isSysEx())
        {
            const uint8* d   = message.getSysExData();
            const int    len = message.getSysExDataSize();
            messageText << directionTag << " SysEx: ";

            for (int i = 0; i < len; ++i)
            {
                if (i > 0)
                    messageText << ' ';

                messageText << String::toHexString (d[i]).paddedLeft ('0', 2);
            }
        }
        else
        {
            messageText << directionTag << ' ' << message.getDescription();
        }

        messageText << newLine;

        const ScopedLock sl (midiMonitorLock);
        midiMonitor.insertTextAtCaret (messageText);
        midiMonitorCharCount += messageText.length();
        midiMonitor.moveCaretToEnd();
    }

    /** Thread-safe SysEx out: hardware + file log; monitor update always on UI thread. */
    void sendSysexHardware (const MidiMessage& msg)
    {
        if (! msg.isSysEx())
            return;

        boolStopReceive = true;

        bool sentAny = false;
        juce::String loggedPort;

        {
            const juce::ScopedLock sl (midiMonitorLock);

            for (auto& midiOutput : midiOutputs)
                if (midiOutput->outDevice.get() != nullptr)
                {
                    midiOutput->outDevice->sendMessageNow (msg);
                    sentAny = true;

                    if (loggedPort.isEmpty())
                        loggedPort = midiOutput->name;
                }
        }

        boolStopReceive = false;

        if (sentAny)
        {
            Logger::writeToLog ("[MIDI] TX port=" + loggedPort
                                + " " + msg.getDescription());
            MidiStreamLogger::logOutgoing (msg, loggedPort);

            if (juce::MessageManager::existsAndIsCurrentThread())
                appendToMidiMonitor (msg, "[TX]");
            else
                juce::MessageManager::callAsync ([this, msg]
                {
                    appendToMidiMonitor (msg, "[TX]");
                });
        }
        else
        {
            static bool loggedNoMidiOut = false;

            if (! loggedNoMidiOut)
            {
                loggedNoMidiOut = true;
                Logger::writeToLog ("[MIDI] No output port open — open Setting tab, "
                                    "select your SY99 in MIDI Output list");
            }
        }
    }

    bool sendToOutputs (const MidiMessage& msg)
    {
        if (msg.isSysEx())
        {
            sendSysexHardware (msg);
            return true;
        }

        boolStopReceive = true;

        bool sentAny = false;
        juce::String loggedPort;

        {
            const juce::ScopedLock sl (midiMonitorLock);

            for (auto& midiOutput : midiOutputs)
                if (midiOutput->outDevice.get() != nullptr)
                {
                    midiOutput->outDevice->sendMessageNow (msg);
                    sentAny = true;

                    if (loggedPort.isEmpty())
                        loggedPort = midiOutput->name;
                }
        }

        if (sentAny)
        {
            Logger::writeToLog ("[MIDI] TX port=" + loggedPort
                                + " " + msg.getDescription());
            appendToMidiMonitor (msg, "[TX]");
        }

        boolStopReceive = false;
        return sentAny;
    }

    // THROTTLE DEBOUNCE: called by ThrottleFlushTimer when the throttle window
    // has elapsed and a value is still pending. Sends the most recent suppressed
    // parameter SysEx and refreshes the throttle/echo timestamps.
    void flushPendingSysex()
    {
        throttleTimer.stopTimer();
        if (! hasPending)
            return;

        const juce::uint32 now = juce::Time::getMillisecondCounter();
        lastSysexSendTime = now;

        // Refresh echo guard with the same address so the (almost immediate)
        // reflection from MIDI Thru does not bounce back into valueSysexIn.
        echoGuard[echoGuardIndex].addr[0] = pendingSysex[3];
        echoGuard[echoGuardIndex].addr[1] = pendingSysex[4];
        echoGuard[echoGuardIndex].addr[2] = pendingSysex[5];
        echoGuard[echoGuardIndex].addr[3] = pendingSysex[6];
        echoGuard[echoGuardIndex].sentAt  = now;
        echoGuardIndex = (echoGuardIndex + 1) % kEchoGuardSize;

        MidiMessage m = MidiMessage::createSysExMessage (pendingSysex, 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
        hasPending = false;
    }

    void sendParamSysexFrame (const uint8_t sysexdata[9])
    {
        uint8_t frame[9];
        memcpy (frame, sysexdata, 9);

        const juce::uint32 now = juce::Time::getMillisecondCounter();

        if (now - lastSysexSendTime < 33)
        {
            for (int i = 0; i < 9; ++i)
                pendingSysex[i] = frame[i];

            hasPending = true;
            throttleTimer.startTimer (35);
            return;
        }

        lastSysexSendTime = now;
        hasPending = false;
        throttleTimer.stopTimer();

        echoGuard[echoGuardIndex].addr[0] = frame[3];
        echoGuard[echoGuardIndex].addr[1] = frame[4];
        echoGuard[echoGuardIndex].addr[2] = frame[5];
        echoGuard[echoGuardIndex].addr[3] = frame[6];
        echoGuard[echoGuardIndex].sentAt  = now;
        echoGuardIndex = (echoGuardIndex + 1) % kEchoGuardSize;

        MidiMessage m = MidiMessage::createSysExMessage (frame, 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }

    void sendFeedbackCc (int channel, int cc, int value, const juce::String& portMatch)
    {
        boolStopReceive = true;

        for (auto& midiOutput : midiOutputs)
        {
            if (midiOutput->outDevice.get() == nullptr)
                continue;

            if (portMatch.isNotEmpty()
                && ! midiOutput->outDevice->getName().containsIgnoreCase (portMatch))
                continue;

            const MidiMessage ccMsg = MidiMessage::controllerEvent (channel, cc, value);
            midiOutput->outDevice->sendMessageNow (ccMsg);
        }

        boolStopReceive = false;
    }
    
    //==============================================================================
    bool hasDeviceListChanged (const StringArray& deviceNames, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
        : midiOutputs;
        
        if (deviceNames.size() != midiDevices.size())
            return true;
        
        for (auto i = 0; i < deviceNames.size(); ++i)
            if (deviceNames[i] != midiDevices[i]->name)
                return true;
        
        return false;
    }
    
    ReferenceCountedObjectPtr<MidiDeviceListEntry> findDeviceWithName (const String& name, bool isInputDevice) const
    {
        const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
        : midiOutputs;
        
        for (auto midiDevice : midiDevices)
            if (midiDevice->name == name)
                return midiDevice;
        
        return nullptr;
    }
    
    void closeUnpluggedDevices (StringArray& currentlyPluggedInDevices, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
        : midiOutputs;
        
        for (auto i = midiDevices.size(); --i >= 0;)
        {
            auto& d = *midiDevices[i];
            
            if (! currentlyPluggedInDevices.contains (d.name))
            {
                if (isInputDevice ? d.inDevice .get() != nullptr
                    : d.outDevice.get() != nullptr)
                    closeDevice (isInputDevice, i);
                
                midiDevices.remove (i);
            }
        }
    }
    
    void updateDeviceList (bool isInputDeviceList)
    {
        auto newDevices = isInputDeviceList ? MidiInput::getAvailableDevices()
                                            : MidiOutput::getAvailableDevices();

        StringArray newDeviceNames;
        for (auto& d : newDevices)
            newDeviceNames.add (d.name);

        if (hasDeviceListChanged (newDeviceNames, isInputDeviceList))
        {
            ReferenceCountedArray<MidiDeviceListEntry>& midiDevices
            = isInputDeviceList ? midiInputs : midiOutputs;
            
            closeUnpluggedDevices (newDeviceNames, isInputDeviceList);
            
            ReferenceCountedArray<MidiDeviceListEntry> newDeviceList;
            
            // add all currently plugged-in devices to the device list
            for (auto& newDevice : newDevices)
            {
                MidiDeviceListEntry::Ptr entry = findDeviceWithName (newDevice.name, isInputDeviceList);
                
                if (entry == nullptr)
                    entry = new MidiDeviceListEntry (newDevice.name, newDevice.identifier);
                else
                    entry->identifier = newDevice.identifier;
                
                newDeviceList.add (entry);
            }
            
            // actually update the device list
            midiDevices = newDeviceList;
            
            // update the selection status of the combo-box
            if (auto* midiSelector = isInputDeviceList ? midiInputSelector.get() : midiOutputSelector.get())
                midiSelector->syncSelectedItemsWithDeviceList (midiDevices);
        }
    }

    static int findMidiDeviceRowForPersistedId (const ReferenceCountedArray<MidiDeviceListEntry>& devices,
                                                const juce::String& persistedId) noexcept
    {
        if (persistedId.isEmpty())
            return -1;

        for (int i = 0; i < devices.size(); ++i)
        {
            if (devices[i]->identifier == persistedId)
                return i;
        }

        for (int i = 0; i < devices.size(); ++i)
        {
            if (devices[i]->name == persistedId)
                return i;
        }

        return -1;
    }

    void restorePersistedMidiDevicesIfNeeded()
    {
        if (midiDevicesRestoredOnce)
            return;

        juce::StringArray savedInputIds, savedOutputIds;
        midiPersistLoadOpenDeviceIds (savedInputIds, savedOutputIds);

        if (savedInputIds.isEmpty() && savedOutputIds.isEmpty())
        {
            midiDevicesRestoredOnce = true;
            return;
        }

        if (midiInputSelector != nullptr && ! savedInputIds.isEmpty())
        {
            juce::SparseSet<int> rows;

            for (const auto& id : savedInputIds)
            {
                const int row = findMidiDeviceRowForPersistedId (midiInputs, id);

                if (row >= 0 && midiInputs[row]->inDevice.get() == nullptr)
                    rows.addRange (juce::Range<int> (row, row + 1));
            }

            if (! rows.isEmpty())
                midiInputSelector->setSelectedRows (rows, juce::sendNotification);
        }

        if (midiOutputSelector != nullptr && ! savedOutputIds.isEmpty())
        {
            juce::SparseSet<int> rows;

            for (const auto& id : savedOutputIds)
            {
                const int row = findMidiDeviceRowForPersistedId (midiOutputs, id);

                if (row >= 0 && midiOutputs[row]->outDevice.get() == nullptr)
                    rows.addRange (juce::Range<int> (row, row + 1));
            }

            if (! rows.isEmpty())
                midiOutputSelector->setSelectedRows (rows, juce::sendNotification);
        }

        midiDevicesRestoredOnce = true;
        juce::Logger::writeToLog ("[MidiPersist] restored in="
                                    + juce::String (savedInputIds.size())
                                    + " out=" + juce::String (savedOutputIds.size()));

        juce::MessageManager::callAsync ([this]
        {
            flushPendingLibraryRecall();
            sy99DrainPendingBankClick0040Fetch();
        });
    }

    void persistOpenMidiDevices() noexcept
    {
        juce::StringArray inputIds, outputIds;

        for (auto& entry : midiInputs)
        {
            if (entry->inDevice.get() == nullptr)
                continue;

            inputIds.add (entry->identifier.isNotEmpty() ? entry->identifier : entry->name);
        }

        for (auto& entry : midiOutputs)
        {
            if (entry->outDevice.get() == nullptr)
                continue;

            outputIds.add (entry->identifier.isNotEmpty() ? entry->identifier : entry->name);
        }

        midiPersistSaveOpenDevices (inputIds, outputIds);
    }
    
    //==============================================================================
    void addLabelAndSetStyle (Label& label)
    {
        label.setFont (Font (15.00f, Font::plain));
        label.setJustificationType (Justification::centredLeft);
        label.setEditable (false, false, false);
        label.setColour (TextEditor::textColourId, Colours::black);
        label.setColour (TextEditor::backgroundColourId, Colour (0x00000000));
        
        addAndMakeVisible (label);
    }

    void syncMonitorSysExOnlyButtonLook() noexcept
    {
        btShowSysExOnly.setColour (ToggleButton::textColourId,
                                   showSysExOnly ? Colours::darkorange : Colours::white);
    }

    void applyMonitorSysExOnlyToggleUi() noexcept
    {
        btShowSysExOnly.setToggleState (showSysExOnly, juce::dontSendNotification);
        syncMonitorSysExOnlyButtonLook();
        btShowSysExOnly.repaint();
    }
    
    void comboBoxChanged	(	ComboBox * 	comboBoxThatHasChanged	) override
    {
        if(comboBoxThatHasChanged == &comboFoot)
        {
        uint8 sysexdata[9] = { 0x43, sysexEngine, 0x34, 0x0f, 0x00, 0x00, 0x2d, 0x00, 0x00 };
        sysexdata[8] = comboFoot.getSelectedItemIndex()+1;
        MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
        }
        if(comboBoxThatHasChanged == &comboMod)
        {
            uint8 sysexdata[9] = { 0x43, DeviceModel::getInstance().getSysExDeviceID(), 0x34, 0x0f, 0x00, 0x00, 0x2c, 0x00, 0x00 };
            sysexdata[8] = comboMod.getSelectedItemIndex()+1;
            MidiMessage m = MidiMessage::createSysExMessage(sysexdata, 9);
            m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
            sendToOutputs (m);
        }
        //MidiMessage m (MidiMessage::noteOn (1, 64, 120));
        // create and send an OSC message with an address and a float value:
//        if (! sender.send (adresseOscFoot, (int) comboFoot.getSelectedItemIndex()+1)) // [5]
//            Logger::writeToLog ("OSC erreur");;
//        if (! sender.send (adresseOscMod, (int) comboMod.getSelectedItemIndex()+1)) // [5]
//            Logger::writeToLog ("OSC erreur");;
    }
    
    void loadData()
    {
        
        
        auto tableFile = BinaryData::TableData_xml;
        
        tutorialData = XmlDocument::parse (tableFile);
        
        dataList   = tutorialData->getChildByName ("DATA");
        columnList = tutorialData->getChildByName ("HEADERS");
        
        numRows = dataList->getNumChildElements();
        
    }
    
    String getAttributeNameForColumnId (const int columnId) const
    {
        forEachXmlChildElement (*columnList, columnXml)
        {
            if (columnXml->getIntAttribute ("columnId") == columnId)
                return columnXml->getStringAttribute ("name");
        }
        
        return {};
    }
    //==============================================================================
    Label midiInputLabel    { "Midi Input Label", TRANS( "MIDI Input:" )};
    Label midiOutputLabel   { "Midi Output Label", TRANS("MIDI Output:") };
    Label incomingMidiLabel { "Incoming Midi Label", TRANS("Received MIDI messages:") };
    
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent midiKeyboard;
    TextEditor midiMonitor  { TRANS("MIDI Monitor" )};
    TextButton pairButton   { TRANS("MIDI Bluetooth devices...") };
    
    TextButton  btBulk {"Bulk Protect"};
    ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;
    std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;

    CriticalSection midiMonitorLock;
    Array<MidiMessage> incomingMessages;
    std::unique_ptr<DemoTabbedComponent> tabs;
    Image imgBack;
    OSCSender   senderMidiIn;

    uint8 data[12];
    
    // controler
    ComboBox comboFoot { "Pedale de sustain"};
    ComboBox comboMod  { "Molette de modulation" };
    Label   labelFoot { "","Pedale de sustain :"};
    Label   labelMod { "","Molette de modulation :"};
    std::unique_ptr<XmlElement> tutorialData;
    XmlElement* columnList = nullptr;
    XmlElement* dataList = nullptr;
    int numRows = 0;
    // MIDI log filter state — read on MIDI thread, written on message thread.
    // Simple bool is acceptable here; follows same pattern as boolStopReceive.
    bool showRealtimeMidi    = false;
    bool showSysExOnly       = false;
    int  sysExMsgCounter     = 0;
    int  midiMonitorCharCount = 0;  // tracks accumulated chars; reset on clear/auto-trim

    TextButton   btShowRealtime  { "Show realtime" };
    ToggleButton btShowSysExOnly { "SysEx only" };
    TextButton btCopyMonitor   { "Copy" };
    TextButton btClearMonitor  { "Clear" };

    // THROTTLE: 30msg/sec limit — timestamp of the last outgoing parameter SysEx
    juce::uint32 lastSysexSendTime = 0;

    // THROTTLE DEBOUNCE: one pending suppressed parameter-change SysEx.
    // If no newer value replaces it within ~33ms, the timer flushes it so the
    // final slider value still reaches the synth.
    uint8 pendingSysex[9] {};
    bool  hasPending = false;

    struct ThrottleFlushTimer : public juce::Timer
    {
        MidiDemo* owner = nullptr;
        void timerCallback() override
        {
            if (owner != nullptr)
                owner->flushPendingSysex();
        }
    };
    ThrottleFlushTimer throttleTimer;

    // ECHO GUARD: ring buffer of recently sent SysEx address bytes [3..6].
    // Incoming reflections matching one of these within 50ms are ignored.
    struct EchoEntry { uint8 addr[4]; juce::uint32 sentAt; };
    static constexpr int kEchoGuardSize = 16;
    EchoEntry echoGuard[kEchoGuardSize] {};
    int echoGuardIndex = 0;

    bool midiDevicesRestoredOnce = false;
    bool pendingLibraryRecallValid = false;
    int pendingLibraryRecallIdx = -1;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiDemo)
};

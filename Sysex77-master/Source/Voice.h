/*
 ==============================================================================
 
 Voice.h
 Created: 13 Nov 2018 5:16:09pm
 Author:  Seb & Ludo
 
 ==============================================================================
 */
#pragma once
#include "Element.h"
#include "Volume.h"
#include "VoiceMode.h"
//==============================================================================
struct VoicePage   : public Component, public Slider::Listener, public ComboBox::Listener, public TextEditor::Listener, public TextButton::Listener,public ChangeListener, public ChangeBroadcaster, public Value::Listener, public ValueTree::Listener, public KeyListener , public Timer
{
    VoicePage()
    {
        
        setOpaque(false);
        
        editAfm1.setElementNumber(1,undoManager);
        addAndMakeVisible(editAfm1);
        editAfm1.setAlwaysOnTop(true);
        editAfm1.setVisible(false);
        

        editAfm2.setElementNumber(2,undoManager);
        addAndMakeVisible(editAfm2);
        editAfm2.setAlwaysOnTop(true);
        editAfm2.setVisible(false);
        
        editAfm3.setElementNumber(3,undoManager);
        addAndMakeVisible(editAfm3);
        editAfm3.setAlwaysOnTop(true);
        editAfm3.setVisible(false);
        
        editAfm4.setElementNumber(4,undoManager);
        addAndMakeVisible(editAfm4);
        editAfm4.setAlwaysOnTop(true);
        editAfm4.setVisible(false);
        
        editFilter1.setElementNumber(1, undoManager);
        addAndMakeVisible(editFilter1);
        editFilter1.setAlwaysOnTop(true);
        editFilter1.setVisible(false);
        
        editFilter2.setElementNumber(2, undoManager);
        addAndMakeVisible(editFilter2);
        editFilter2.setAlwaysOnTop(true);
        editFilter2.setVisible(false);
        
        editFilter3.setElementNumber(3, undoManager);
        addAndMakeVisible(editFilter3);
        editFilter3.setAlwaysOnTop(true);
        editFilter3.setVisible(false);
        
        editFilter4.setElementNumber(4, undoManager);
        addAndMakeVisible(editFilter4);
        editFilter4.setAlwaysOnTop(true);
        editFilter4.setVisible(false);
        
        editWave1.setElementNumber(1, undoManager);
        addAndMakeVisible(editWave1);
        editWave1.setAlwaysOnTop(true);
        editWave1.setVisible(false);
        
        editWave2.setElementNumber(2, undoManager);
        addAndMakeVisible(editWave2);
        editWave2.setAlwaysOnTop(true);
        editWave2.setVisible(false);
        
        editWave3.setElementNumber(3, undoManager);
        addAndMakeVisible(editWave3);
        editWave3.setAlwaysOnTop(true);
        editWave3.setVisible(false);
        
        editWave4.setElementNumber(4, undoManager);
        addAndMakeVisible(editWave4);
        editWave4.setAlwaysOnTop(true);
        editWave4.setVisible(false);
        
        

        addAndMakeVisible(comboMode);
        
        addAndMakeVisible(element1);
        addAndMakeVisible(element2);
        addAndMakeVisible(element3);
        addAndMakeVisible(element4);
        
        element1.setOpNumber(1, undoManager);
        element2.setOpNumber(2, undoManager);
        element3.setOpNumber(3, undoManager);
        element4.setOpNumber(4, undoManager);
        
        element1.addChangeListener(this);
        element2.addChangeListener(this);
        element3.addChangeListener(this);
        element4.addChangeListener(this);
        
        element1.elementValue.addListener(this);
        element2.elementValue.addListener(this);
        element3.elementValue.addListener(this);
        element4.elementValue.addListener(this);
        
        comboMode.getSelectedIdAsValue().referTo(valueTreeVoice.getPropertyAsValue(IDs::VOICEMODE, &undoManager));
        comboMode.setEditableText (false);
        comboMode.setJustificationType (Justification::centredBottom);
        comboMode.addListener(this);
        comboMode.addItem("1 AFM MONO", 1);
        comboMode.addItem("2 AFM MONO", 2);
        comboMode.addItem("4 AFM MONO", 3);
        comboMode.addItem("1 AFM POLY", 4);
        comboMode.addItem("2 AFM POLY", 5);
        comboMode.addItem("1 AWM POLY", 6);
        comboMode.addItem("2 AWM POLY", 7);
        comboMode.addItem("4 AWM POLY", 8);
        comboMode.addItem("1 AFM & 1 AWM POLY", 9);
        comboMode.addItem("2 AFM & 2 AWM POLY", 10);
        comboMode.addItem("DRUM SET", 11);
        comboMode.setSelectedId(1);

        // ELMODE receive: listen to incoming SysEx; match address 02 00 00 00.
        valueSysexIn.addListener(this);
        // [LIBSYNC] Library slot name → editName (no VNAM echo to synth).
        bankSelectedVoiceNameValue.addListener(this);

        // Voice Common Params panel (Group B: 02 00 00 28..43)
        addAndMakeVisible (commonPanel);
        buildCommonPanel();
        
       
 
    
        addAndMakeVisible(editName);
        editName.setText("INIT");
        editName.setMultiLine(false);
        editName.setInputRestrictions(10);
        editName.setJustification(Justification::centred);
        //      editName.setColour(TextEditor::ColourIds::textColourId, SYColText);
        editName.addListener(this);
        
    int sysexdata[9] = { 0x43, 0X10, 0x34, 0x0d, 0x00, 0x00, 0x32, 0x00, 0x7f };
        op1.setMidiSysex(sysexdata);
        addAndMakeVisible(op1);
        op1.setTextOnOff("1", "1");
        
        addBtAndMakeStyle(op2);
        addBtAndMakeStyle(op3);
        addBtAndMakeStyle(op4);
        addLabelStyle(labelOpOn);
        addLabelStyle(labelMode);
        addLabelStyle(labelName);
        
// init master volume slider
//   { 0x43, 0x10, 0x34, 0x02, 0x00, 0x00, 0x3f, 0x00, 0x00 };
        sysexdata[3] = 0x02;
        sysexdata[6] = 0x3f;
        sliderMaster.setMidiSysex(sysexdata);
        sliderMaster.setRangeAndRound(0, 127, 127) ;
        sliderMaster.setSliderStyle(MidiSlider::SliderStyle::LinearHorizontal);
        sliderMaster.setPopupDisplayEnabled(true, true, nullptr);
        
        addAndMakeVisible(sliderMaster);
        labelMasterVolume.attachToComponent(&sliderMaster, true);
        
        Value valueCommonVolume = valueTreeVoice.getPropertyAsValue(IDs::COMMONVOLUME.toString(), &undoManager);
        sliderMaster.getValueObject().referTo(valueCommonVolume);

        valueTreeVoice.addListener(this);
        addKeyListener(this); //to manage undo redo key
        startTimer(500);
        // specify here where to send OSC messages to: host URL and UDP port number
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        
  
        
    }
    ~VoicePage()
    {
        stopTimer();
          valueTreeVoice.removeListener(this);
          removeKeyListener(this);
        element1.removeChangeListener(this);
        element2.removeChangeListener(this);
        element3.removeChangeListener(this);
        element4.removeChangeListener(this);
        editName.removeListener(this);
        comboMode.removeListener(this);
        valueSysexIn.removeListener(this);
        bankSelectedVoiceNameValue.removeListener(this);
    }
    
    #include "ValueTrees.h"
    
        virtual void broughtToFront() override
    {
    Logger::writeToLog("Voice: BroughtToFront");
   
        editWave1.setVisible(false);
        editAfm1.setVisible(false);
        editFilter1.setVisible(false);
    
        editWave2.setVisible(false);
        editAfm2.setVisible(false);
        editFilter2.setVisible(false);

        editWave2.setVisible(false);
        editAfm2.setVisible(false);
        editFilter2.setVisible(false);

        editWave2.setVisible(false);
        editAfm2.setVisible(false);
        editFilter2.setVisible(false);

        
    }
    
    void changeListenerCallback (ChangeBroadcaster* source) override
    {
       
        
        Logger::writeToLog("Voice: changeListener");
    //    Logger::writeToLog(source);
    //    test.setVisible(true);
        
    }
    void paint (Graphics& g) override
    {
            Logger::writeToLog("Voice: Paint");
    /*
        
        // g.setColour(Colours::white);
        ColourGradient myGradient {	Colour(0xff404040),static_cast<float>(getWidth()/2),0,Colour(0xff000000),static_cast<float>(getWidth()/2),static_cast<float>(getHeight()),false };
        g.setGradientFill(myGradient);
        //        g.setColour(Colour(0x10000000));
        g.fillAll();
        */
    }
    void valueChanged(Value & value) override
    {
        if (value.refersToSameSourceAs (bankSelectedVoiceNameValue))
        {
            const String name = value.getValue().toString();
            receivingVNAM = true;
            editName.setText (name, juce::dontSendNotification);
            receivingVNAM = false;
            return;
        }

        if (value.refersToSameSourceAs(valueSysexIn))
        {
            auto incoming = value.getValue();
            const int b3 = (int) incoming[0];
            const int b4 = (int) incoming[1];
            const int b5 = (int) incoming[2];
            const int b6 = (int) incoming[3];
            const int b8 = (int) incoming[5];

            // ELMODE receive: address 02 00 00 00 → update comboMode
            if (b3 == 0x02 && b4 == 0x00 && b5 == 0x00 && b6 == 0x00)
            {
                if (b8 >= 0 && b8 <= 10)
                    comboMode.setSelectedId (b8 + 1, juce::dontSendNotification);
                return;
            }

            // VNAM receive: addresses 02 00 00 01..0A → 10-char voice name buffer.
            // Programmatic editName.setText uses dontSendNotification so we do not
            // re-trigger textEditorReturnKeyPressed (no outbound VNAM echo).
            if (b3 == 0x02 && b4 == 0x00 && b5 == 0x00
                && b6 >= 0x01 && b6 <= 0x0A)
            {
                const int idx = b6 - 0x01;
                voiceNameBuffer[idx] = (char) (b8 & 0x7F);
                receivingVNAM = true;
                String name (voiceNameBuffer, sizeof (voiceNameBuffer));
                editName.setText (name.trimEnd(), juce::dontSendNotification);
                receivingVNAM = false;
                return;
            }
            return;
        }

        if(value.refersToSameSourceAs(element1.elementValue))
        {
            Logger::writeToLog("Value change element1");
        if(element1.elementValue == Element::commande::WaveEdit)
            editWave1.setVisible(true);
        if(element1.elementValue == Element::commande::AfmEdit)
                editAfm1.setVisible(true);
        if(element1.elementValue == Element::commande::FilterEdit)
                editFilter1.setVisible(true);
        if(element1.elementValue == Element::commande::VolumeEdit)
        {
            //editVolume1.setVisible(true);
            editWave1.setVisible(true);
            editWave1.setTabVolume();
        }
        if(element1.elementValue == Element::commande::VolumeAFM)
            {
                //editVolume1.setVisible(true);
                editAfm1.setVisible(true);
                editAfm1.setTabVolume();
            }
            
             element1.elementValue =0;
        }



        
        if(value.refersToSameSourceAs(element2.elementValue))
        {
                      Logger::writeToLog("Value change element2");
            if(element2.elementValue == Element::commande::WaveEdit)
                editWave2.setVisible(true);
            if(element2.elementValue == Element::commande::AfmEdit)
                editAfm2.setVisible(true);
            if(element2.elementValue == Element::commande::FilterEdit)
                editFilter2.setVisible(true);
            if(element2.elementValue == Element::commande::VolumeEdit)
            {
                editWave2.setVisible(true);
                editWave2.setTabVolume();
            }
            if(element2.elementValue == Element::commande::VolumeAFM)
            {
                //editVolume1.setVisible(true);
                editAfm2.setVisible(true);
                editAfm2.setTabVolume();
            }
             element2.elementValue =0;
        }
 
        if(value.refersToSameSourceAs(element3.elementValue))
        {
            Logger::writeToLog("Value change element3");
            if(element3.elementValue == Element::commande::WaveEdit)
                editWave3.setVisible(true);
            if(element3.elementValue == Element::commande::AfmEdit)
                editAfm3.setVisible(true);
            if(element3.elementValue == Element::commande::FilterEdit)
                editFilter3.setVisible(true);
            if(element3.elementValue == Element::commande::VolumeEdit)
            {
                editWave3.setVisible(true);
                editWave3.setTabVolume();
            }
            if(element3.elementValue == Element::commande::VolumeAFM)
            {
                //editVolume1.setVisible(true);
                editAfm3.setVisible(true);
                editAfm3.setTabVolume();
            }
             element3.elementValue =0;
        }
        
        if(value.refersToSameSourceAs(element4.elementValue))
        {
            Logger::writeToLog("Value change element4");
            if(element4.elementValue == Element::commande::WaveEdit)
                editWave4.setVisible(true);
            if(element4.elementValue == Element::commande::AfmEdit)
                editAfm4.setVisible(true);
            if(element4.elementValue == Element::commande::FilterEdit)
                editFilter4.setVisible(true);
            if(element4.elementValue == Element::commande::VolumeEdit)
            {
                editWave4.setVisible(true);
                editWave4.setTabVolume();
            }
            if(element4.elementValue == Element::commande::VolumeAFM)
            {
                //editVolume1.setVisible(true);
                editAfm4.setVisible(true);
                editAfm4.setTabVolume();
            }
             element4.elementValue =0;
        }
    }
    void buttonClicked (Button* button) override
    {
            Logger::writeToLog("Voice: ButtonClicked");
    }
    void textEditorReturnKeyPressed	(	TextEditor & 	editText	) override
    {
        Logger::writeToLog("Voice TextEditor return");
        // Programmatic updates from VNAM receive must never send back.
        if (receivingVNAM)
            return;
        String str = editText.getText();
        // VNAM*: 10 SysEx messages, address byte [6] = 0x01 .. 0x0A, value = char.
        // Pad short names with space (0x20) so the synth display is fully cleared.
        uint8 sysexdata[9] = { 0x43, sysexEngine, 0x34, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00 };
        const int len = str.length();
        const char* data = str.toRawUTF8();
        oscMidiMessage.clear();
        for (auto i = 0; i < 10; ++i)
        {
            sysexdata[6] = (uint8) (0x01 + i);
            sysexdata[8] = (uint8) (i < len ? data[i] : 0x20);
            oscMidiMessage.set (i, MidiMessage::createSysExMessage (sysexdata, 9));
        }
        if (! sender.send (oscSendMidiMessage, (int) 10)) // [5]
            Logger::writeToLog ("OSC erreur voice VNAM");
    }
  

    
    void textEditorFocusLost	(	TextEditor & 		) override
    {
        Logger::writeToLog("Voice TextEditor lostFocus");
    }
    void 	sliderValueChanged (Slider *slider) override
    {
    
        
        
 }
    
    void comboBoxChanged	(	ComboBox * 	comboBoxThatHasChanged	) override
    {
        Logger::writeToLog("Voice comboBox Changed");
        if(comboBoxThatHasChanged == &comboMode)
        {
            if (! sender.send (adresseOpMode, (int) comboMode.getSelectedItemIndex())) // [5]
                Logger::writeToLog ("OSC erreur voice opMode");;
 
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMMono1)
            {
                setNombreElements(Element::mode::AFMmono);
                element1.setOpMode(1);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMMono2)
            {
                setNombreElements(2);
                                element1.setOpMode(Element::mode::AFMmono);
                                element2.setOpMode(Element::mode::AFMmono);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMMono4)
            {
                setNombreElements(4);
                element1.setOpMode(Element::mode::AFMmono);
                element2.setOpMode(Element::mode::AFMmono);
                element3.setOpMode(Element::mode::AFMmono);
                element4.setOpMode(Element::mode::AFMmono);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFM2AWM2)
            {
                setNombreElements(4);
                element1.setOpMode(Element::mode::AFMPoly);
                element2.setOpMode(Element::mode::AFMPoly);
                element3.setOpMode(Element::mode::AWM);
                element4.setOpMode(Element::mode::AWM);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AWMPoly4)
            {
                setNombreElements(4);
                element1.setOpMode(Element::mode::AWM);
                element2.setOpMode(Element::mode::AWM);
                element3.setOpMode(Element::mode::AWM);
                element4.setOpMode(Element::mode::AWM);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AWMPoly1)
            {
                setNombreElements(1);
                element1.setOpMode(Element::mode::AWM);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AWMPoly2)
            {
                setNombreElements(2);
                element1.setOpMode(Element::mode::AWM);
                element2.setOpMode(Element::mode::AWM);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMPoly2)
            {
                setNombreElements(2);
                element1.setOpMode(Element::mode::AFMPoly);
                element2.setOpMode(Element::mode::AFMPoly);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFM1AWM1)
            {
                setNombreElements(2);
                element1.setOpMode(Element::mode::AFMPoly);
                element2.setOpMode(Element::mode::AWM);            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMPoly1)
            {
                setNombreElements(1);
                element1.setOpMode(Element::mode::AFMPoly);
            }
            if(comboMode.getSelectedItemIndex() == voiceMode::AFMPoly2)
            {
                setNombreElements(2);
                element1.setOpMode(Element::mode::AFMPoly);
                element2.setOpMode(Element::mode::AFMPoly);
            }
        }

    }
void setNombreElements (int nombre)
    {
        nombreElements = nombre;
        element2.setVisible(false);
        element3.setVisible(false);
        element4.setVisible(false);
        if(nombre > 1)
        element2.setVisible(true);
        if(nombre > 2)
        {
        element3.setVisible(true);
        element4.setVisible(true);
        }
        
        Logger::writeToLog("setNombre elements");
        resized();
        
    }

    void resized() override
    {
       Logger::writeToLog("Voice: Resized");
        auto boundsZone = getBounds();
        boundsZone.setY(0);
        //boundsZone.setHeight(getHeight()+20);
        editWave1.setBounds(boundsZone);
        editWave2.setBounds(boundsZone);
        editWave3.setBounds(boundsZone);
        editWave4.setBounds(boundsZone);
       
        editFilter1.setBounds(boundsZone);
        editFilter2.setBounds(boundsZone);
        editFilter3.setBounds(boundsZone);
        editFilter4.setBounds(boundsZone);
        editAfm1.setBounds(boundsZone);
        editAfm2.setBounds(boundsZone);
        editAfm3.setBounds(boundsZone);
        editAfm4.setBounds(boundsZone);
        
        grid = getWidth()/10;
        auto wGrid = (getWidth()-20) * 0.7;
        auto hGrid = (getHeight() - 50)/nombreElements;
        comboMode.setBounds(getWidth()/2, 4, 160, 24);
        
// Redraw celon le nombre d'elements
     
        element1.setBounds(10, 48, wGrid, hGrid);
        element2.setBounds(10, 48 + hGrid,wGrid, hGrid );
        element3.setBounds(10,  48 + (hGrid * 2),wGrid, hGrid);
        element4.setBounds(10,  48 + (hGrid*3), wGrid,hGrid) ;
        
        op1.setBounds (10,4,24,24);
        op2.setBounds (10+24, 4,24,24);
        op3.setBounds (10+48, 4,24,24);
        op4.setBounds (10+72, 4,24,24);
        editName.setBounds(20 + 96, 4, 100, 24);
        labelName.setBounds(116,20,100,24);
        
        sliderMaster.setBounds(getWidth() -106, 4, 96, 20);
        
        labelMode.setBounds(getWidth()/2 + 24, 20, 96, 24);
        labelOpOn.setBounds(10, 20, 96, 24);
        labelMasterVolume.setBounds(getWidth()-106, 20, 96, 24);

        // Voice Common Params panel — right column under the master volume slider.
        const int panelX = (int) (wGrid + 20);
        const int panelY = 48;
        const int panelW = getWidth() - panelX - 10;
        const int panelH = getHeight() - panelY - 10;
        commonPanel.setBounds (panelX, panelY, panelW, panelH);
        layoutCommonPanel();
    }
    void addBtAndMakeStyle (TextButton& textButton)
    {
        textButton.setClickingTogglesState(true);
        textButton.setColour(TextButton::ColourIds::buttonOnColourId, Colours::red);
        addAndMakeVisible (textButton);
    }
    void addAndMakePitch ( Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setLookAndFeel(&myLookAndFeel);
        slider.addListener(this);
        slider.setRange(0, 127);
        slider.setNumDecimalPlacesToDisplay(0);
    }
    void addSliderStyleV (Slider& slider)
    {
        addAndMakeVisible(slider);
        slider.setPopupDisplayEnabled(true, true, this);
        slider.setPopupDisplayEnabled(true, true, this);
        slider.setColour(Slider::ColourIds::thumbColourId, Colours::red);
        slider.setColour(Slider::ColourIds::trackColourId, Colours::darkorange);
        slider.setRange(0, 127);
        slider.setNumDecimalPlacesToDisplay(0);
        slider.addListener(this);
    }
    void addButtonState (TextButton& bt)
    {
        addAndMakeVisible(bt);
        bt.setColour(TextButton::ColourIds::buttonOnColourId, Colours::darkorange);
        bt.setClickingTogglesState(true);
        bt.addListener(this);
    }
    void addLabelStyle (Label& label)
    {
        label.setJustificationType(Justification::centredBottom);
        label.setColour(Label::ColourIds::textColourId, Colours::darkorange);
        addAndMakeVisible (label);
    }

    enum voiceMode
    {
        AFMMono1 = 0,
        AFMMono2 = 1,
        AFMMono4 = 2,
        AFMPoly1 = 3,
        AFMPoly2 = 4,
        AWMPoly1 = 5,
        AWMPoly2 = 6,
        AWMPoly4 = 7,
        AFM1AWM1 = 8,
        AFM2AWM2 = 9,
        DrumSet = 10,
        
    };
    
  
    WaveVue editWave1;
    WaveVue editWave2;
    WaveVue editWave3;
    WaveVue editWave4;


    
    FilterVue editFilter1;
    FilterVue editFilter2;
    FilterVue editFilter3;
    FilterVue editFilter4;
    
    AFMVue editAfm1;
    AFMVue editAfm2;
    AFMVue editAfm3;
    AFMVue editAfm4;
    
    int grid;
    MidiSlider sliderMaster;
    
    ComboBox    comboMode;
    TextEditor  editName {TRANS("Edit Name")};
    MidiButton op1;
    TextButton op2 {"2"};
    TextButton op3{"3"};
    TextButton op4{"4"};
    

    Element element1;
    Element element2;
    Element element3;
    Element element4;
    
    Label labelOpOn{"-Operateurs-", "Operateurs On-Off"};
    Label labelName {" ", TRANS("Name")};
    Label labelMode{"", "Operateurs Mode"};

    
    Label   labelMasterVolume{"M",TRANS("Master Volume")};
    int nombreElements = 1;
    OSCSender sender;  // [2]
    CustomLookAndFeel myLookAndFeel;

    // VNAM receive: 10-char buffer fed by incoming SysEx 02 00 00 01..0A.
    // Initialised to spaces so any partial update still renders cleanly.
    char voiceNameBuffer[10] { ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
    bool receivingVNAM = false;

    // -------- Voice Common Params (addresses 02 00 00 28 .. 43) --------
    // Continuous params → MidiSlider; enum/assign params → MidiCombo.
    Component   commonPanel;
    Label       labelCommon { "labelCommon", TRANS ("Common Params") };
    // 28 WPBR, 29 ATPBR, 2B PMRNG, 2D AMRNG, 2F FMRNG, 31 PNLRNG,
    // 33 CORNG, 35 PNBRNG, 37 EGBRNG, 39 WLLML, 3A MCTUN, 3B RNDP, 43 SPTPNT
    MidiSlider  sliderWPBR, sliderATPBR, sliderPMRNG, sliderAMRNG, sliderFMRNG,
                sliderPNLRNG, sliderCORNG, sliderPNBRNG, sliderEGBRNG,
                sliderWLLML, sliderMCTUN, sliderRNDP, sliderSPTPNT;
    Label       labWPBR  { "WPBR",   "WPBR"   };
    Label       labATPBR { "ATPBR",  "ATPBR"  };
    Label       labPMRNG { "PMRNG",  "PMRNG"  };
    Label       labAMRNG { "AMRNG",  "AMRNG"  };
    Label       labFMRNG { "FMRNG",  "FMRNG"  };
    Label       labPNLRNG{ "PNLRNG", "PNLRNG" };
    Label       labCORNG { "CORNG",  "CORNG"  };
    Label       labPNBRNG{ "PNBRNG", "PNBRNG" };
    Label       labEGBRNG{ "EGBRNG", "EGBRNG" };
    Label       labWLLML { "WLLML",  "WLLML"  };
    Label       labMCTUN { "MCTUN",  "MCTUN"  };
    Label       labRNDP  { "RNDP",   "RNDP"   };
    Label       labSPTPNT{ "SPTPNT", "SPTPNT" };
    // 2A PMASN, 2C AMASN, 2E FMASN, 30 PNLASN, 32 COASN, 34 PNBASN,
    // 36 EGBASN, 38 WLASN, 42 AFTMD
    MidiCombo   comboPMASN, comboAMASN, comboFMASN, comboPNLASN, comboCOASN,
                comboPNBASN, comboEGBASN, comboWLASN, comboAFTMD;
    Label       labPMASN { "PMASN",  "PMASN"  };
    Label       labAMASN { "AMASN",  "AMASN"  };
    Label       labFMASN { "FMASN",  "FMASN"  };
    Label       labPNLASN{ "PNLASN", "PNLASN" };
    Label       labCOASN { "COASN",  "COASN"  };
    Label       labPNBASN{ "PNBASN", "PNBASN" };
    Label       labEGBASN{ "EGBASN", "EGBASN" };
    Label       labWLASN { "WLASN",  "WLASN"  };
    Label       labAFTMD { "AFTMD",  "AFTMD"  };

    void setupCommonParam (MidiSlider& s, Label& lab, uint8 NN, int lo, int hi)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x02, 0x00, 0x00, NN, 0x00, 0 };
        s.setMidiSysex (sysex);
        s.setRangeAndRound (lo, hi, lo);
        s.setSliderStyle (Slider::SliderStyle::LinearHorizontal);
        s.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);
        s.setPopupDisplayEnabled (true, true, &commonPanel);
        commonPanel.addAndMakeVisible (s);
        lab.setJustificationType (Justification::centredLeft);
        lab.setColour (Label::textColourId, Colours::darkorange);
        commonPanel.addAndMakeVisible (lab);
    }
    void setupCommonAssign (MidiCombo& c, Label& lab, uint8 NN, int lo, int hi)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x02, 0x00, 0x00, NN, 0x00, 0 };
        c.setMidiSysex (sysex);
        for (int v = lo; v <= hi; ++v)
            c.addItem (String (v), v + 1); // ID = value+1 to avoid 0
        commonPanel.addAndMakeVisible (c);
        lab.setJustificationType (Justification::centredLeft);
        lab.setColour (Label::textColourId, Colours::darkorange);
        commonPanel.addAndMakeVisible (lab);
    }
    void setupAFTMD()
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x02, 0x00, 0x00, 0x42, 0x00, 0 };
        comboAFTMD.setMidiSysex (sysex);
        comboAFTMD.addItem ("all", 1);
        comboAFTMD.addItem ("top", 2);
        comboAFTMD.addItem ("btm", 3);
        comboAFTMD.addItem ("hi",  4);
        comboAFTMD.addItem ("low", 5);
        commonPanel.addAndMakeVisible (comboAFTMD);
        labAFTMD.setJustificationType (Justification::centredLeft);
        labAFTMD.setColour (Label::textColourId, Colours::darkorange);
        commonPanel.addAndMakeVisible (labAFTMD);
    }
    void buildCommonPanel()
    {
        commonPanel.addAndMakeVisible (labelCommon);
        labelCommon.setJustificationType (Justification::centredTop);
        labelCommon.setColour (Label::textColourId, Colours::white);

        setupCommonParam (sliderWPBR,   labWPBR,   0x28,   0,  12);
        setupCommonParam (sliderATPBR,  labATPBR,  0x29, -12,  12);
        setupCommonAssign (comboPMASN,  labPMASN,  0x2A,   0, 121);
        setupCommonParam (sliderPMRNG,  labPMRNG,  0x2B,   0, 127);
        setupCommonAssign (comboAMASN,  labAMASN,  0x2C,   0, 121);
        setupCommonParam (sliderAMRNG,  labAMRNG,  0x2D,   0, 127);
        setupCommonAssign (comboFMASN,  labFMASN,  0x2E,   0, 121);
        setupCommonParam (sliderFMRNG,  labFMRNG,  0x2F,   0, 127);
        setupCommonAssign (comboPNLASN, labPNLASN, 0x30,   0, 121);
        setupCommonParam (sliderPNLRNG, labPNLRNG, 0x31,   0, 127);
        setupCommonAssign (comboCOASN,  labCOASN,  0x32,   0, 121);
        setupCommonParam (sliderCORNG,  labCORNG,  0x33,   0, 127);
        setupCommonAssign (comboPNBASN, labPNBASN, 0x34,   0, 121);
        setupCommonParam (sliderPNBRNG, labPNBRNG, 0x35,   0, 127);
        setupCommonAssign (comboEGBASN, labEGBASN, 0x36,   0, 121);
        setupCommonParam (sliderEGBRNG, labEGBRNG, 0x37,   0, 127);
        setupCommonAssign (comboWLASN,  labWLASN,  0x38,   0, 121);
        setupCommonParam (sliderWLLML,  labWLLML,  0x39,   0, 127);
        setupCommonParam (sliderMCTUN,  labMCTUN,  0x3A,   0,  65);
        setupCommonParam (sliderRNDP,   labRNDP,   0x3B,   0,   7);
        setupAFTMD();
        setupCommonParam (sliderSPTPNT, labSPTPNT, 0x43,   0, 127);
    }
    void layoutCommonPanel()
    {
        // Two columns: label (40px) + control (rest).
        const int rowH = 18;
        const int gap  = 2;
        const int w    = commonPanel.getWidth();
        const int labW = 56;
        int y = 0;
        labelCommon.setBounds (0, y, w, rowH); y += rowH + gap;

        auto place = [&] (Label& l, Component& c)
        {
            l.setBounds (0, y, labW, rowH);
            c.setBounds (labW, y, w - labW, rowH);
            y += rowH + gap;
        };
        place (labWPBR,   sliderWPBR);
        place (labATPBR,  sliderATPBR);
        place (labPMASN,  comboPMASN);
        place (labPMRNG,  sliderPMRNG);
        place (labAMASN,  comboAMASN);
        place (labAMRNG,  sliderAMRNG);
        place (labFMASN,  comboFMASN);
        place (labFMRNG,  sliderFMRNG);
        place (labPNLASN, comboPNLASN);
        place (labPNLRNG, sliderPNLRNG);
        place (labCOASN,  comboCOASN);
        place (labCORNG,  sliderCORNG);
        place (labPNBASN, comboPNBASN);
        place (labPNBRNG, sliderPNBRNG);
        place (labEGBASN, comboEGBASN);
        place (labEGBRNG, sliderEGBRNG);
        place (labWLASN,  comboWLASN);
        place (labWLLML,  sliderWLLML);
        place (labMCTUN,  sliderMCTUN);
        place (labRNDP,   sliderRNDP);
        place (labAFTMD,  comboAFTMD);
        place (labSPTPNT, sliderSPTPNT);
    }

    
    UndoManager undoManager;
};

//==============================================================================

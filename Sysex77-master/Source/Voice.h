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
#include "MixerSection.h"
#include "LiveSynthState.h"
#include "Sy99ParamRegistry.h"
#include "VoicesTableModel.h"
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
        Sy99ParamRegistry::fillEnumComboFromMeta (comboMode, Sy99ParamRegistry::Id::ELMODE);
        comboMode.setSelectedId(1);

        // ELMODE receive: listen to incoming SysEx; match address 02 00 00 00.
        valueSysexIn.addListener(this);
        // [LIBSYNC] Library slot name → editName (no VNAM echo to synth).
        bankSelectedVoiceNameValue.addListener(this);
        bankVoiceSlotApplyTrigger.addListener(this);

        // Voice Common Params panel (Group B: 02 00 00 28..43)
        addAndMakeVisible (commonPanel);
        buildCommonPanel();
        initMixerLevelControls();
        
       
 
    
        addAndMakeVisible (btReadVoiceFromSy99);
        btReadVoiceFromSy99.setTooltip (
            "SY99: Utility → MIDI → Bulk Dump → 07:1 Voice → DUMP OUT, then press Stop sync.");
        btReadVoiceFromSy99.addListener (this);

        addAndMakeVisible (btVoicePrev);
        btVoicePrev.setTooltip ("Previous voice — opens in editor + Bank Select + Program Change on SY99");
        btVoicePrev.addListener (this);
        addAndMakeVisible (btVoiceNext);
        btVoiceNext.setTooltip ("Next voice — opens in editor + Bank Select + Program Change on SY99");
        btVoiceNext.addListener (this);

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

        setupEditorPatchDirtyListeners();
    }
    ~VoicePage()
    {
        stopTimer();
        removeEditorPatchDirtyListeners();
        ensurePitchControlsParentedToCommonPanel();
          valueTreeVoice.removeListener(this);
          removeKeyListener(this);
        element1.removeChangeListener(this);
        element2.removeChangeListener(this);
        element3.removeChangeListener(this);
        element4.removeChangeListener(this);
        editName.removeListener(this);
        comboMode.removeListener(this);
        btReadVoiceFromSy99.removeListener (this);
        btVoicePrev.removeListener (this);
        btVoiceNext.removeListener (this);
        valueSysexIn.removeListener(this);
        bankSelectedVoiceNameValue.removeListener(this);
        bankVoiceSlotApplyTrigger.removeListener(this);
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
        if (value.refersToSameSourceAs (bankVoiceSlotApplyTrigger))
        {
            applyLiveSynthStateToEditor();
            return;
        }

        if (value.refersToSameSourceAs (bankSelectedVoiceNameValue))
        {
            if (getLiveSynthState().hasAnySyncSource())
                applyLiveSynthStateToEditor();
            else
            {
                const String name = value.getValue().toString();
                receivingVNAM = true;
                voiceNameDirtyForMidiOut = false;
                editName.setText (name, juce::dontSendNotification);
                receivingVNAM = false;
            }
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

            // ELMODE receive: address 02 00 00 00 → update comboMode + element layout
            if (b3 == 0x02 && b4 == 0x00 && b5 == 0x00 && b6 == 0x00)
            {
                if (b8 >= 0 && b8 <= 10)
                {
                    comboMode.setSelectedId (b8 + 1, juce::dontSendNotification);
                    valueTreeVoice.setProperty (IDs::VOICEMODE, b8 + 1, nullptr);
                    syncVoiceModeElementLayoutFromRaw (b8);
                }
                return;
            }

            // Master volume (WOL): address 02 00 00 3f
            if (b3 == 0x02 && b4 == 0x00 && b5 == 0x00 && b6 == 0x3f)
            {
                if (b8 >= 0 && b8 <= 127)
                {
                    sliderMaster.setValue ((double) b8, juce::dontSendNotification);
                    valueTreeVoice.setProperty (IDs::COMMONVOLUME, b8, nullptr);
                }
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
    static int eldtUiFromResolved (int raw, LiveSynthState::ParamSource src) noexcept
    {
        if (src == LiveSynthState::ParamSource::Bulk8101)
            return juce::jlimit (-7, 7, raw);

        if (raw < 0)
            return -1;

        if (src == LiveSynthState::ParamSource::Live)
        {
            constexpr int delta = 9;

            if (raw > delta - 2)
            {
                raw -= delta;
                raw = ~raw;
            }
        }

        return juce::jlimit (-7, 7, raw);
    }

    static int elnsUiFromResolved (int raw, LiveSynthState::ParamSource src) noexcept
    {
        if (src == LiveSynthState::ParamSource::Bulk8101)
            return juce::jlimit (-64, 63, raw);

        if (raw < 0)
            return -1;

        if (src == LiveSynthState::ParamSource::Live)
            return YamahaLmVoiceDump::uiFromElementNoteShiftSysex (raw);

        return raw;
    }

    static int atpbrUiFromResolved (int raw, LiveSynthState::ParamSource /*src*/) noexcept
    {
        if (raw < 0)
            return -1;

        return YamahaLmVoiceDump::uiFromVoiceCommonAtpbrSysex (raw);
    }

    static int aftmdUiFromResolved (int raw, LiveSynthState::ParamSource /*src*/) noexcept
    {
        if (raw < 0)
            return -1;

        return YamahaLmVoiceDump::uiFromVoiceCommonAftmdSysex (raw);
    }

    static int efsdvsnsUiFromResolved (int raw, LiveSynthState::ParamSource src) noexcept
    {
        if (raw < 0)
            return -1;

        if (src == LiveSynthState::ParamSource::Live)
            return YamahaLmVoiceDump::uiFromMixerEffectSendSigned7Sysex (raw);

        return YamahaLmVoiceDump::uiFromMixerEffectSendSigned7Sysex (raw);
    }

    void applyResolvedSlider (MidiSlider& slider, const juce::Identifier& id, int uiValue,
                              juce::NotificationType notify = juce::dontSendNotification) noexcept
    {
        const int lo = (int) slider.getMinimum();
        const int hi = (int) slider.getMaximum();

        if (uiValue < lo || uiValue > hi)
            return;

        slider.setValue ((double) uiValue, notify);
        valueTreeVoice.setProperty (id, uiValue, nullptr);
    }

    void applyResolvedCombo (MidiCombo& combo, int uiValue,
                             juce::NotificationType notify = juce::dontSendNotification) noexcept
    {
        if (uiValue < 0)
            return;

        combo.setSelectedId (uiValue + 1, notify);
    }

    MidiSlider& commonSliderForId (Sy99ParamRegistry::Id id) noexcept
    {
        switch (id)
        {
            case Sy99ParamRegistry::Id::WPBR:   return sliderWPBR;
            case Sy99ParamRegistry::Id::PMRNG:  return sliderPMRNG;
            case Sy99ParamRegistry::Id::AMRNG:  return sliderAMRNG;
            case Sy99ParamRegistry::Id::FMRNG:  return sliderFMRNG;
            case Sy99ParamRegistry::Id::PNLRNG: return sliderPNLRNG;
            case Sy99ParamRegistry::Id::CORNG:  return sliderCORNG;
            case Sy99ParamRegistry::Id::PNBRNG: return sliderPNBRNG;
            case Sy99ParamRegistry::Id::EGBRNG: return sliderEGBRNG;
            case Sy99ParamRegistry::Id::WLLML:  return sliderWLLML;
            case Sy99ParamRegistry::Id::MCTUN:  return sliderMCTUN;
            case Sy99ParamRegistry::Id::RNDP:   return sliderRNDP;
            case Sy99ParamRegistry::Id::SPTPNT: return sliderSPTPNT;
            default: break;
        }

        jassertfalse;
        return sliderWPBR;
    }

    MidiCombo& commonComboForId (Sy99ParamRegistry::Id id) noexcept
    {
        switch (id)
        {
            case Sy99ParamRegistry::Id::PMASN:  return comboPMASN;
            case Sy99ParamRegistry::Id::AMASN:  return comboAMASN;
            case Sy99ParamRegistry::Id::FMASN:  return comboFMASN;
            case Sy99ParamRegistry::Id::PNLASN: return comboPNLASN;
            case Sy99ParamRegistry::Id::COASN:  return comboCOASN;
            case Sy99ParamRegistry::Id::PNBASN: return comboPNBASN;
            case Sy99ParamRegistry::Id::EGBASN: return comboEGBASN;
            case Sy99ParamRegistry::Id::WLASN:  return comboWLASN;
            default: break;
        }

        jassertfalse;
        return comboPMASN;
    }

    /** Phase 1: snapshot UI into gLiveSynthState baseline (no MIDI / no .syx). */
    void commitEditorSessionToLiveSynthBaseline() noexcept
    {
        Logger::writeToLog ("[SyncFromSY99] commitEditorSessionToLiveSynthBaseline slot="
                            + String (bankSelectedVoiceIndex));

        auto& lm = getLiveSynthState();

        const int elmode = comboMode.getSelectedItemIndex();

        if (elmode >= 0 && elmode <= 10)
            Sy99ParamRegistry::mirrorBaselineValue (lm, Sy99ParamRegistry::Id::ELMODE, 0, elmode);

        const int wol = (int) sliderMaster.getValue();

        if (wol >= 0 && wol <= 127)
            Sy99ParamRegistry::mirrorBaselineValue (lm, Sy99ParamRegistry::Id::WOL, 0, wol);

        struct ElLevelCommit { MidiSlider* slider; int* lmBulk; int elIdx; };
        const ElLevelCommit elLevels[] = {
            { &sliderMixerEl1Level, &lm.lmElvlE1Raw, 0 },
            { &sliderMixerEl2Level, &lm.lmElvlRaw[1], 1 },
            { &sliderMixerEl3Level, &lm.lmElvlRaw[2], 2 },
            { &sliderMixerEl4Level, &lm.lmElvlRaw[3], 3 },
        };

        for (const auto& el : elLevels)
        {
            const int elvl = (int) el.slider->getValue();

            if (elvl < 0 || elvl > 127)
                continue;

#if JUCE_DEBUG
            if (el.elIdx == 0)
                Sy99ParamRegistry::debugWriteElvlSlot (&lm.lmElvlE1Raw, elvl,
                                                       "commitEditorSessionToLiveSynthBaseline",
                                                       0, "baseline", "bulk8101");
            else
                Sy99ParamRegistry::debugWriteElvlSlot (&lm.lmElvlRaw[(size_t) el.elIdx], elvl,
                                                       "commitEditorSessionToLiveSynthBaseline",
                                                       el.elIdx, "baseline", "bulk8101");
            Sy99ParamRegistry::debugWriteElvlSlot (&lm.elementLevelRaw[(size_t) el.elIdx], elvl,
                                                   "commitEditorSessionToLiveSynthBaseline",
                                                   el.elIdx, "baseline", "live");
#else
            if (el.elIdx == 0)
                lm.lmElvlE1Raw = elvl;
            else
                lm.lmElvlRaw[(size_t) el.elIdx] = elvl;

            lm.elementLevelRaw[(size_t) el.elIdx] = elvl;
#endif
        }

        const int outselE1 = (int) sliderMixerEl1Outsel.getValue();
#if JUCE_DEBUG
        Sy99ParamRegistry::debugLogUiBindingAuditWrite ("sliderMixerEl1Outsel", 0,
                                                        "ValueTree(ELEMENT1OUTSEL)+SysEx(03/00/08)+LiveSynthState",
                                                        Sy99ParamRegistry::Id::OUTSEL, outselE1);
        Sy99ParamRegistry::debugWriteOutselSlot (&lm.lmOutselE1Raw, (int) (outselE1 & 0x06),
                                                 "commitEditorSessionToLiveSynthBaseline", 0,
                                                 "baseline", "bulk8101");
        Sy99ParamRegistry::debugWriteOutselSlot (&lm.elementOutselRaw[0], outselE1,
                                                 "commitEditorSessionToLiveSynthBaseline", 0,
                                                 "baseline", "live");
#else
        lm.lmOutselE1Raw = (int) (outselE1 & 0x06);
        lm.elementOutselRaw[0] = outselE1;
#endif

        const int outselE2 = (int) sliderMixerEl2Outsel.getValue();
#if JUCE_DEBUG
        Sy99ParamRegistry::debugLogUiBindingAuditWrite ("sliderMixerEl2Outsel", 1,
                                                        "ValueTree(ELEMENT2OUTSEL)+SysEx(03/20/08)+LiveSynthState",
                                                        Sy99ParamRegistry::Id::OUTSEL, outselE2);
        Sy99ParamRegistry::debugWriteOutselSlot (&lm.lmOutselRaw[1], (int) (outselE2 & 0x06),
                                                 "commitEditorSessionToLiveSynthBaseline", 1,
                                                 "baseline", "bulk8101");
        Sy99ParamRegistry::debugWriteOutselSlot (&lm.elementOutselRaw[1], outselE2,
                                                 "commitEditorSessionToLiveSynthBaseline", 1,
                                                 "baseline", "live");
#else
        lm.lmOutselRaw[1] = (int) (outselE2 & 0x06);
        lm.elementOutselRaw[1] = outselE2;
#endif

        MidiSlider* mixerDetuneSliders[] = {
            &sliderMixerEl1Detune, &sliderMixerEl2Detune,
            &sliderMixerEl3Detune, &sliderMixerEl4Detune,
        };
        MidiSlider* mixerNoteShiftSliders[] = {
            &sliderMixerEl1NoteShift, &sliderMixerEl2NoteShift,
            &sliderMixerEl3NoteShift, &sliderMixerEl4NoteShift,
        };

        for (int e = 0; e < 4; ++e)
        {
            const int eldt = (int) mixerDetuneSliders[(size_t) e]->getValue();
            const int elns = (int) mixerNoteShiftSliders[(size_t) e]->getValue();

#if JUCE_DEBUG
            Sy99ParamRegistry::debugLogUiBindingAuditWrite ("sliderMixerElDetune", e,
                                                            "ValueTree(ELEMENTnFINE)+SysEx(03/NN/01)+LiveSynthState",
                                                            Sy99ParamRegistry::Id::ELDT, eldt);
            Sy99ParamRegistry::debugLogUiBindingAuditWrite ("sliderMixerElNoteShift", e,
                                                            "ValueTree(ELEMENTnPITCH)+SysEx(03/NN/02)+LiveSynthState",
                                                            Sy99ParamRegistry::Id::ELNS, elns);
            juce::Logger::writeToLog ("[ELDT audit] stage=baseline idx=" + juce::String (e)
                                      + " baselineRaw=" + juce::String (eldt));
            Sy99ParamRegistry::debugWriteEldtSlot (&lm.lmEldtRaw[(size_t) e], eldt,
                                                   "commitEditorSessionToLiveSynthBaseline", e, "baseline");
            Sy99ParamRegistry::debugWriteEldtSlot (&lm.elementEldtRaw[(size_t) e], eldt,
                                                   "commitEditorSessionToLiveSynthBaseline", e, "baseline");
#else
            lm.lmEldtRaw[(size_t) e] = eldt;
            lm.elementEldtRaw[(size_t) e] = eldt;
#endif

#if JUCE_DEBUG
            Sy99ParamRegistry::debugWriteElnsSlot (&lm.lmElnsRaw[(size_t) e], elns,
                                                   "commitEditorSessionToLiveSynthBaseline", e,
                                                   "baseline", "bulk8101");
            Sy99ParamRegistry::debugWriteElnsSlot (&lm.elementElnsRaw[(size_t) e], elns,
                                                   "commitEditorSessionToLiveSynthBaseline", e,
                                                   "baseline", "live");
#else
            lm.lmElnsRaw[(size_t) e] = elns;
            lm.elementElnsRaw[(size_t) e] = elns;
#endif
        }

        MidiSlider* noteLimitLow[] = {
            &sliderMixerEl1NoteLimitLow, &sliderMixerEl2NoteLimitLow,
            &sliderMixerEl3NoteLimitLow, &sliderMixerEl4NoteLimitLow,
        };
        MidiSlider* noteLimitHigh[] = {
            &sliderMixerEl1NoteLimitHigh, &sliderMixerEl2NoteLimitHigh,
            &sliderMixerEl3NoteLimitHigh, &sliderMixerEl4NoteLimitHigh,
        };
        MidiSlider* velLimitLow[] = {
            &sliderMixerEl1VelocityLimitLow, &sliderMixerEl2VelocityLimitLow,
            &sliderMixerEl3VelocityLimitLow, &sliderMixerEl4VelocityLimitLow,
        };
        MidiSlider* velLimitHigh[] = {
            &sliderMixerEl1VelocityLimitHigh, &sliderMixerEl2VelocityLimitHigh,
            &sliderMixerEl3VelocityLimitHigh, &sliderMixerEl4VelocityLimitHigh,
        };

        for (int e = 0; e < 4; ++e)
        {
            lm.elementEnllRaw[(size_t) e] = (int) noteLimitLow[e]->getValue();
            lm.elementEnlhRaw[(size_t) e] = (int) noteLimitHigh[e]->getValue();
            lm.elementEvllRaw[(size_t) e] = (int) velLimitLow[e]->getValue();
            lm.elementEvlhRaw[(size_t) e] = (int) velLimitHigh[e]->getValue();
        }

        lm.lmEvllRaw[0] = lm.elementEvllRaw[0];
        lm.lmEvlhRaw[0] = lm.elementEvlhRaw[0];
        lm.lmEvllRaw[1] = lm.elementEvllRaw[1];
        lm.lmEvlhRaw[1] = lm.elementEvlhRaw[1];

        lm.lm0040EfmodeRaw = (int) sliderEfmode.getValue();
#if JUCE_DEBUG
        Sy99ParamRegistry::debugLogUiBindingAuditWrite ("sliderMixerEl1Efsendsel", 0,
                                                        "ValueTree(ELEMENT1EFSENDSEL)+SysEx(03/00/09)+LiveSynthState",
                                                        Sy99ParamRegistry::Id::EFLN1EL,
                                                        (int) sliderMixerEl1Efsendsel.getValue());
        Sy99ParamRegistry::debugLogUiBindingAuditWrite ("sliderMixerEl1Efsendlvl", 0,
                                                        "ValueTree(ELEMENT1EFSENDLVL)+SysEx(03/00/0A)+LiveSynthState",
                                                        Sy99ParamRegistry::Id::EFSDLV,
                                                        (int) sliderMixerEl1Efsendlvl.getValue());
        Sy99ParamRegistry::debugLogUiBindingAuditWrite ("sliderMixerEl1Efsendvsns", 0,
                                                        "ValueTree(ELEMENT1EFSENDVSNS)+SysEx(03/00/0B)+LiveSynthState",
                                                        Sy99ParamRegistry::Id::EFSDVSNS,
                                                        (int) sliderMixerEl1Efsendvsns.getValue());
        Sy99ParamRegistry::debugLogUiBindingAuditWrite ("sliderMixerEl1Efsendscl", 0,
                                                        "ValueTree(ELEMENT1EFSENDSCL)+SysEx(03/00/0C)+LiveSynthState",
                                                        Sy99ParamRegistry::Id::EFSDSCL,
                                                        (int) sliderMixerEl1Efsendscl.getValue());
        Sy99ParamRegistry::debugWriteEfsendSlot (&lm.lmEfln1ElRaw, Sy99ParamRegistry::Id::EFLN1EL,
                                                 (int) sliderMixerEl1Efsendsel.getValue(),
                                                 "commitEditorSessionToLiveSynthBaseline", 0, "baseline");
        Sy99ParamRegistry::debugWriteEfsendSlot (&lm.lmEfsdlvRaw, Sy99ParamRegistry::Id::EFSDLV,
                                                 (int) sliderMixerEl1Efsendlvl.getValue(),
                                                 "commitEditorSessionToLiveSynthBaseline", 0, "baseline");
        Sy99ParamRegistry::debugWriteEfsendSlot (&lm.lmEfsdvlRaw, Sy99ParamRegistry::Id::EFSDVSNS,
                                                 (int) sliderMixerEl1Efsendvsns.getValue(),
                                                 "commitEditorSessionToLiveSynthBaseline", 0, "baseline");
        Sy99ParamRegistry::debugWriteEfsendSlot (&lm.lmEfsdsclRaw, Sy99ParamRegistry::Id::EFSDSCL,
                                                 (int) sliderMixerEl1Efsendscl.getValue(),
                                                 "commitEditorSessionToLiveSynthBaseline", 0, "baseline");
        Sy99ParamRegistry::debugWriteEfsendSlot (&lm.elementEfsendselRaw[0],
                                                 Sy99ParamRegistry::Id::EFLN1EL, lm.lmEfln1ElRaw,
                                                 "commitEditorSessionToLiveSynthBaseline", 0, "baseline");
        Sy99ParamRegistry::debugWriteEfsendSlot (&lm.elementEfsendlvlRaw[0],
                                                 Sy99ParamRegistry::Id::EFSDLV, lm.lmEfsdlvRaw,
                                                 "commitEditorSessionToLiveSynthBaseline", 0, "baseline");
        Sy99ParamRegistry::debugWriteEfsendSlot (&lm.elementEfsdvsnsRaw[0],
                                                 Sy99ParamRegistry::Id::EFSDVSNS, lm.lmEfsdvlRaw,
                                                 "commitEditorSessionToLiveSynthBaseline", 0, "baseline");
        Sy99ParamRegistry::debugWriteEfsendSlot (&lm.elementEfsdsclRaw[0],
                                                 Sy99ParamRegistry::Id::EFSDSCL, lm.lmEfsdsclRaw,
                                                 "commitEditorSessionToLiveSynthBaseline", 0, "baseline");
#else
        lm.lmEfln1ElRaw = (int) sliderMixerEl1Efsendsel.getValue();
        lm.lmEfsdlvRaw = (int) sliderMixerEl1Efsendlvl.getValue();
        lm.lmEfsdvlRaw = (int) sliderMixerEl1Efsendvsns.getValue();
        lm.lmEfsdsclRaw = (int) sliderMixerEl1Efsendscl.getValue();
        lm.elementEfsendselRaw[0] = lm.lmEfln1ElRaw;
        lm.elementEfsendlvlRaw[0] = lm.lmEfsdlvRaw;
        lm.elementEfsdvsnsRaw[0] = lm.lmEfsdvlRaw;
        lm.elementEfsdsclRaw[0] = lm.lmEfsdsclRaw;
#endif
        lm.efmodeRaw = lm.lm0040EfmodeRaw;

#if JUCE_DEBUG
        Sy99ParamRegistry::debugLogEfsendAuditBaseline (Sy99ParamRegistry::Id::EFLN1EL, 0, lm.lmEfln1ElRaw);
        Sy99ParamRegistry::debugLogEfsendAuditBaseline (Sy99ParamRegistry::Id::EFSDLV, 0, lm.lmEfsdlvRaw);
        Sy99ParamRegistry::debugLogEfsendAuditBaseline (Sy99ParamRegistry::Id::EFSDVSNS, 0, lm.lmEfsdvlRaw);
        Sy99ParamRegistry::debugLogEfsendAuditBaseline (Sy99ParamRegistry::Id::EFSDSCL, 0, lm.lmEfsdsclRaw);
#endif

        for (auto id : Sy99ParamRegistry::kCommonDirectSliderIds)
        {
            const int v = (int) commonSliderForId (id).getValue();
            Sy99ParamRegistry::mirrorBaselineValue (lm, id, 0, v);
        }

        for (auto id : Sy99ParamRegistry::kCommonDirectComboIds)
        {
            const int v = commonComboForId (id).getSelectedItemIndex();
            Sy99ParamRegistry::mirrorBaselineValue (lm, id, 0, v);
        }

        Sy99ParamRegistry::mirrorBaselineValue (lm, Sy99ParamRegistry::Id::ATPBR, 0,
                                                (int) sliderATPBR.getValue());
        Sy99ParamRegistry::mirrorBaselineValue (lm, Sy99ParamRegistry::Id::AFTMD, 0,
                                                comboAFTMD.getSelectedItemIndex());

        const String str = editName.getText();
        const char* data = str.toRawUTF8();
        const int len = str.length();

        for (int i = 0; i < 10; ++i)
        {
            const char c = (char) (i < len ? (data[i] & 0x7f) : 0x20);
#if JUCE_DEBUG
            Sy99ParamRegistry::debugWriteAuditCharWithStage (&lm.lmVoiceName[i], i, c,
                                                             "commitEditorSessionToLiveSynthBaseline",
                                                             "baseline");
            Sy99ParamRegistry::debugWriteAuditCharWithStage (&voiceNameBuffer[i], i, c,
                                                             "commitEditorSessionToLiveSynthBaseline",
                                                             "baseline");
#else
            lm.lmVoiceName[i] = c;
            voiceNameBuffer[i] = c;
#endif
        }

        lm.lmVoiceName[10] = '\0';

#if JUCE_DEBUG
        {
            const int baselineByIdx[4] = {
                (int) (outselE1 & 0x06),
                (int) (outselE2 & 0x06),
                -1,
                -1,
            };
            Sy99ParamRegistry::debugLogOutselAuditAll (lm, "baseline-commit", nullptr, baselineByIdx);
        }
#endif

        logCommittedBaselineSnapshot();

#if JUCE_DEBUG
        Sy99ParamRegistry::debugDumpAuditedSyncState (lm, bankSelectedVoiceIndex,
                                                      editName.getText().toRawUTF8());
        Sy99ParamRegistry::debugLogTrackedOverwriteSnapshot (lm, "AfterBaseline");
#endif
    }

    void logCommittedBaselineSnapshot() const noexcept
    {
        const auto& lm = getLiveSynthState();
        juce::String s = "[SyncFromSY99] baseline commit: VNAM=\""
                         + String (lm.lmVoiceName).trimEnd() + "\"";

        if (lm.lmElmodeRaw >= 0)
            s << " ELMODE=" << lm.lmElmodeRaw;

        if (lm.lmWolRaw >= 0)
            s << " WOL=" << lm.lmWolRaw;

        if (lm.lmElvlE1Raw >= 0)
            s << " ELVL_E1=" << lm.lmElvlE1Raw;

        for (int e = 1; e < 4; ++e)
            if (lm.lmElvlRaw[e] >= 0)
                s << " ELVL_E" << (e + 1) << "=" << lm.lmElvlRaw[e];

        if (lm.lmOutselE1Raw >= 0)
            s << " OUTSEL_E1=0x" + juce::String::toHexString (lm.lmOutselE1Raw).toUpperCase();

        if (lm.elementOutselRaw[1] >= 0)
            s << " OUTSEL_E2=0x" + juce::String::toHexString (lm.elementOutselRaw[1]).toUpperCase();

        Logger::writeToLog (s);
    }

    /** Stage 3: LM 8101VC + 0040VC + live param9 → bound controls only; no outbound SysEx echo. */
    bool applyLiveSynthStateToEditor() noexcept
    {
        const auto& lm = getLiveSynthState();

        if (! lm.hasAnySyncSource())
            return false;

        LiveSynthState::ParamSource src = LiveSynthState::ParamSource::None;
        int elmodeResolved = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::ELMODE, 0, src);
        int wolResolved = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::WOL, 0, src);
        Sy99ParamRegistry::debugLogResolvedParam (lm, Sy99ParamRegistry::Id::WOL, 0, "WOL");

        Logger::writeToLog ("[SyncFromSY99] apply diag: has8101=" + String ((int) lm.hasParsedBulk8101)
                            + " has0040=" + String ((int) lm.hasParsedBulk0040)
                            + " param9=" + String (lm.parameterFrameCount)
                            + " wol=" + String (wolResolved)
                            + " elmode=" + String (elmodeResolved)
                            + " dirty=" + String ((int) editorPatchDirty()));

        Logger::writeToLog ("[SyncFromSY99] applyLiveSynthStateToEditor slot="
                            + String (bankSelectedVoiceIndex));

        EditorPatchDirtySuppressGuard suppressDirtyMark;

        if (lm.lmVoiceName[0] != '\0' || lm.voiceNameCharCount > 0)
        {
            String name = lm.voiceNameCharCount > 0
                            ? String (lm.voiceName, lm.voiceNameCharCount).trimEnd()
                            : String (lm.lmVoiceName).trimEnd();

            if (name.isEmpty() && lm.lmVoiceName[0] != '\0')
                name = String (lm.lmVoiceName).trimEnd();

            receivingVNAM = true;
            voiceNameDirtyForMidiOut = false;
            editName.setText (name, juce::dontSendNotification);
            receivingVNAM = false;
        }

        refreshEnumCombosFromMetaRegistry();

        if (elmodeResolved >= 0 && elmodeResolved <= 10)
        {
            comboMode.setSelectedId (elmodeResolved + 1, juce::dontSendNotification);
            valueTreeVoice.setProperty (IDs::VOICEMODE, elmodeResolved + 1, nullptr);
            syncVoiceModeElementLayoutFromRaw (elmodeResolved);
        }

        if (wolResolved >= 0 && wolResolved <= 127)
        {
            sliderMaster.setValue ((double) wolResolved, juce::dontSendNotification);
            valueTreeVoice.setProperty (IDs::COMMONVOLUME, wolResolved, nullptr);
        }

        juce::String commonLog;

        for (auto id : Sy99ParamRegistry::kCommonDirectSliderIds)
        {
            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            const int resolved = Sy99ParamRegistry::resolveParam (lm, id, 0, rowSrc);
            if (id == Sy99ParamRegistry::Id::WPBR)
                Sy99ParamRegistry::debugLogResolvedParam (lm, id, 0, "WPBR");

            if (resolved < 0)
                continue;

            commonSliderForId (id).setValue ((double) resolved, juce::dontSendNotification);

            if (const auto* m = Sy99ParamRegistry::metaFor (id))
                commonLog << " " << m->tag << "=" << resolved
                          << "(" << LiveSynthState::paramSourceTag (rowSrc) << ")";
        }

        {
            LiveSynthState::ParamSource atpbrSrc = LiveSynthState::ParamSource::None;
            const int atpbrResolved = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::ATPBR, 0, atpbrSrc);

            if (atpbrResolved >= 0)
            {
                const int atpbrUi = atpbrUiFromResolved (atpbrResolved, atpbrSrc);
                sliderATPBR.setValue ((double) atpbrUi, juce::dontSendNotification);
                commonLog << " ATPBR=" << atpbrUi << "(" << LiveSynthState::paramSourceTag (atpbrSrc) << ")";
            }
        }

        for (auto id : Sy99ParamRegistry::kCommonDirectComboIds)
        {
            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            const int resolved = Sy99ParamRegistry::resolveParam (lm, id, 0, rowSrc);

            if (resolved < 0)
                continue;

            applyResolvedCombo (commonComboForId (id), resolved);

            if (const auto* m = Sy99ParamRegistry::metaFor (id))
                commonLog << " " << m->tag << "=" << resolved
                          << "(" << LiveSynthState::paramSourceTag (rowSrc) << ")";
        }

        {
            LiveSynthState::ParamSource aftmdSrc = LiveSynthState::ParamSource::None;
            const int aftmdResolved = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::AFTMD, 0, aftmdSrc);

            if (aftmdResolved >= 0)
            {
                const int aftmdUi = aftmdUiFromResolved (aftmdResolved, aftmdSrc);
                applyResolvedCombo (comboAFTMD, aftmdUi);
                commonLog << " AFTMD=" << aftmdUi << "(" << LiveSynthState::paramSourceTag (aftmdSrc) << ")";
            }
        }

        juce::String elvlLog;

        struct ElLevelUi { MidiSlider* slider; juce::Identifier id; int elementIndex; };
        const ElLevelUi elLevels[] = {
            { &sliderMixerEl1Level, IDs::ELEMENT1VOLUME, 0 },
            { &sliderMixerEl2Level, IDs::ELEMENT2VOLUME, 1 },
            { &sliderMixerEl3Level, IDs::ELEMENT3VOLUME, 2 },
            { &sliderMixerEl4Level, IDs::ELEMENT4VOLUME, 3 },
        };

        for (const auto& el : elLevels)
        {
            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            const int elvlRaw = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::ELVL,
                                                                 el.elementIndex, rowSrc);

            if (el.elementIndex <= 1)
            {
                const char* elvlLabel = el.elementIndex == 0 ? "ELVL_E1" : "ELVL_E2";
                Sy99ParamRegistry::debugLogResolvedParam (lm, Sy99ParamRegistry::Id::ELVL,
                                                          el.elementIndex, elvlLabel);
            }

            if (elvlRaw < 0 || elvlRaw > 127 || el.slider == nullptr)
                continue;

            el.slider->setValue ((double) elvlRaw, juce::dontSendNotification);
            valueTreeVoice.setProperty (el.id, elvlRaw, nullptr);
            elvlLog << " E" << (el.elementIndex + 1) << "=" << elvlRaw;
        }

        juce::String outselLog;

#if JUCE_DEBUG
        int outselUiByIdx[4] = { -1, -1, -1, -1 };
#endif

        {
            LiveSynthState::ParamSource outselSrc = LiveSynthState::ParamSource::None;
            int outselE1 = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::OUTSEL, 0, outselSrc);
            Sy99ParamRegistry::debugLogResolvedParam (lm, Sy99ParamRegistry::Id::OUTSEL, 0, "OUTSEL1");

            if (outselE1 >= 0)
            {
                const int outselUi = (int) ((uint8) outselE1 & 0x06);
#if JUCE_DEBUG
                Sy99ParamRegistry::debugLogUiBindingAudit ("sliderMixerEl1Outsel", 0, "resolveParam",
                                                             Sy99ParamRegistry::Id::OUTSEL,
                                                             outselE1, outselUi);
                Sy99ParamRegistry::debugLogUiBindingAudit ("mixerEl1OutputGroup", 0, "resolveParam",
                                                            Sy99ParamRegistry::Id::OUTSEL,
                                                            outselE1, outselUi);
#endif
                sliderMixerEl1Outsel.setValue ((double) outselUi, juce::dontSendNotification);
                valueTreeVoice.setProperty (IDs::ELEMENT1OUTSEL, outselUi, nullptr);
                outselLog = " OUTSEL_E1=0x" + String::toHexString (outselUi).toUpperCase();
#if JUCE_DEBUG
                outselUiByIdx[0] = outselUi;
#endif
            }
        }

        {
            LiveSynthState::ParamSource outselSrc = LiveSynthState::ParamSource::None;
            const int outselE2 = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::OUTSEL, 1, outselSrc);

            if (outselE2 >= 0)
            {
#if JUCE_DEBUG
                Sy99ParamRegistry::debugLogUiBindingAudit ("sliderMixerEl2Outsel", 1, "resolveParam",
                                                           Sy99ParamRegistry::Id::OUTSEL,
                                                           outselE2, outselE2);
                Sy99ParamRegistry::debugLogUiBindingAudit ("mixerEl2OutputGroup", 1, "resolveParam",
                                                           Sy99ParamRegistry::Id::OUTSEL,
                                                           outselE2, outselE2);
#endif
                sliderMixerEl2Outsel.setValue ((double) outselE2, juce::dontSendNotification);
#if JUCE_DEBUG
                outselUiByIdx[1] = (int) (outselE2 & 0x06);
#endif
            }
        }

#if JUCE_DEBUG
        for (int e = 2; e < 4; ++e)
        {
            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            const int resolved = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::OUTSEL, e, rowSrc);
            if (resolved >= 0)
                Sy99ParamRegistry::debugLogUiBindingAudit ("(no mixer UI control)", e, "other",
                                                           Sy99ParamRegistry::Id::OUTSEL,
                                                           resolved, -1);
        }
#endif

#if JUCE_DEBUG
        for (int e = 0; e < 4; ++e)
        {
            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            const int resolved = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::OUTSEL, e, rowSrc);
            Sy99ParamRegistry::debugLogOutselAudit (lm, "ui-apply", e, outselUiByIdx[e], -1);
            juce::ignoreUnused (resolved);
        }
#endif

        Element* elements[] = { &element1, &element2, &element3, &element4 };
        const juce::Identifier fineIds[] = {
            IDs::ELEMENT1FINE, IDs::ELEMENT2FINE, IDs::ELEMENT3FINE, IDs::ELEMENT4FINE,
        };
        const juce::Identifier pitchIds[] = {
            IDs::ELEMENT1PITCH, IDs::ELEMENT2PITCH, IDs::ELEMENT3PITCH, IDs::ELEMENT4PITCH,
        };
        MidiSlider* mixerDetuneSliders[] = {
            &sliderMixerEl1Detune, &sliderMixerEl2Detune,
            &sliderMixerEl3Detune, &sliderMixerEl4Detune,
        };
        MidiSlider* mixerNoteShiftSliders[] = {
            &sliderMixerEl1NoteShift, &sliderMixerEl2NoteShift,
            &sliderMixerEl3NoteShift, &sliderMixerEl4NoteShift,
        };

        juce::String pitchLog;

        for (int e = 0; e < 4; ++e)
        {
            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            const int eldtResolved = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::ELDT, e, rowSrc);

            if (rowSrc != LiveSynthState::ParamSource::None)
            {
                const int eldtUi = eldtUiFromResolved (eldtResolved, rowSrc);
#if JUCE_DEBUG
                Sy99ParamRegistry::debugLogUiBindingAudit ("sliderMixerElDetune", e, "resolveParam",
                                                           Sy99ParamRegistry::Id::ELDT,
                                                           eldtResolved, eldtUi);
#endif
                applyResolvedSlider (*mixerDetuneSliders[(size_t) e], fineIds[(size_t) e], eldtUi);
                pitchLog << " ELDT_E" << (e + 1) << "=" << eldtUi;
            }

            rowSrc = LiveSynthState::ParamSource::None;
            const int elnsResolved = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::ELNS, e, rowSrc);
            if (e == 0)
                Sy99ParamRegistry::debugLogResolvedParam (lm, Sy99ParamRegistry::Id::ELNS, 0, "ELNS1");

            if (rowSrc != LiveSynthState::ParamSource::None)
            {
                const int elnsUi = elnsUiFromResolved (elnsResolved, rowSrc);
#if JUCE_DEBUG
                Sy99ParamRegistry::debugLogUiBindingAudit ("sliderMixerElNoteShift", e, "resolveParam",
                                                           Sy99ParamRegistry::Id::ELNS,
                                                           elnsResolved, elnsUi);
#endif
                applyResolvedSlider (*mixerNoteShiftSliders[(size_t) e], pitchIds[(size_t) e], elnsUi);
                pitchLog << " ELNS_E" << (e + 1) << "=" << elnsUi;
            }
        }

#if JUCE_DEBUG
        for (int e = 0; e < 4; ++e)
        {
            juce::Logger::writeToLog ("[MIXER ROW audit] stage=postApply idx=" + juce::String (e)
                                      + " mixerDetune=" + juce::String ((int) mixerDetuneSliders[(size_t) e]->getValue())
                                      + " mixerNoteShift=" + juce::String ((int) mixerNoteShiftSliders[(size_t) e]->getValue())
                                      + " pitchDetune=" + juce::String ((int) elements[e]->getDetuneSlider().getValue())
                                      + " pitchNoteShift=" + juce::String ((int) elements[e]->getNoteShiftSlider().getValue())
                                      + " vtFine=" + juce::String ((int) valueTreeVoice.getProperty (fineIds[(size_t) e]))
                                      + " vtPitch=" + juce::String ((int) valueTreeVoice.getProperty (pitchIds[(size_t) e])));
        }
#endif

        MidiSlider* noteLimitLow[] = {
            &sliderMixerEl1NoteLimitLow, &sliderMixerEl2NoteLimitLow,
            &sliderMixerEl3NoteLimitLow, &sliderMixerEl4NoteLimitLow,
        };
        MidiSlider* noteLimitHigh[] = {
            &sliderMixerEl1NoteLimitHigh, &sliderMixerEl2NoteLimitHigh,
            &sliderMixerEl3NoteLimitHigh, &sliderMixerEl4NoteLimitHigh,
        };
        MidiSlider* velLimitLow[] = {
            &sliderMixerEl1VelocityLimitLow, &sliderMixerEl2VelocityLimitLow,
            &sliderMixerEl3VelocityLimitLow, &sliderMixerEl4VelocityLimitLow,
        };
        MidiSlider* velLimitHigh[] = {
            &sliderMixerEl1VelocityLimitHigh, &sliderMixerEl2VelocityLimitHigh,
            &sliderMixerEl3VelocityLimitHigh, &sliderMixerEl4VelocityLimitHigh,
        };
        const juce::Identifier enllIds[] = {
            IDs::ELEMENT1NOTELIMITLOW, IDs::ELEMENT2NOTELIMITLOW,
            IDs::ELEMENT3NOTELIMITLOW, IDs::ELEMENT4NOTELIMITLOW,
        };
        const juce::Identifier enlhIds[] = {
            IDs::ELEMENT1NOTELIMITHIGH, IDs::ELEMENT2NOTELIMITHIGH,
            IDs::ELEMENT3NOTELIMITHIGH, IDs::ELEMENT4NOTELIMITHIGH,
        };
        const juce::Identifier evllIds[] = {
            IDs::ELEMENT1VELOCITYLIMITLOW, IDs::ELEMENT2VELOCITYLIMITLOW,
            IDs::ELEMENT3VELOCITYLIMITLOW, IDs::ELEMENT4VELOCITYLIMITLOW,
        };
        const juce::Identifier evlhIds[] = {
            IDs::ELEMENT1VELOCITYLIMITHIGH, IDs::ELEMENT2VELOCITYLIMITHIGH,
            IDs::ELEMENT3VELOCITYLIMITHIGH, IDs::ELEMENT4VELOCITYLIMITHIGH,
        };

        juce::String limitsLog;

        for (int e = 0; e < 4; ++e)
        {
            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            const int enll = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::ENLL, e, rowSrc);

            if (enll >= 0)
                applyResolvedSlider (*noteLimitLow[e], enllIds[(size_t) e], enll);

            rowSrc = LiveSynthState::ParamSource::None;
            const int enlh = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::ENLH, e, rowSrc);

            if (enlh >= 0)
                applyResolvedSlider (*noteLimitHigh[e], enlhIds[(size_t) e], enlh);

            rowSrc = LiveSynthState::ParamSource::None;
            const int evll = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::EVLL, e, rowSrc);

            if (evll >= 0)
            {
                applyResolvedSlider (*velLimitLow[e], evllIds[(size_t) e], evll);
                limitsLog << " EVLL_E" << (e + 1) << "=" << evll;
            }

            rowSrc = LiveSynthState::ParamSource::None;
            const int evlh = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::EVLH, e, rowSrc);

            if (evlh >= 0)
            {
                applyResolvedSlider (*velLimitHigh[e], evlhIds[(size_t) e], evlh);
                limitsLog << " EVLH_E" << (e + 1) << "=" << evlh;
            }
        }

        {
            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            const int efmode = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::EFMODE, 0, rowSrc);
            Sy99ParamRegistry::debugLogResolvedParam (lm, Sy99ParamRegistry::Id::EFMODE, 0, "EFMODE");

            if (efmode >= 0)
                applyResolvedSlider (sliderEfmode, IDs::EFMODE, efmode);

            struct EfsendApplyRow
            {
                int elementIndex;
                MidiSlider* sel;
                MidiSlider* lvl;
                MidiSlider* vsns;
                MidiSlider* scl;
                juce::Identifier selId;
                juce::Identifier lvlId;
                juce::Identifier vsnsId;
                juce::Identifier sclId;
                const char* selSliderName;
                const char* lvlSliderName;
                const char* efsdlvLogLabel;
            };

            const EfsendApplyRow efsendRows[] = {
                { 0, &sliderMixerEl1Efsendsel, &sliderMixerEl1Efsendlvl,
                  &sliderMixerEl1Efsendvsns, &sliderMixerEl1Efsendscl,
                  IDs::ELEMENT1EFSENDSEL, IDs::ELEMENT1EFSENDLVL,
                  IDs::ELEMENT1EFSENDVSNS, IDs::ELEMENT1EFSENDSCL,
                  "sliderMixerEl1Efsendsel", "sliderMixerEl1Efsendlvl", "EFSDLV_E1" },
                { 1, &sliderMixerEl2Efsendsel, &sliderMixerEl2Efsendlvl,
                  &sliderMixerEl2Efsendvsns, &sliderMixerEl2Efsendscl,
                  IDs::ELEMENT2EFSENDSEL, IDs::ELEMENT2EFSENDLVL,
                  IDs::ELEMENT2EFSENDVSNS, IDs::ELEMENT2EFSENDSCL,
                  "sliderMixerEl2Efsendsel", "sliderMixerEl2Efsendlvl", "EFSDLV_E2" },
            };

            for (const auto& er : efsendRows)
            {
                rowSrc = LiveSynthState::ParamSource::None;
                const int efln = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::EFLN1EL,
                                                                  er.elementIndex, rowSrc);

                if (efln >= 0)
                {
#if JUCE_DEBUG
                    Sy99ParamRegistry::debugLogUiBindingAudit (er.selSliderName, er.elementIndex,
                                                               "resolveParam",
                                                               Sy99ParamRegistry::Id::EFLN1EL,
                                                               efln, efln);
#endif
                    applyResolvedSlider (*er.sel, er.selId, efln);
                }

                rowSrc = LiveSynthState::ParamSource::None;
                const int efsdlv = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::EFSDLV,
                                                                    er.elementIndex, rowSrc);
                Sy99ParamRegistry::debugLogResolvedParam (lm, Sy99ParamRegistry::Id::EFSDLV,
                                                          er.elementIndex, er.efsdlvLogLabel);

                if (efsdlv >= 0)
                {
#if JUCE_DEBUG
                    Sy99ParamRegistry::debugLogUiBindingAudit (er.lvlSliderName, er.elementIndex,
                                                               "resolveParam",
                                                               Sy99ParamRegistry::Id::EFSDLV,
                                                               efsdlv, efsdlv);
#endif
                    applyResolvedSlider (*er.lvl, er.lvlId, efsdlv);
                }

                rowSrc = LiveSynthState::ParamSource::None;
                const int efsdvl = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::EFSDVSNS,
                                                                    er.elementIndex, rowSrc);

                if (efsdvl >= 0)
                {
                    const int efsdUi = efsdvsnsUiFromResolved (efsdvl, rowSrc);
                    applyResolvedSlider (*er.vsns, er.vsnsId, efsdUi);
                }

                rowSrc = LiveSynthState::ParamSource::None;
                const int efsdscl = Sy99ParamRegistry::resolveParam (lm, Sy99ParamRegistry::Id::EFSDSCL,
                                                                     er.elementIndex, rowSrc);

                if (efsdscl >= 0)
                {
                    const int sclUi = efsdvsnsUiFromResolved (efsdscl, rowSrc);
                    applyResolvedSlider (*er.scl, er.sclId, sclUi);
                }
            }
        }

        Logger::writeToLog ("[SyncFromSY99] applyLiveSynthStateToEditor done: name=\""
                            + String (lm.lmVoiceName).trimEnd()
                            + "\" ELMODE=" + String (elmodeResolved)
                            + " WOL=" + String (wolResolved)
                            + commonLog
                            + elvlLog
                            + outselLog
                            + pitchLog
                            + limitsLog
                            + " (editor only — move controls or SEND VOICE for SY99)");

#if JUCE_DEBUG
        Sy99ParamRegistry::debugLogAllCatalogParamsForSync (lm);
        Sy99ParamRegistry::debugDumpAuditedSyncState (lm, bankSelectedVoiceIndex,
                                                      editName.getText().toRawUTF8());
        Sy99ParamRegistry::debugLogTrackedOverwriteSnapshot (lm, "AfterApply");
#endif

        if (auto& refresh = refreshMixerBoundControlsCallback(); refresh != nullptr)
            refresh();

        if (auto& relayout = syncCommonMixerLayoutCallback(); relayout != nullptr)
            relayout();

        return true;
    }

    /** Combo labels from params_meta.json enumNames (CMS). */
    void refreshEnumCombosFromMetaRegistry() noexcept
    {
        Sy99ParamRegistry::fillEnumComboFromMeta (comboMode, Sy99ParamRegistry::Id::ELMODE);
    }

    /** Element tabs / op layout for ELMODE raw 0…10 (matches combo item index). No MIDI out. */
    void syncVoiceModeElementLayoutFromRaw (int raw) noexcept
    {
        switch (raw)
        {
            case voiceMode::AFMMono1:
                setNombreElements (Element::mode::AFMmono);
                element1.setOpMode (1);
                break;
            case voiceMode::AFMMono2:
                setNombreElements (2);
                element1.setOpMode (Element::mode::AFMmono);
                element2.setOpMode (Element::mode::AFMmono);
                break;
            case voiceMode::AFMMono4:
                setNombreElements (4);
                element1.setOpMode (Element::mode::AFMmono);
                element2.setOpMode (Element::mode::AFMmono);
                element3.setOpMode (Element::mode::AFMmono);
                element4.setOpMode (Element::mode::AFMmono);
                break;
            case voiceMode::AFMPoly1:
                setNombreElements (1);
                element1.setOpMode (Element::mode::AFMPoly);
                break;
            case voiceMode::AFMPoly2:
                setNombreElements (2);
                element1.setOpMode (Element::mode::AFMPoly);
                element2.setOpMode (Element::mode::AFMPoly);
                break;
            case voiceMode::AWMPoly1:
                setNombreElements (1);
                element1.setOpMode (Element::mode::AWM);
                break;
            case voiceMode::AWMPoly2:
                setNombreElements (2);
                element1.setOpMode (Element::mode::AWM);
                element2.setOpMode (Element::mode::AWM);
                break;
            case voiceMode::AWMPoly4:
                setNombreElements (4);
                element1.setOpMode (Element::mode::AWM);
                element2.setOpMode (Element::mode::AWM);
                element3.setOpMode (Element::mode::AWM);
                element4.setOpMode (Element::mode::AWM);
                break;
            case voiceMode::AFM1AWM1:
                setNombreElements (2);
                element1.setOpMode (Element::mode::AFMPoly);
                element2.setOpMode (Element::mode::AWM);
                break;
            case voiceMode::AFM2AWM2:
                setNombreElements (4);
                element1.setOpMode (Element::mode::AFMPoly);
                element2.setOpMode (Element::mode::AFMPoly);
                element3.setOpMode (Element::mode::AWM);
                element4.setOpMode (Element::mode::AWM);
                break;
            default:
                break;
        }

        sendChangeMessage();
    }

    void buttonClicked (Button* button) override
    {
        if (button == &btVoicePrev)
        {
            stepLibraryVoiceSlot (-1);
            return;
        }

        if (button == &btVoiceNext)
        {
            stepLibraryVoiceSlot (1);
            return;
        }

        if (button == &btReadVoiceFromSy99)
        {
            const bool stopping = isLiveVoiceReadActive();

            if (! stopping)
                Logger::writeToLog ("[SyncFromSY99] Start sync slot="
                                    + String (bankSelectedVoiceIndex));

            toggleLiveVoiceReadFromSy99();

            if (stopping)
            {
#if JUCE_DEBUG
                Sy99ParamRegistry::debugLogTrackedOverwriteSnapshot (getLiveSynthState(), "StopSync");
#endif
                EditorPatchDirtySuppressGuard suppressDirtyMark;

                if (applyLiveSynthStateToEditor())
                {
                    commitEditorSessionToLiveSynthBaseline();
                    editorSyncBaselineGracePeriod() = true;
                    clearEditorPatchDirty();
                    Logger::writeToLog ("[SyncFromSY99] clearEditorPatchDirty slot="
                                        + String (bankSelectedVoiceIndex));

                    juce::MessageManager::callAsync ([]
                    {
                        suppressEditorPatchDirtyMark() = true;
                        clearEditorPatchDirty();
                        suppressEditorPatchDirtyMark() = false;
                        editorSyncBaselineGracePeriod() = false;
                    });
                }
            }

            btReadVoiceFromSy99.setButtonText (stopping ? TRANS ("Sync from SY99")
                                                         : TRANS ("Stop sync"));
            return;
        }

        Logger::writeToLog ("Voice: ButtonClicked");
    }

    void textEditorTextChanged (TextEditor& ed) override
    {
        Logger::writeToLog ("TXT");
        if (&ed == &editName && ! receivingVNAM && editName.hasKeyboardFocus (true))
        {
            voiceNameDirtyForMidiOut = true;
            markEditorPatchDirty();
            flushVoiceNameToSynth();
        }
    }

    void textEditorReturnKeyPressed (TextEditor& editText) override
    {
        if (&editText != &editName)
            return;
        Logger::writeToLog ("Voice TextEditor return — push VNAM");
        flushVoiceNameToSynth();
    }

    void textEditorFocusLost (TextEditor& ed) override
    {
        if (&ed != &editName)
            return;
        Logger::writeToLog ("Voice TextEditor lostFocus — push VNAM if dirty");
        flushVoiceNameToSynth();
    }

    /** VNAM outbound: ten `02 00 00 01..0A` SysEx via OSC `/77MidiMessage`. Sends only when `voiceNameDirtyForMidiOut`. */
    void flushVoiceNameToSynth()
    {
        Logger::writeToLog ("OUT1 flush entered");
        if (receivingVNAM || ! voiceNameDirtyForMidiOut)
            return;
        String str = editName.getText();
        uint8 sysexdata[9] = { 0x43, sysexEngine, 0x34, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00 };
        const int len = str.length();
        const char* data = str.toRawUTF8();
        oscMidiMessage.clear();
        for (auto i = 0; i < 10; ++i)
        {
            sysexdata[6] = (uint8) (0x01 + i);
            sysexdata[8] = (uint8) (i < len ? data[i] & 0x7F : 0x20);
            oscMidiMessage.set (i, MidiMessage::createSysExMessage (sysexdata, 9));
        }
        if (! sender.send (oscSendMidiMessage, (int) 10))
            Logger::writeToLog ("OSC erreur voice VNAM");
        else
        {
            Logger::writeToLog ("OUT2 osc sent");
            Logger::writeToLog ("Voice VNAM: queued 10 SysEx chars");
            voiceNameDirtyForMidiOut = false;
        }
    }
    void sliderValueChanged (Slider* slider) override
    {
        if (isEditorPatchDirtySlider (slider))
            markEditorPatchDirty();
    }

    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override
    {
        Logger::writeToLog ("Voice comboBox Changed");

        if (isEditorPatchDirtyCombo (comboBoxThatHasChanged))
            markEditorPatchDirty();

        if (comboBoxThatHasChanged == &comboMode)
        {
            if (! sender.send (adresseOpMode, (int) comboMode.getSelectedItemIndex())) // [5]
                Logger::writeToLog ("OSC erreur voice opMode");;

            syncVoiceModeElementLayoutFromRaw (comboMode.getSelectedItemIndex());
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
        
        Logger::writeToLog("setNombre elements count=" + String (nombre));
        resized();
        sendChangeMessage();
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
        auto hGrid = (getHeight() - 58)/nombreElements;

        const int topY = 4;
        const int topH = 24;
        const int labelY = topY + topH + 2;
        const int masterW = 96;
        const int masterX = getWidth() - masterW - 10;
        const int comboW = 168;

        int x = 10;
        op1.setBounds (x, topY, 24, topH);       x += 24;
        op2.setBounds (x, topY, 24, topH);       x += 24;
        op3.setBounds (x, topY, 24, topH);       x += 24;
        op4.setBounds (x, topY, 24, topH);       x += 32;

        btVoicePrev.setBounds (x, topY, 26, topH); x += 30;
        editName.setBounds (x, topY, 112, topH);
        const int nameX = x;
        x += 116;
        btVoiceNext.setBounds (x, topY, 26, topH); x += 34;

        const int comboX = jmax (x + 8, masterX - comboW - 12);
        const int readW = jmax (120, comboX - x - 8);
        btReadVoiceFromSy99.setBounds (x, topY, readW, topH);

        comboMode.setBounds (comboX, topY, comboW, topH);
        sliderMaster.setBounds (masterX, topY, masterW, 20);

        labelName.setBounds (nameX, labelY, 112, 16);
        labelMode.setBounds (comboX, labelY, comboW, 16);
        labelOpOn.setBounds (10, labelY, 96, 16);
        labelMasterVolume.setBounds (masterX, labelY, masterW, 16);
        
// Redraw celon le nombre d'elements
     
        element1.setBounds(10, 52, wGrid, hGrid);
        element2.setBounds(10, 52 + hGrid,wGrid, hGrid );
        element3.setBounds(10,  52 + (hGrid * 2),wGrid, hGrid);
        element4.setBounds(10,  52 + (hGrid*3), wGrid,hGrid) ;

        // Voice Common Params panel — right column under the master volume slider.
        const int panelX = (int) (wGrid + 20);
        const int panelY = 52;
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
    TextButton btVoicePrev { TRANS ("Prev") };
    TextButton btVoiceNext { TRANS ("Next") };
    TextButton btReadVoiceFromSy99 { TRANS ("Sync from SY99") };
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
    /** After user edits editName (ignored during inbound VNAM / LIB mirror). Clear after successful OSC queue. */
    bool voiceNameDirtyForMidiOut = false;

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

    MidiSlider  sliderEfmode;
    MidiSlider  sliderMixerEl1Level;
    MidiSlider  sliderMixerEl1Outsel;
    MidiSlider  sliderMixerEl1Efsendsel;
    MidiSlider  sliderMixerEl1Efsendlvl;
    MidiSlider  sliderMixerEl1Efsendvsns;
    MidiSlider  sliderMixerEl1Efsendscl;
    MidiSlider  sliderMixerEl2Level;
    MidiSlider  sliderMixerEl2Outsel;
    MidiSlider  sliderMixerEl2Efsendsel;
    MidiSlider  sliderMixerEl2Efsendlvl;
    MidiSlider  sliderMixerEl2Efsendvsns;
    MidiSlider  sliderMixerEl2Efsendscl;
    MidiSlider  sliderMixerEl3Level;
    MidiSlider  sliderMixerEl4Level;
    MidiSlider  sliderMixerEl1NoteLimitLow;
    MidiSlider  sliderMixerEl2NoteLimitLow;
    MidiSlider  sliderMixerEl3NoteLimitLow;
    MidiSlider  sliderMixerEl4NoteLimitLow;
    MidiSlider  sliderMixerEl1NoteLimitHigh;
    MidiSlider  sliderMixerEl2NoteLimitHigh;
    MidiSlider  sliderMixerEl3NoteLimitHigh;
    MidiSlider  sliderMixerEl4NoteLimitHigh;
    MidiSlider  sliderMixerEl1VelocityLimitLow;
    MidiSlider  sliderMixerEl2VelocityLimitLow;
    MidiSlider  sliderMixerEl3VelocityLimitLow;
    MidiSlider  sliderMixerEl4VelocityLimitLow;
    MidiSlider  sliderMixerEl1VelocityLimitHigh;
    MidiSlider  sliderMixerEl2VelocityLimitHigh;
    MidiSlider  sliderMixerEl3VelocityLimitHigh;
    MidiSlider  sliderMixerEl4VelocityLimitHigh;
    MidiSlider  sliderMixerEl1Detune;
    MidiSlider  sliderMixerEl2Detune;
    MidiSlider  sliderMixerEl3Detune;
    MidiSlider  sliderMixerEl4Detune;
    MidiSlider  sliderMixerEl1NoteShift;
    MidiSlider  sliderMixerEl2NoteShift;
    MidiSlider  sliderMixerEl3NoteShift;
    MidiSlider  sliderMixerEl4NoteShift;

    bool isEditorPatchDirtySlider (Slider* slider) noexcept
    {
        if (slider == &sliderMaster
            || slider == &sliderWPBR || slider == &sliderATPBR
            || slider == &sliderPMRNG || slider == &sliderAMRNG || slider == &sliderFMRNG
            || slider == &sliderPNLRNG || slider == &sliderCORNG || slider == &sliderPNBRNG
            || slider == &sliderEGBRNG || slider == &sliderWLLML || slider == &sliderMCTUN
            || slider == &sliderRNDP || slider == &sliderSPTPNT
            || slider == &sliderMixerEl1Level || slider == &sliderMixerEl2Level
            || slider == &sliderMixerEl3Level || slider == &sliderMixerEl4Level
            || slider == &sliderMixerEl1Outsel || slider == &sliderMixerEl2Outsel
            || slider == &sliderMixerEl1Efsendsel || slider == &sliderMixerEl1Efsendlvl
            || slider == &sliderMixerEl1Efsendvsns
            || slider == &sliderMixerEl2Efsendsel || slider == &sliderMixerEl2Efsendlvl
            || slider == &sliderMixerEl2Efsendvsns || slider == &sliderMixerEl2Efsendscl
            || slider == &sliderMixerEl1NoteLimitLow || slider == &sliderMixerEl2NoteLimitLow
            || slider == &sliderMixerEl3NoteLimitLow || slider == &sliderMixerEl4NoteLimitLow
            || slider == &sliderMixerEl1NoteLimitHigh || slider == &sliderMixerEl2NoteLimitHigh
            || slider == &sliderMixerEl3NoteLimitHigh || slider == &sliderMixerEl4NoteLimitHigh
            || slider == &sliderMixerEl1VelocityLimitLow || slider == &sliderMixerEl2VelocityLimitLow
            || slider == &sliderMixerEl3VelocityLimitLow || slider == &sliderMixerEl4VelocityLimitLow
            || slider == &sliderMixerEl1VelocityLimitHigh || slider == &sliderMixerEl2VelocityLimitHigh
            || slider == &sliderMixerEl3VelocityLimitHigh || slider == &sliderMixerEl4VelocityLimitHigh
            || slider == &sliderMixerEl1Detune || slider == &sliderMixerEl2Detune
            || slider == &sliderMixerEl3Detune || slider == &sliderMixerEl4Detune
            || slider == &sliderMixerEl1NoteShift || slider == &sliderMixerEl2NoteShift
            || slider == &sliderMixerEl3NoteShift || slider == &sliderMixerEl4NoteShift)
            return true;

        return slider == &element1.getDetuneSlider()
            || slider == &element1.getNoteShiftSlider()
            || slider == &element2.getDetuneSlider()
            || slider == &element2.getNoteShiftSlider()
            || slider == &element3.getDetuneSlider()
            || slider == &element3.getNoteShiftSlider()
            || slider == &element4.getDetuneSlider()
            || slider == &element4.getNoteShiftSlider();
    }

    bool isEditorPatchDirtyCombo (ComboBox* combo) noexcept
    {
        return combo == &comboMode || combo == &comboPMASN || combo == &comboAMASN
            || combo == &comboFMASN || combo == &comboPNLASN || combo == &comboCOASN
            || combo == &comboPNBASN || combo == &comboEGBASN || combo == &comboWLASN
            || combo == &comboAFTMD;
    }

    struct EditorPatchDirtySuppressGuard final
    {
        EditorPatchDirtySuppressGuard() noexcept { suppressEditorPatchDirtyMark() = true; }
        ~EditorPatchDirtySuppressGuard() noexcept { suppressEditorPatchDirtyMark() = false; }
        JUCE_DECLARE_NON_COPYABLE (EditorPatchDirtySuppressGuard)
    };

    void setupEditorPatchDirtyListeners()
    {
        MidiSlider* const sliders[] = {
            &sliderMaster,
            &sliderWPBR, &sliderATPBR, &sliderPMRNG, &sliderAMRNG, &sliderFMRNG,
            &sliderPNLRNG, &sliderCORNG, &sliderPNBRNG, &sliderEGBRNG,
            &sliderWLLML, &sliderMCTUN, &sliderRNDP, &sliderSPTPNT,
            &sliderMixerEl1Level, &sliderMixerEl2Level, &sliderMixerEl3Level, &sliderMixerEl4Level,
            &sliderMixerEl1Outsel, &sliderMixerEl2Outsel,
            &sliderMixerEl1Efsendsel, &sliderMixerEl1Efsendlvl, &sliderMixerEl1Efsendvsns,
            &sliderMixerEl2Efsendsel, &sliderMixerEl2Efsendlvl, &sliderMixerEl2Efsendvsns,
            &sliderMixerEl2Efsendscl,
            &sliderMixerEl1NoteLimitLow, &sliderMixerEl2NoteLimitLow,
            &sliderMixerEl3NoteLimitLow, &sliderMixerEl4NoteLimitLow,
            &sliderMixerEl1NoteLimitHigh, &sliderMixerEl2NoteLimitHigh,
            &sliderMixerEl3NoteLimitHigh, &sliderMixerEl4NoteLimitHigh,
            &sliderMixerEl1VelocityLimitLow, &sliderMixerEl2VelocityLimitLow,
            &sliderMixerEl3VelocityLimitLow, &sliderMixerEl4VelocityLimitLow,
            &sliderMixerEl1VelocityLimitHigh, &sliderMixerEl2VelocityLimitHigh,
            &sliderMixerEl3VelocityLimitHigh, &sliderMixerEl4VelocityLimitHigh,
            &sliderMixerEl1Detune, &sliderMixerEl2Detune,
            &sliderMixerEl3Detune, &sliderMixerEl4Detune,
            &sliderMixerEl1NoteShift, &sliderMixerEl2NoteShift,
            &sliderMixerEl3NoteShift, &sliderMixerEl4NoteShift,
        };

        for (auto* s : sliders)
            s->addListener (this);

        Element* const elements[] = { &element1, &element2, &element3, &element4 };

        for (auto* el : elements)
        {
            el->getDetuneSlider().addListener (this);
            el->getNoteShiftSlider().addListener (this);
        }

        ComboBox* const combos[] = {
            &comboPMASN, &comboAMASN, &comboFMASN, &comboPNLASN, &comboCOASN,
            &comboPNBASN, &comboEGBASN, &comboWLASN, &comboAFTMD,
        };

        for (auto* c : combos)
            c->addListener (this);
    }

    void removeEditorPatchDirtyListeners()
    {
        MidiSlider* const sliders[] = {
            &sliderMaster,
            &sliderWPBR, &sliderATPBR, &sliderPMRNG, &sliderAMRNG, &sliderFMRNG,
            &sliderPNLRNG, &sliderCORNG, &sliderPNBRNG, &sliderEGBRNG,
            &sliderWLLML, &sliderMCTUN, &sliderRNDP, &sliderSPTPNT,
            &sliderMixerEl1Level, &sliderMixerEl2Level, &sliderMixerEl3Level, &sliderMixerEl4Level,
            &sliderMixerEl1Outsel, &sliderMixerEl2Outsel,
            &sliderMixerEl1Efsendsel, &sliderMixerEl1Efsendlvl, &sliderMixerEl1Efsendvsns,
            &sliderMixerEl2Efsendsel, &sliderMixerEl2Efsendlvl, &sliderMixerEl2Efsendvsns,
            &sliderMixerEl2Efsendscl,
            &sliderMixerEl1NoteLimitLow, &sliderMixerEl2NoteLimitLow,
            &sliderMixerEl3NoteLimitLow, &sliderMixerEl4NoteLimitLow,
            &sliderMixerEl1NoteLimitHigh, &sliderMixerEl2NoteLimitHigh,
            &sliderMixerEl3NoteLimitHigh, &sliderMixerEl4NoteLimitHigh,
            &sliderMixerEl1VelocityLimitLow, &sliderMixerEl2VelocityLimitLow,
            &sliderMixerEl3VelocityLimitLow, &sliderMixerEl4VelocityLimitLow,
            &sliderMixerEl1VelocityLimitHigh, &sliderMixerEl2VelocityLimitHigh,
            &sliderMixerEl3VelocityLimitHigh, &sliderMixerEl4VelocityLimitHigh,
            &sliderMixerEl1Detune, &sliderMixerEl2Detune,
            &sliderMixerEl3Detune, &sliderMixerEl4Detune,
            &sliderMixerEl1NoteShift, &sliderMixerEl2NoteShift,
            &sliderMixerEl3NoteShift, &sliderMixerEl4NoteShift,
        };

        for (auto* s : sliders)
            s->removeListener (this);

        Element* const elements[] = { &element1, &element2, &element3, &element4 };

        for (auto* el : elements)
        {
            el->getDetuneSlider().removeListener (this);
            el->getNoteShiftSlider().removeListener (this);
        }

        ComboBox* const combos[] = {
            &comboPMASN, &comboAMASN, &comboFMASN, &comboPNLASN, &comboCOASN,
            &comboPNBASN, &comboEGBASN, &comboWLASN, &comboAFTMD,
        };

        for (auto* c : combos)
            c->removeListener (this);
    }

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
        sliderATPBR.setVoiceCommonSymmetric12SysexEncode (true);  // SY99 ATPBR ladder: 1C..11, 00, 01..0C
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
        setupCommonParam (sliderMCTUN,  labMCTUN,  0x3A,   0, 127);
        setupCommonParam (sliderRNDP,   labRNDP,   0x3B,   0,   7);
        setupAFTMD();
        setupCommonParam (sliderSPTPNT, labSPTPNT, 0x43,   0, 127);
    }

    void setupMixerElLevel (MidiSlider& slider, uint8 elementOffset, const Identifier& volumeId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x00, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (0, 127, 100);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);

        Value valueElLevel = valueTreeVoice.getPropertyAsValue (volumeId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElLevel);
    }

    void setupMixerElNoteLimitLow (MidiSlider& slider, uint8 elementOffset, const Identifier& noteLimitLowId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x03, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (0, 127, 0);
        slider.setMidiNoteNumberTextDisplay();
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 40, 16);

        Value valueElNoteLimitLow = valueTreeVoice.getPropertyAsValue (noteLimitLowId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElNoteLimitLow);
    }

    void setupMixerElNoteLimitHigh (MidiSlider& slider, uint8 elementOffset, const Identifier& noteLimitHighId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x04, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (0, 127, 127);
        slider.setMidiNoteNumberTextDisplay();
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 40, 16);

        const bool hasSavedNoteLimitHigh = valueTreeVoice.hasProperty (noteLimitHighId);
        Value valueElNoteLimitHigh = valueTreeVoice.getPropertyAsValue (noteLimitHighId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElNoteLimitHigh);

        // TEMP (ENLH): far-right (127) until patch/pattern restore loads ELEMENTnNOTELIMITHIGH — not a stored patch value.
        if (!hasSavedNoteLimitHigh)
            slider.setValue (127, dontSendNotification);
    }

    void setupMixerElVelocityLimitLow (MidiSlider& slider, uint8 elementOffset,
                                       const Identifier& velocityLimitLowId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x05, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (0, 127, 0);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);

        Value valueElVelocityLimitLow = valueTreeVoice.getPropertyAsValue (velocityLimitLowId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElVelocityLimitLow);
    }

    void setupMixerElVelocityLimitHigh (MidiSlider& slider, uint8 elementOffset,
                                        const Identifier& velocityLimitHighId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x06, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (0, 127, 127);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);

        Value valueElVelocityLimitHigh = valueTreeVoice.getPropertyAsValue (velocityLimitHighId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElVelocityLimitHigh);
    }

    void setupMixerElDetune (MidiSlider& slider, uint8 elementOffset, const Identifier& fineId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x01, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (-7, 7, 0);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);
        slider.setLookAndFeel (nullptr);
        slider.setNumDecimalPlacesToDisplay (0);
        slider.setDoubleClickReturnValue (true, 0);

        Value valueElFine = valueTreeVoice.getPropertyAsValue (fineId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElFine);
    }

    void setupMixerElNoteShift (MidiSlider& slider, uint8 elementOffset, const Identifier& pitchId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x02, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setElementNoteShiftSignedSysexEncode (true);
        slider.setRange (-64, 63, 1);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);
        slider.setNumDecimalPlacesToDisplay (0);
        slider.setDoubleClickReturnValue (true, 0);

        Value valueElPitch = valueTreeVoice.getPropertyAsValue (pitchId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElPitch);
    }

    void setupMixerElOutsel (MidiSlider& slider, uint8 elementOffset, const Identifier& outselId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x08, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (0, 127, 0);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);

        Value valueElOutsel = valueTreeVoice.getPropertyAsValue (outselId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElOutsel);
    }

    void setupMixerElEfsendsel (MidiSlider& slider, uint8 elementOffset, const Identifier& efsendselId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x09, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (0, 127, 0);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);

        Value valueElEfsendsel = valueTreeVoice.getPropertyAsValue (efsendselId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElEfsendsel);
    }

    /** Global EFMODE (08 00 00 20): hidden receive-only; drives Send 3 visibility on El.1 mixer strip. */
    void setupEffectEfmode (MidiSlider& slider)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x08, 0x00, 0x00, 0x20, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (0, 2, 0);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);

        Value valueEfmode = valueTreeVoice.getPropertyAsValue (IDs::EFMODE.toString(), &undoManager);
        slider.getValueObject().referTo (valueEfmode);
    }

    void setupMixerElEfsendlvl (MidiSlider& slider, uint8 elementOffset, const Identifier& efsendlvlId)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x0A, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (0, 127, 0);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);

        Value valueElEfsendlvl = valueTreeVoice.getPropertyAsValue (efsendlvlId.toString(), &undoManager);
        slider.getValueObject().referTo (valueElEfsendlvl);
    }

    void setupMixerElEfsendSigned7 (MidiSlider& slider, uint8 elementOffset, uint8 nn,
                                    const Identifier& id)
    {
        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, nn, 0x00, 0x00 };
        slider.setMidiSysex (sysex);
        slider.setRangeAndRound (-7, 7, 0);
        slider.setMixerEffectSendSigned7SysexEncode (true);
        slider.setSliderStyle (Slider::LinearHorizontal);
        slider.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);

        Value valueEl = valueTreeVoice.getPropertyAsValue (id.toString(), &undoManager);
        slider.getValueObject().referTo (valueEl);
    }

    void initMixerLevelControls()
    {
#if JUCE_DEBUG
        {
            static bool outselUiBitmapLogged = false;

            if (! outselUiBitmapLogged)
            {
                Logger::writeToLog ("[OUTSEL audit] ui-bitmap: off=0x00 G1=0x02 G2=0x04 both=0x06"
                                    " (MixerOutputGroupBinding out1Bit/out2Bit mask=0x06)");
                outselUiBitmapLogged = true;
            }
        }
#endif
        setupEffectEfmode (sliderEfmode);
        setupMixerElLevel (sliderMixerEl1Level, 0x00, IDs::ELEMENT1VOLUME);
        setupMixerElOutsel (sliderMixerEl1Outsel, 0x00, IDs::ELEMENT1OUTSEL);
        setupMixerElEfsendsel (sliderMixerEl1Efsendsel, 0x00, IDs::ELEMENT1EFSENDSEL);
        setupMixerElEfsendlvl (sliderMixerEl1Efsendlvl, 0x00, IDs::ELEMENT1EFSENDLVL);
        setupMixerElEfsendSigned7 (sliderMixerEl1Efsendvsns, 0x00, 0x0B, IDs::ELEMENT1EFSENDVSNS);
        setupMixerElEfsendSigned7 (sliderMixerEl1Efsendscl, 0x00, 0x0C, IDs::ELEMENT1EFSENDSCL);
        setupMixerElLevel (sliderMixerEl2Level, 0x20, IDs::ELEMENT2VOLUME);
        setupMixerElOutsel (sliderMixerEl2Outsel, 0x20, IDs::ELEMENT2OUTSEL);
        setupMixerElEfsendsel (sliderMixerEl2Efsendsel, 0x20, IDs::ELEMENT2EFSENDSEL);
        setupMixerElEfsendlvl (sliderMixerEl2Efsendlvl, 0x20, IDs::ELEMENT2EFSENDLVL);
        setupMixerElEfsendSigned7 (sliderMixerEl2Efsendvsns, 0x20, 0x0B, IDs::ELEMENT2EFSENDVSNS);
        setupMixerElEfsendSigned7 (sliderMixerEl2Efsendscl, 0x20, 0x0C, IDs::ELEMENT2EFSENDSCL);
        setupMixerElLevel (sliderMixerEl3Level, 0x40, IDs::ELEMENT3VOLUME);
        setupMixerElLevel (sliderMixerEl4Level, 0x60, IDs::ELEMENT4VOLUME);
        setupMixerElNoteLimitLow (sliderMixerEl1NoteLimitLow, 0x00, IDs::ELEMENT1NOTELIMITLOW);
        setupMixerElNoteLimitLow (sliderMixerEl2NoteLimitLow, 0x20, IDs::ELEMENT2NOTELIMITLOW);
        setupMixerElNoteLimitLow (sliderMixerEl3NoteLimitLow, 0x40, IDs::ELEMENT3NOTELIMITLOW);
        setupMixerElNoteLimitLow (sliderMixerEl4NoteLimitLow, 0x60, IDs::ELEMENT4NOTELIMITLOW);
        setupMixerElNoteLimitHigh (sliderMixerEl1NoteLimitHigh, 0x00, IDs::ELEMENT1NOTELIMITHIGH);
        setupMixerElNoteLimitHigh (sliderMixerEl2NoteLimitHigh, 0x20, IDs::ELEMENT2NOTELIMITHIGH);
        setupMixerElNoteLimitHigh (sliderMixerEl3NoteLimitHigh, 0x40, IDs::ELEMENT3NOTELIMITHIGH);
        setupMixerElNoteLimitHigh (sliderMixerEl4NoteLimitHigh, 0x60, IDs::ELEMENT4NOTELIMITHIGH);
        setupMixerElVelocityLimitLow (sliderMixerEl1VelocityLimitLow, 0x00, IDs::ELEMENT1VELOCITYLIMITLOW);
        setupMixerElVelocityLimitLow (sliderMixerEl2VelocityLimitLow, 0x20, IDs::ELEMENT2VELOCITYLIMITLOW);
        setupMixerElVelocityLimitLow (sliderMixerEl3VelocityLimitLow, 0x40, IDs::ELEMENT3VELOCITYLIMITLOW);
        setupMixerElVelocityLimitLow (sliderMixerEl4VelocityLimitLow, 0x60, IDs::ELEMENT4VELOCITYLIMITLOW);
        setupMixerElVelocityLimitHigh (sliderMixerEl1VelocityLimitHigh, 0x00, IDs::ELEMENT1VELOCITYLIMITHIGH);
        setupMixerElVelocityLimitHigh (sliderMixerEl2VelocityLimitHigh, 0x20, IDs::ELEMENT2VELOCITYLIMITHIGH);
        setupMixerElVelocityLimitHigh (sliderMixerEl3VelocityLimitHigh, 0x40, IDs::ELEMENT3VELOCITYLIMITHIGH);
        setupMixerElVelocityLimitHigh (sliderMixerEl4VelocityLimitHigh, 0x60, IDs::ELEMENT4VELOCITYLIMITHIGH);
        setupMixerElDetune (sliderMixerEl1Detune, 0x00, IDs::ELEMENT1FINE);
        setupMixerElDetune (sliderMixerEl2Detune, 0x20, IDs::ELEMENT2FINE);
        setupMixerElDetune (sliderMixerEl3Detune, 0x40, IDs::ELEMENT3FINE);
        setupMixerElDetune (sliderMixerEl4Detune, 0x60, IDs::ELEMENT4FINE);
        setupMixerElNoteShift (sliderMixerEl1NoteShift, 0x00, IDs::ELEMENT1PITCH);
        setupMixerElNoteShift (sliderMixerEl2NoteShift, 0x20, IDs::ELEMENT2PITCH);
        setupMixerElNoteShift (sliderMixerEl3NoteShift, 0x40, IDs::ELEMENT3PITCH);
        setupMixerElNoteShift (sliderMixerEl4NoteShift, 0x60, IDs::ELEMENT4PITCH);
    }

    void layoutCommonPanel()
    {
        const int rowH = 18;
        const int w    = commonPanel.getWidth();
        int y = 0;
        labelCommon.setBounds (0, y, w, rowH);
    }

    /** Common-tab reparented rows may live under VoiceCommonPage; pull back before destroying VoicePage / tab teardown. */
    void ensurePitchControlsParentedToCommonPanel()
    {
        auto restoreSliderRow = [&] (Label& lab, MidiSlider& s)
        {
            if (lab.getParentComponent() == &commonPanel && s.getParentComponent() == &commonPanel)
                return;
            if (auto* p = lab.getParentComponent())
                p->removeChildComponent (&lab);
            if (auto* p = s.getParentComponent())
                p->removeChildComponent (&s);
            commonPanel.addAndMakeVisible (lab);
            commonPanel.addAndMakeVisible (s);
            s.setPopupDisplayEnabled (true, true, &commonPanel);
        };
        auto restoreComboRow = [&] (Label& lab, MidiCombo& c)
        {
            if (lab.getParentComponent() == &commonPanel && c.getParentComponent() == &commonPanel)
                return;
            if (auto* p = lab.getParentComponent())
                p->removeChildComponent (&lab);
            if (auto* p = c.getParentComponent())
                p->removeChildComponent (&c);
            commonPanel.addAndMakeVisible (lab);
            commonPanel.addAndMakeVisible (c);
        };
        restoreSliderRow (labWPBR, sliderWPBR);
        restoreSliderRow (labRNDP, sliderRNDP);
        restoreSliderRow (labMCTUN, sliderMCTUN);
        restoreSliderRow (labSPTPNT, sliderSPTPNT);
        restoreComboRow (labAFTMD, comboAFTMD);
        restoreSliderRow (labATPBR, sliderATPBR);
        restoreComboRow (labPNLASN, comboPNLASN);
        restoreSliderRow (labPNLRNG, sliderPNLRNG);
        restoreComboRow (labPNBASN, comboPNBASN);
        restoreSliderRow (labPNBRNG, sliderPNBRNG);
        restoreComboRow (labPMASN, comboPMASN);
        restoreSliderRow (labPMRNG, sliderPMRNG);
        restoreComboRow (labAMASN, comboAMASN);
        restoreSliderRow (labAMRNG, sliderAMRNG);
        restoreComboRow (labFMASN, comboFMASN);
        restoreSliderRow (labFMRNG, sliderFMRNG);
        restoreComboRow (labCOASN, comboCOASN);
        restoreSliderRow (labCORNG, sliderCORNG);
        restoreComboRow (labEGBASN, comboEGBASN);
        restoreSliderRow (labEGBRNG, sliderEGBRNG);
        restoreComboRow (labWLASN, comboWLASN);
        restoreSliderRow (labWLLML, sliderWLLML);
    }

    
    UndoManager undoManager;
};

//==============================================================================
/** Voice Common tab: hosts panels migrated incrementally from VoicePage::commonPanel. */
struct VoiceCommonPage : public Component,
                         private ChangeListener
{
    explicit VoiceCommonPage (VoicePage& voicePageRef)
        : voice (voicePageRef)
    {
        voice.addChangeListener (this);

        addAndMakeVisible (viewport);
        viewport.setViewedComponent (&content, false);
        viewport.setScrollBarsShown (true, false);

        content.addAndMakeVisible (mixerGroup);
        mixerGroup.addAndMakeVisible (mixerSection);
        content.addAndMakeVisible (pitchGroup);
        content.addAndMakeVisible (microTuneGroup);
        content.addAndMakeVisible (afterTouchGroup);
        content.addAndMakeVisible (controllersGroup);
        content.addAndMakeVisible (panGroup);
        content.addAndMakeVisible (randomPitchGroup);
        content.addAndMakeVisible (otherGroup);

        auto reparentPitchSliderRow = [&] (Label& lab, MidiSlider& s)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&s);
            pitchGroup.addAndMakeVisible (lab);
            pitchGroup.addAndMakeVisible (s);
            s.setPopupDisplayEnabled (true, true, &pitchGroup);
        };

        reparentPitchSliderRow (voice.labWPBR, voice.sliderWPBR);

        auto reparentMicroTuneSliderRow = [&] (Label& lab, MidiSlider& s)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&s);
            microTuneGroup.addAndMakeVisible (lab);
            microTuneGroup.addAndMakeVisible (s);
            s.setPopupDisplayEnabled (true, true, &microTuneGroup);
        };

        reparentMicroTuneSliderRow (voice.labMCTUN, voice.sliderMCTUN);

        auto reparentAfterTouchSliderRow = [&] (Label& lab, MidiSlider& s)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&s);
            afterTouchGroup.addAndMakeVisible (lab);
            afterTouchGroup.addAndMakeVisible (s);
            s.setPopupDisplayEnabled (true, true, &afterTouchGroup);
        };
        auto reparentAfterTouchComboRow = [&] (Label& lab, MidiCombo& c)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&c);
            afterTouchGroup.addAndMakeVisible (lab);
            afterTouchGroup.addAndMakeVisible (c);
        };

        reparentAfterTouchSliderRow (voice.labSPTPNT, voice.sliderSPTPNT);
        reparentAfterTouchComboRow  (voice.labAFTMD,  voice.comboAFTMD);
        reparentAfterTouchSliderRow (voice.labATPBR,  voice.sliderATPBR);

        auto reparentControllersCombo = [&] (Label& lab, MidiCombo& c)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&c);
            controllersGroup.addAndMakeVisible (lab);
            controllersGroup.addAndMakeVisible (c);
        };
        auto reparentControllersSlider = [&] (Label& lab, MidiSlider& s)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&s);
            controllersGroup.addAndMakeVisible (lab);
            controllersGroup.addAndMakeVisible (s);
            s.setPopupDisplayEnabled (true, true, &controllersGroup);
        };

        reparentControllersCombo  (voice.labPMASN,  voice.comboPMASN);
        reparentControllersSlider (voice.labPMRNG,  voice.sliderPMRNG);
        reparentControllersCombo  (voice.labAMASN,  voice.comboAMASN);
        reparentControllersSlider (voice.labAMRNG,  voice.sliderAMRNG);
        reparentControllersCombo  (voice.labFMASN,  voice.comboFMASN);
        reparentControllersSlider (voice.labFMRNG,  voice.sliderFMRNG);

        auto reparentPanComboRow = [&] (Label& lab, MidiCombo& c)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&c);
            panGroup.addAndMakeVisible (lab);
            panGroup.addAndMakeVisible (c);
        };
        auto reparentPanSliderRow = [&] (Label& lab, MidiSlider& s)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&s);
            panGroup.addAndMakeVisible (lab);
            panGroup.addAndMakeVisible (s);
            s.setPopupDisplayEnabled (true, true, &panGroup);
        };

        reparentPanComboRow  (voice.labPNLASN, voice.comboPNLASN);
        reparentPanSliderRow (voice.labPNLRNG, voice.sliderPNLRNG);
        reparentPanComboRow  (voice.labPNBASN, voice.comboPNBASN);
        reparentPanSliderRow (voice.labPNBRNG, voice.sliderPNBRNG);

        auto reparentRandomPitchSliderRow = [&] (Label& lab, MidiSlider& s)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&s);
            randomPitchGroup.addAndMakeVisible (lab);
            randomPitchGroup.addAndMakeVisible (s);
            s.setPopupDisplayEnabled (true, true, &randomPitchGroup);
        };

        reparentRandomPitchSliderRow (voice.labRNDP, voice.sliderRNDP);

        auto reparentOtherCombo = [&] (Label& lab, MidiCombo& c)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&c);
            otherGroup.addAndMakeVisible (lab);
            otherGroup.addAndMakeVisible (c);
        };
        auto reparentOtherSlider = [&] (Label& lab, MidiSlider& s)
        {
            voice.commonPanel.removeChildComponent (&lab);
            voice.commonPanel.removeChildComponent (&s);
            otherGroup.addAndMakeVisible (lab);
            otherGroup.addAndMakeVisible (s);
            s.setPopupDisplayEnabled (true, true, &otherGroup);
        };

        reparentOtherCombo  (voice.labCOASN,  voice.comboCOASN);
        reparentOtherSlider (voice.labCORNG,  voice.sliderCORNG);
        reparentOtherCombo  (voice.labEGBASN, voice.comboEGBASN);
        reparentOtherSlider (voice.labEGBRNG, voice.sliderEGBRNG);
        reparentOtherCombo  (voice.labWLASN,  voice.comboWLASN);
        reparentOtherSlider (voice.labWLLML,  voice.sliderWLLML);

        mixerSection.attachStripLevelControl (0, voice.sliderMixerEl1Level);
        mixerSection.attachStripLevelControl (1, voice.sliderMixerEl2Level);
        mixerSection.attachStripLevelControl (2, voice.sliderMixerEl3Level);
        mixerSection.attachStripLevelControl (3, voice.sliderMixerEl4Level);
        voice.sliderMixerEl1Level.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2Level.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl3Level.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl4Level.setPopupDisplayEnabled (true, true, &mixerGroup);

        mixerSection.attachStripNoteLimitLowControl (0, voice.sliderMixerEl1NoteLimitLow);
        mixerSection.attachStripNoteLimitLowControl (1, voice.sliderMixerEl2NoteLimitLow);
        mixerSection.attachStripNoteLimitLowControl (2, voice.sliderMixerEl3NoteLimitLow);
        mixerSection.attachStripNoteLimitLowControl (3, voice.sliderMixerEl4NoteLimitLow);
        voice.sliderMixerEl1NoteLimitLow.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2NoteLimitLow.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl3NoteLimitLow.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl4NoteLimitLow.setPopupDisplayEnabled (true, true, &mixerGroup);

        mixerSection.attachStripNoteLimitHighControl (0, voice.sliderMixerEl1NoteLimitHigh);
        mixerSection.attachStripNoteLimitHighControl (1, voice.sliderMixerEl2NoteLimitHigh);
        mixerSection.attachStripNoteLimitHighControl (2, voice.sliderMixerEl3NoteLimitHigh);
        mixerSection.attachStripNoteLimitHighControl (3, voice.sliderMixerEl4NoteLimitHigh);
        voice.sliderMixerEl1NoteLimitHigh.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2NoteLimitHigh.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl3NoteLimitHigh.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl4NoteLimitHigh.setPopupDisplayEnabled (true, true, &mixerGroup);

        mixerSection.attachStripVelocityLimitLowControl (0, voice.sliderMixerEl1VelocityLimitLow);
        mixerSection.attachStripVelocityLimitLowControl (1, voice.sliderMixerEl2VelocityLimitLow);
        mixerSection.attachStripVelocityLimitLowControl (2, voice.sliderMixerEl3VelocityLimitLow);
        mixerSection.attachStripVelocityLimitLowControl (3, voice.sliderMixerEl4VelocityLimitLow);
        voice.sliderMixerEl1VelocityLimitLow.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2VelocityLimitLow.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl3VelocityLimitLow.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl4VelocityLimitLow.setPopupDisplayEnabled (true, true, &mixerGroup);

        mixerSection.attachStripVelocityLimitHighControl (0, voice.sliderMixerEl1VelocityLimitHigh);
        mixerSection.attachStripVelocityLimitHighControl (1, voice.sliderMixerEl2VelocityLimitHigh);
        mixerSection.attachStripVelocityLimitHighControl (2, voice.sliderMixerEl3VelocityLimitHigh);
        mixerSection.attachStripVelocityLimitHighControl (3, voice.sliderMixerEl4VelocityLimitHigh);
        voice.sliderMixerEl1VelocityLimitHigh.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2VelocityLimitHigh.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl3VelocityLimitHigh.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl4VelocityLimitHigh.setPopupDisplayEnabled (true, true, &mixerGroup);

        mixerSection.attachStripDetuneControl (0, voice.sliderMixerEl1Detune);
        mixerSection.attachStripDetuneControl (1, voice.sliderMixerEl2Detune);
        mixerSection.attachStripDetuneControl (2, voice.sliderMixerEl3Detune);
        mixerSection.attachStripDetuneControl (3, voice.sliderMixerEl4Detune);
        voice.sliderMixerEl1Detune.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2Detune.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl3Detune.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl4Detune.setPopupDisplayEnabled (true, true, &mixerGroup);

        mixerSection.attachStripNoteShiftControl (0, voice.sliderMixerEl1NoteShift);
        mixerSection.attachStripNoteShiftControl (1, voice.sliderMixerEl2NoteShift);
        mixerSection.attachStripNoteShiftControl (2, voice.sliderMixerEl3NoteShift);
        mixerSection.attachStripNoteShiftControl (3, voice.sliderMixerEl4NoteShift);
        voice.sliderMixerEl1NoteShift.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2NoteShift.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl3NoteShift.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl4NoteShift.setPopupDisplayEnabled (true, true, &mixerGroup);

        mixerSection.bindOutputGroupForStrip (0, mixerEl1OutputGroup, voice.sliderMixerEl1Outsel);
        mixerSection.bindOutputGroupForStrip (1, mixerEl2OutputGroup, voice.sliderMixerEl2Outsel);
        mixerSection.bindEffectSendSelectForStrip (0, mixerEl1EffectSendSelect,
                                                   voice.sliderMixerEl1Efsendsel,
                                                   &voice.sliderEfmode);
        mixerSection.bindEffectSendSelectForStrip (1, mixerEl2EffectSendSelect,
                                                   voice.sliderMixerEl2Efsendsel,
                                                   &voice.sliderEfmode);
        mixerSection.attachStripEffectSendControls (0,
                                                    voice.sliderMixerEl1Efsendlvl,
                                                    voice.sliderMixerEl1Efsendvsns,
                                                    voice.sliderMixerEl1Efsendscl);
        mixerSection.attachStripEffectSendControls (1,
                                                    voice.sliderMixerEl2Efsendlvl,
                                                    voice.sliderMixerEl2Efsendvsns,
                                                    voice.sliderMixerEl2Efsendscl);
        voice.sliderMixerEl1Efsendlvl.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl1Efsendvsns.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl1Efsendscl.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2Efsendlvl.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2Efsendvsns.setPopupDisplayEnabled (true, true, &mixerGroup);
        voice.sliderMixerEl2Efsendscl.setPopupDisplayEnabled (true, true, &mixerGroup);

        syncMixerStripVisibility();

        refreshMixerBoundControlsCallback() = [this]()
        {
            refreshMixerBoundControls();
        };

        syncCommonMixerLayoutCallback() = [this]()
        {
            syncMixerStripVisibility();
        };
    }

    void refreshMixerBoundControls() noexcept
    {
        mixerEl1OutputGroup.refreshButtonsFromSlider();
        mixerEl2OutputGroup.refreshButtonsFromSlider();
        mixerEl1EffectSendSelect.refreshButtonsFromSlider();
        mixerEl2EffectSendSelect.refreshButtonsFromSlider();
    }

    ~VoiceCommonPage() override
    {
        refreshMixerBoundControlsCallback() = nullptr;
        syncCommonMixerLayoutCallback() = nullptr;
        voice.removeChangeListener (this);
        voice.ensurePitchControlsParentedToCommonPanel();
    }

    void visibilityChanged() override
    {
        Component::visibilityChanged();

        if (isShowing() && isVisible())
            syncMixerStripVisibility();
    }

    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        if (source == &voice)
            syncMixerStripVisibility();
    }

    void syncMixerStripVisibility()
    {
        mixerSection.setActiveStripCount (voice.nombreElements);

        Element* elements[] = { &voice.element1, &voice.element2,
                                &voice.element3, &voice.element4 };
        for (int i = 0; i < 4; ++i)
            mixerSection.setStripEngineType (i, elements[i]->getOpMode());

        mixerSection.relayoutLiveStripRows();

        Logger::writeToLog ("[Mixer layout] activeStrips=" + String (mixerSection.getActiveStripCount())
                            + " nombreElements=" + String (voice.nombreElements));

        resized();
    }

    void resized() override
    {
        viewport.setBounds (getLocalBounds());

        // Compact two-column layout so Common tab fits one screenshot without scrolling.
        const int outerPad = 6;
        const int rowH = 14;
        const int gap = 1;
        const int labW = 44;
        const int groupInsetX = 8;
        const int groupInsetY = 12;
        const int gapBetweenGroups = 4;
        const int colGap = 6;
        const int pairGap = 6;

        const auto groupHeightForRows = [&] (int rows) noexcept
        {
            return groupInsetY + rows * (rowH + gap) + 6;
        };

        const int mixerInnerH = MixerSection::preferredHeight();
        const int mixerGroupH = groupInsetY * 2 + mixerInnerH + 6;
        const int pitchGroupH = groupHeightForRows (1);
        const int microTuneGroupH = groupHeightForRows (1);
        const int afterTouchGroupH = groupHeightForRows (3);
        const int controllersGroupH = groupHeightForRows (3);
        const int panGroupH = groupHeightForRows (4);
        const int randomPitchGroupH = groupHeightForRows (1);
        const int otherGroupH = groupHeightForRows (3);

        const int contentW = jmax (160, viewport.getMaximumVisibleWidth());
        const int fullW = jmax (160, contentW - 2 * outerPad);
        const int colW = jmax (120, (fullW - colGap) / 2);

        int yCur = outerPad;
        mixerGroup.setBounds (outerPad, yCur, fullW, mixerGroupH);
        yCur += mixerGroupH + gapBetweenGroups;

        const int paramsTop = yCur;
        int leftY = paramsTop;
        pitchGroup.setBounds (outerPad, leftY, colW, pitchGroupH);
        leftY += pitchGroupH + gapBetweenGroups;
        microTuneGroup.setBounds (outerPad, leftY, colW, microTuneGroupH);
        leftY += microTuneGroupH + gapBetweenGroups;
        afterTouchGroup.setBounds (outerPad, leftY, colW, afterTouchGroupH);
        leftY += afterTouchGroupH + gapBetweenGroups;
        randomPitchGroup.setBounds (outerPad, leftY, colW, randomPitchGroupH);
        leftY += randomPitchGroupH + outerPad;

        const int rightX = outerPad + colW + colGap;
        int rightY = paramsTop;
        controllersGroup.setBounds (rightX, rightY, colW, controllersGroupH);
        rightY += controllersGroupH + gapBetweenGroups;
        panGroup.setBounds (rightX, rightY, colW, panGroupH);
        rightY += panGroupH + gapBetweenGroups;
        otherGroup.setBounds (rightX, rightY, colW, otherGroupH);
        rightY += otherGroupH + outerPad;

        yCur = jmax (leftY, rightY);
        content.setSize (contentW, yCur);

        const int viewH = viewport.getMaximumVisibleHeight();
        viewport.setScrollBarsShown (yCur > viewH + 2, false);

        {
            auto box = mixerGroup.getLocalBounds().reduced (groupInsetX, groupInsetY);
            const int mixerInnerW = MixerSection::preferredContentWidth (mixerSection.getActiveStripCount(),
                                                                         box.getWidth());
            const int mixerInnerH = MixerSection::preferredHeight();
            mixerSection.setBounds (box.getX(), box.getY(),
                                    jmin (mixerInnerW, box.getWidth()),
                                    jmin (box.getHeight(), mixerInnerH));
        }
        {
            auto box = pitchGroup.getLocalBounds().reduced (groupInsetX, groupInsetY);
            int y = box.getY();
            const int x0 = box.getX();
            const int w = box.getWidth();
            auto row = [&] (Label& l, Component& c)
            {
                l.setBounds (x0, y, labW, rowH);
                c.setBounds (x0 + labW, y, w - labW, rowH);
                y += rowH + gap;
            };
            row (voice.labWPBR, voice.sliderWPBR);
        }
        {
            auto box = microTuneGroup.getLocalBounds().reduced (groupInsetX, groupInsetY);
            int y = box.getY();
            const int x0 = box.getX();
            const int w = box.getWidth();
            auto row = [&] (Label& l, Component& c)
            {
                l.setBounds (x0, y, labW, rowH);
                c.setBounds (x0 + labW, y, w - labW, rowH);
                y += rowH + gap;
            };
            row (voice.labMCTUN, voice.sliderMCTUN);
        }
        {
            auto box = afterTouchGroup.getLocalBounds().reduced (groupInsetX, groupInsetY);
            int y = box.getY();
            const int x0 = box.getX();
            const int w = box.getWidth();
            auto row = [&] (Label& l, Component& c)
            {
                l.setBounds (x0, y, labW, rowH);
                c.setBounds (x0 + labW, y, w - labW, rowH);
                y += rowH + gap;
            };
            row (voice.labSPTPNT, voice.sliderSPTPNT);
            row (voice.labAFTMD,  voice.comboAFTMD);
            row (voice.labATPBR,  voice.sliderATPBR);
        }
        {
            auto box = panGroup.getLocalBounds().reduced (groupInsetX, groupInsetY);
            int y = box.getY();
            const int x0 = box.getX();
            const int w = box.getWidth();
            auto row = [&] (Label& l, Component& c)
            {
                l.setBounds (x0, y, labW, rowH);
                c.setBounds (x0 + labW, y, w - labW, rowH);
                y += rowH + gap;
            };
            row (voice.labPNLASN, voice.comboPNLASN);
            row (voice.labPNLRNG, voice.sliderPNLRNG);
            row (voice.labPNBASN, voice.comboPNBASN);
            row (voice.labPNBRNG, voice.sliderPNBRNG);
        }
        {
            auto box = controllersGroup.getLocalBounds().reduced (groupInsetX, groupInsetY);
            int y = box.getY();
            const int x0 = box.getX();
            const int innerW = box.getWidth();
            const int half = (innerW - pairGap) / 2;

            auto pairRow = [&] (Label& labA, MidiCombo& comboA, Label& labR, MidiSlider& sliderR)
            {
                const int leftW = half;
                const int rightW = innerW - half - pairGap;
                const int leftX = x0;
                const int rightX = x0 + leftW + pairGap;
                labA.setBounds (leftX, y, labW, rowH);
                comboA.setBounds (leftX + labW, y, leftW - labW, rowH);
                labR.setBounds (rightX, y, labW, rowH);
                sliderR.setBounds (rightX + labW, y, rightW - labW, rowH);
                y += rowH + gap;
            };

            pairRow (voice.labPMASN,  voice.comboPMASN,  voice.labPMRNG, voice.sliderPMRNG);
            pairRow (voice.labAMASN, voice.comboAMASN, voice.labAMRNG, voice.sliderAMRNG);
            pairRow (voice.labFMASN, voice.comboFMASN, voice.labFMRNG, voice.sliderFMRNG);
        }
        {
            auto box = randomPitchGroup.getLocalBounds().reduced (groupInsetX, groupInsetY);
            int y = box.getY();
            const int x0 = box.getX();
            const int w = box.getWidth();
            auto row = [&] (Label& l, Component& c)
            {
                l.setBounds (x0, y, labW, rowH);
                c.setBounds (x0 + labW, y, w - labW, rowH);
                y += rowH + gap;
            };
            row (voice.labRNDP, voice.sliderRNDP);
        }
        {
            auto box = otherGroup.getLocalBounds().reduced (groupInsetX, groupInsetY);
            int y = box.getY();
            const int x0 = box.getX();
            const int innerW = box.getWidth();
            const int half = (innerW - pairGap) / 2;

            auto pairRow = [&] (Label& labA, MidiCombo& comboA, Label& labR, MidiSlider& sliderR)
            {
                const int leftW = half;
                const int rightW = innerW - half - pairGap;
                const int leftX = x0;
                const int rightX = x0 + leftW + pairGap;
                labA.setBounds (leftX, y, labW, rowH);
                comboA.setBounds (leftX + labW, y, leftW - labW, rowH);
                labR.setBounds (rightX, y, labW, rowH);
                sliderR.setBounds (rightX + labW, y, rightW - labW, rowH);
                y += rowH + gap;
            };

            pairRow (voice.labCOASN, voice.comboCOASN, voice.labCORNG, voice.sliderCORNG);
            pairRow (voice.labEGBASN, voice.comboEGBASN, voice.labEGBRNG, voice.sliderEGBRNG);
            pairRow (voice.labWLASN, voice.comboWLASN, voice.labWLLML, voice.sliderWLLML);
        }
    }

private:
    VoicePage& voice;
    Viewport viewport;
    Component content;
    GroupComponent mixerGroup       { {}, TRANS ("Mixer") };
    MixerSection mixerSection;
    GroupComponent pitchGroup      { {}, TRANS ("Pitch Bend") };
    GroupComponent microTuneGroup  { {}, TRANS ("Micro Tune") };
    GroupComponent afterTouchGroup { {}, TRANS ("After Touch") };
    GroupComponent controllersGroup { {}, TRANS ("Modulation Depth") };
    GroupComponent panGroup        { {}, TRANS ("Pan Control") };
    GroupComponent randomPitchGroup { {}, TRANS ("Random Pitch") };
    GroupComponent otherGroup      { {}, TRANS ("Other") };

    MixerOutputGroupBinding mixerEl1OutputGroup;
    MixerOutputGroupBinding mixerEl2OutputGroup;
    MixerEffectSendSelectBinding mixerEl1EffectSendSelect;
    MixerEffectSendSelectBinding mixerEl2EffectSendSelect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceCommonPage)
};

//==============================================================================

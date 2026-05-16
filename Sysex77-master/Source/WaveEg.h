/*
  ==============================================================================

    WaveEg.h
    Created: 10 Feb 2019 11:27:36am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include "ADSR.h"

class WaveEg    : public ElementComponent, public TextButton::Listener, Slider::Listener, public ChangeListener, public Value::Listener
{
public:
    WaveEg()
    {
        addAndMakeVisible(keyDraw);
        addAndMakeVisible(samplePathLeft);
        addAndMakeVisible(samplePathRight);
        addAndMakeVisible(labelSlope);
        labelSlope.setJustificationType(Justification::centred);
        labelSlope.attachToComponent(&sliderSlope, true);

        sliderSlope.setRangeAndRound(-7, 7, 0);
        sliderSlope.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(sliderSlope);
        sliderSlope.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
        sliderSlope.addListener(this);

        addAndMakeVisible(sliderL0);
        addAndMakeVisible(sliderL1);
        addAndMakeVisible(sliderL2);
        addAndMakeVisible(sliderL3);
        addAndMakeVisible(sliderL4);
        addAndMakeVisible(sliderRL1);
        addAndMakeVisible(sliderRL2);
        addAndMakeVisible(sliderR1);
        addAndMakeVisible(sliderR2);
        addAndMakeVisible(sliderR3);
        addAndMakeVisible(sliderR4);
        addAndMakeVisible(sliderRR1);
        addAndMakeVisible(sliderRR2);

        addAndMakeVisible(labelL0);
        addAndMakeVisible(labelL1);
        addAndMakeVisible(labelL2);
        addAndMakeVisible(labelL3);
        addAndMakeVisible(labelL4);
        addAndMakeVisible(labelRL1);
        addAndMakeVisible(labelRL2);
        addAndMakeVisible(labelR1);
        addAndMakeVisible(labelR2);
        addAndMakeVisible(labelR3);
        addAndMakeVisible(labelR4);
        addAndMakeVisible(labelRR1);
        addAndMakeVisible(labelRR2);
        labelL0.attachToComponent (&sliderL0,  false);
        labelL1.attachToComponent (&sliderL1,  false);
        labelL2.attachToComponent (&sliderL2,  false);
        labelL3.attachToComponent (&sliderL3,  false);
        labelL4.attachToComponent (&sliderL4,  false);
        labelRL1.attachToComponent(&sliderRL1, false);
        labelRL2.attachToComponent(&sliderRL2, false);
        labelR1.attachToComponent (&sliderR1,  false);
        labelR2.attachToComponent (&sliderR2,  false);
        labelR3.attachToComponent (&sliderR3,  false);
        labelR4.attachToComponent (&sliderR4,  false);
        labelRR1.attachToComponent(&sliderRR1, false);
        labelRR2.attachToComponent(&sliderRR2, false);

        addAndMakeVisible(egWave);
        egWave.addChangeListener(this);
        egWave.setModeHold(true);
        egWave.setKeyOffSegment(4);
        egWave.setName("EG AWM Vol");

        // AWM EG R2 E1/E3 — addresses differ between elements so can't use standard
        // element-offset formula; two separate sliders with hardcoded per-element addresses.
        addAndMakeVisible(sliderAwmR2E1);
        labelAwmR2E1.attachToComponent(&sliderAwmR2E1, false);
        sliderAwmR2E1.setRangeAndRound(0, 62, 0);   // E1: display = raw-1 so max raw = 63 → display 62
        sliderAwmR2E1.setPopupDisplayEnabled(true, true, this);

        addAndMakeVisible(sliderAwmR2E3);
        labelAwmR2E3.attachToComponent(&sliderAwmR2E3, false);
        sliderAwmR2E3.setRangeAndRound(0, 63, 0);   // E3: direct encoding
        sliderAwmR2E3.setPopupDisplayEnabled(true, true, this);
    }

    ~WaveEg()
    {
        sliderSlope.removeListener(this);
        sliderL0.getValueObject().removeListener(this);
        sliderL1.getValueObject().removeListener(this);
        sliderL2.getValueObject().removeListener(this);
        sliderL3.getValueObject().removeListener(this);
        sliderL4.getValueObject().removeListener(this);
        sliderRL1.getValueObject().removeListener(this);
        sliderRL2.getValueObject().removeListener(this);
        sliderR1.getValueObject().removeListener(this);
        sliderR2.getValueObject().removeListener(this);
        sliderR3.getValueObject().removeListener(this);
        sliderR4.getValueObject().removeListener(this);
        sliderRR1.getValueObject().removeListener(this);
        sliderRR2.getValueObject().removeListener(this);
    }

    void changeListenerCallback(ChangeBroadcaster* source) override
    {
        if (isUpdating) return;
        if (source == &egWave && egWave.getSegmentCount() >= 6)
        {
            isUpdating = true;
            // Inverse of toPctL: pct = val*100/63 → val = pct*63/100
            auto fromPctL = [](int pct) { return (int)(pct * 63.0 / 100.0); };
            // Inverse of toPctR: pct = (64-val)*100/64 → val = 64 - pct*64/100
            auto fromPctR = [](int pct) { return (int)(64.0 - pct * 64.0 / 100.0); };
            sliderL0.setValue (fromPctL(egWave.getSegmentLevel(0)), dontSendNotification);
            sliderR1.setValue (fromPctR(egWave.getSegmentRate(0)),  dontSendNotification);
            sliderL1.setValue (fromPctL(egWave.getSegmentLevel(1)), dontSendNotification);
            sliderR2.setValue (fromPctR(egWave.getSegmentRate(1)),  dontSendNotification);
            sliderL2.setValue (fromPctL(egWave.getSegmentLevel(2)), dontSendNotification);
            sliderR3.setValue (fromPctR(egWave.getSegmentRate(2)),  dontSendNotification);
            sliderL3.setValue (fromPctL(egWave.getSegmentLevel(3)), dontSendNotification);
            sliderR4.setValue (fromPctR(egWave.getSegmentRate(3)),  dontSendNotification);
            sliderL4.setValue (fromPctL(egWave.getSegmentLevel(4)), dontSendNotification);
            sliderRR1.setValue(fromPctR(egWave.getSegmentRate(4)),  dontSendNotification);
            sliderRL1.setValue(fromPctL(egWave.getSegmentLevel(5)), dontSendNotification);
            sliderRR2.setValue(fromPctR(egWave.getSegmentRate(5)),  dontSendNotification);
            sliderRL2.setValue(fromPctL(egWave.getReleaseLevel()),  dontSendNotification);
            isUpdating = false;
        }
    }

    void sliderValueChanged(Slider* slider) override
    {
        if (isUpdating) return;

        if (slider == &sliderSlope)
            resized();

        if (egWave.getSegmentCount() >= 6)
        {
            isUpdating = true;
            // Levels: AWM unsigned 0..63 → 0-100% display
            auto toPctL = [](double val) { return (int)(val * 100.0 / 63.0); };
            // Rates: AWM unsigned 0..63 → inverted 0-100%
            auto toPctR = [](double val) { return (int)((64.0 - val) * 100.0 / 64.0); };
            if      (slider == &sliderL0)  egWave.setSegmentLevel(0, toPctL(sliderL0.getValue()));
            else if (slider == &sliderR1)  egWave.setSegmentRate (0, toPctR(sliderR1.getValue()));
            else if (slider == &sliderL1)  egWave.setSegmentLevel(1, toPctL(sliderL1.getValue()));
            else if (slider == &sliderR2)  egWave.setSegmentRate (1, toPctR(sliderR2.getValue()));
            else if (slider == &sliderL2)  egWave.setSegmentLevel(2, toPctL(sliderL2.getValue()));
            else if (slider == &sliderR3)  egWave.setSegmentRate (2, toPctR(sliderR3.getValue()));
            else if (slider == &sliderL3)  egWave.setSegmentLevel(3, toPctL(sliderL3.getValue()));
            else if (slider == &sliderR4)  egWave.setSegmentRate (3, toPctR(sliderR4.getValue()));
            else if (slider == &sliderL4)  egWave.setSegmentLevel(4, toPctL(sliderL4.getValue()));
            else if (slider == &sliderRR1) egWave.setSegmentRate (4, toPctR(sliderRR1.getValue()));
            else if (slider == &sliderRL1) egWave.setSegmentLevel(5, toPctL(sliderRL1.getValue()));
            else if (slider == &sliderRL2) egWave.setReleaseLevel(toPctL(sliderRL2.getValue()));
            else if (slider == &sliderRR2)
            {
                egWave.setSegmentRate(5, toPctR(sliderRR2.getValue()));
                egWave.setRelease(toPctR(sliderRR2.getValue()));
            }
            egWave.repaint();
            isUpdating = false;
        }
    }

    void syncSegmentsFromSliders()
    {
        if (egWave.getSegmentCount() < 6) return;
        isUpdating = true;
        auto toPctL = [](double v) { return (int)(v * 100.0 / 63.0); };
        auto toPctR = [](double v) { return (int)((64.0 - v) * 100.0 / 64.0); };
        egWave.setSegmentLevel(0, toPctL(sliderL0.getValue()));
        egWave.setSegmentRate (0, toPctR(sliderR1.getValue()));
        egWave.setSegmentLevel(1, toPctL(sliderL1.getValue()));
        egWave.setSegmentRate (1, toPctR(sliderR2.getValue()));
        egWave.setSegmentLevel(2, toPctL(sliderL2.getValue()));
        egWave.setSegmentRate (2, toPctR(sliderR3.getValue()));
        egWave.setSegmentLevel(3, toPctL(sliderL3.getValue()));
        egWave.setSegmentRate (3, toPctR(sliderR4.getValue()));
        egWave.setSegmentLevel(4, toPctL(sliderL4.getValue()));
        egWave.setSegmentRate (4, toPctR(sliderRR1.getValue()));
        egWave.setSegmentLevel(5, toPctL(sliderRL1.getValue()));
        egWave.setSegmentRate (5, toPctR(sliderRR2.getValue()));
        egWave.setRelease(toPctR(sliderRR2.getValue()));
        egWave.setReleaseLevel(toPctL(sliderRL2.getValue()));
        isUpdating = false;
    }

    void visibilityChanged() override
    {
        if (!isVisible()) return;
        juce::MessageManager::callAsync([this]()
        {
            if (!isVisible()) return;
            syncSegmentsFromSliders();
            egWave.setBounds(0, 0,
                (int)(getWidth()  * 0.65f),
                (int)(getHeight() * 0.80f));
            egWave.resized();
            egWave.repaint();
        });
    }

    void valueChanged(Value& /*v*/) override
    {
        if (isUpdating) return;
        syncSegmentsFromSliders();
        egWave.repaint();
    }

    void setElementNumber(int element, UndoManager& um) override
    {
        Logger::writeToLog("WaveEg setElement");
        egWave.clearSegments();

        // Legacy template (b3=0x00) preserved only for slope/levels that have no
        // confirmed manual address. Rate sliders below are remapped to the AWM
        // Amp EG block (b3=0x07, NN=0x50..0x54 per sy99_sysex_complete.md / Group 0x07).
        int sysexdata[9]  = { 0x43, 0x10, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        int sysexdata2[9] = { 0x43, 0x10, 0x34, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 };

        int elemOffset = 0;
        if      (element == 2) elemOffset = 0x20;
        else if (element == 3) elemOffset = 0x40;
        else if (element == 4) elemOffset = 0x60;
        sysexdata[4]  = elemOffset;
        sysexdata2[4] = elemOffset;

        String e = String(element);
        sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue("EGAWMVOL_SLOPE_E" + e, &um));
        sliderL0.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGAWMVOL_L0_E"   + e, &um));
        sliderL1.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGAWMVOL_L1_E"   + e, &um));
        sliderL2.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGAWMVOL_L2_E"   + e, &um));
        sliderL3.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGAWMVOL_L3_E"   + e, &um));
        sliderL4.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGAWMVOL_L4_E"   + e, &um));
        sliderRL1.getValueObject().referTo (valueTreeVoice.getPropertyAsValue("EGAWMVOL_RL1_E"  + e, &um));
        sliderRL2.getValueObject().referTo (valueTreeVoice.getPropertyAsValue("EGAWMVOL_RL2_E"  + e, &um));
        sliderR1.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGAWMVOL_R1_E"   + e, &um));
        sliderR2.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGAWMVOL_R2_E"   + e, &um));
        sliderR3.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGAWMVOL_R3_E"   + e, &um));
        sliderR4.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGAWMVOL_R4_E"   + e, &um));
        sliderRR1.getValueObject().referTo (valueTreeVoice.getPropertyAsValue("EGAWMVOL_RR1_E"  + e, &um));
        sliderRR2.getValueObject().referTo (valueTreeVoice.getPropertyAsValue("EGAWMVOL_RR2_E"  + e, &um));

        // sliderSlope → PARS / Amp EG Rate Scaling (0x07, NN=0x57). UI range -7..+7
        // (set in constructor). Byte encoding for PARS likely two's-complement; pending HW check.
        int sysexSlope[9] = { 0x43, 0x10, 0x34, 0x07, elemOffset, 0x00, 0x57, 0x00, 0 };
        sliderSlope.setMidiSysex(sysexSlope);

        auto toPctL = [](double val) { return (int)(val * 100.0 / 63.0); };
        auto toPctR = [](double val) { return (int)((64.0 - val) * 100.0 / 64.0); };

        // Segment 0: L0 → R1 → L1
        // sliderR1 → AWM PARI / Amp EG Attack Rate (0x07, NN=0x50). E1: display=raw-1 (0..62).
        sysexdata[6] = 0x00; sysexdata2[6] = 0x01;
        egWave.addSegment(toPctL(sliderL0.getValue()), toPctR(sliderR1.getValue()), "L0\xe2\x86\x92L1", 63, sysexdata, 63, sysexdata2);
        sliderL0.setMidiSysex(sysexdata);  sliderL0.setRangeAndRound(0, 63, 0);
        int sysexR1[9] = { 0x43, 0x10, 0x34, 0x07, elemOffset, 0x00, 0x50, 0x00, 0 };
        sliderR1.setMidiSysex(sysexR1);
        sliderR1.setRangeAndRound(0, (element == 1 ? 62 : 63), 0);

        // Segment 1: L1 → R2 → L2 — sliderR2 = PAR2 / Decay Rate (0x07, NN=0x51).
        sysexdata[6] = 0x02; sysexdata2[6] = 0x03;
        egWave.addSegment(toPctL(sliderL1.getValue()), toPctR(sliderR2.getValue()), "L1\xe2\x86\x92L2", 63, sysexdata, 63, sysexdata2);
        sliderL1.setMidiSysex(sysexdata);  sliderL1.setRangeAndRound(0, 63, 0);
        int sysexR2[9] = { 0x43, 0x10, 0x34, 0x07, elemOffset, 0x00, 0x51, 0x00, 0 };
        sliderR2.setMidiSysex(sysexR2);    sliderR2.setRangeAndRound(0, 63, 0);

        // Segment 2: L2 → R3 → L3 — sliderR3=PAR3 (0x52), sliderL2=PAL2 (0x55).
        sysexdata[6] = 0x04; sysexdata2[6] = 0x05;
        egWave.addSegment(toPctL(sliderL2.getValue()), toPctR(sliderR3.getValue()), "L2\xe2\x86\x92L3", 63, sysexdata, 63, sysexdata2);
        int sysexL2[9] = { 0x43, 0x10, 0x34, 0x07, elemOffset, 0x00, 0x55, 0x00, 0 };
        sliderL2.setMidiSysex(sysexL2);    sliderL2.setRangeAndRound(0, 63, 0);
        int sysexR3[9] = { 0x43, 0x10, 0x34, 0x07, elemOffset, 0x00, 0x52, 0x00, 0 };
        sliderR3.setMidiSysex(sysexR3);    sliderR3.setRangeAndRound(0, 63, 0);

        // Segment 3: L3 → R4 → L4 — sliderR4=PAR4 (0x53), sliderL3=PAL3 (0x56).
        sysexdata[6] = 0x06; sysexdata2[6] = 0x07;
        egWave.addSegment(toPctL(sliderL3.getValue()), toPctR(sliderR4.getValue()), "L3\xe2\x86\x92L4", 63, sysexdata, 63, sysexdata2);
        int sysexL3[9] = { 0x43, 0x10, 0x34, 0x07, elemOffset, 0x00, 0x56, 0x00, 0 };
        sliderL3.setMidiSysex(sysexL3);    sliderL3.setRangeAndRound(0, 63, 0);
        int sysexR4[9] = { 0x43, 0x10, 0x34, 0x07, elemOffset, 0x00, 0x53, 0x00, 0 };
        sliderR4.setMidiSysex(sysexR4);    sliderR4.setRangeAndRound(0, 63, 0);

        // Segment 4: L4 → RR1 → RL1 — sliderRR1=PARR1 (0x54); L4/RL1 stay legacy b3=0x00.
        sysexdata[6] = 0x08; sysexdata2[6] = 0x09;
        egWave.addSegment(toPctL(sliderL4.getValue()), toPctR(sliderRR1.getValue()), "L4\xe2\x86\x92RL1", 63, sysexdata, 63, sysexdata2);
        sliderL4.setMidiSysex(sysexdata);  sliderL4.setRangeAndRound(0, 63, 0);
        int sysexRR1[9] = { 0x43, 0x10, 0x34, 0x07, elemOffset, 0x00, 0x54, 0x00, 0 };
        sliderRR1.setMidiSysex(sysexRR1);  sliderRR1.setRangeAndRound(0, 63, 0);

        // Segment 5: RL1 → RR2 → RL2
        sysexdata[6] = 0x0a; sysexdata2[6] = 0x0b;
        egWave.addSegment(toPctL(sliderRL1.getValue()), toPctR(sliderRR2.getValue()), "RL1\xe2\x86\x92RL2", 63, sysexdata, 63, sysexdata2);
        sliderRL1.setMidiSysex(sysexdata);  sliderRL1.setRangeAndRound(0, 63, 0);
        sliderRR2.setMidiSysex(sysexdata2); sliderRR2.setRangeAndRound(0, 63, 0);

        sysexdata[6] = 0x0c;
        sliderRL2.setMidiSysex(sysexdata);
        sliderRL2.setRangeAndRound(0, 63, 0);
        egWave.setRelease(toPctR(sliderRR2.getValue()));
        egWave.setReleaseLevel(toPctL(sliderRL2.getValue()));
        egWave.repaint();

        // AWM EG R2 E1 — confirmed: b3=0x07, b4=0x00, b6=0x50; range 0..62 (display=raw-1)
        // AWM EG R2 E3 — confirmed: b3=0x07, b4=0x40, b6=0x51; range 0..63 direct
        // Addresses are not derivable from the standard element-offset formula, hence hardcoded.
        int sysexAwmR2E1[9] = { 0x43, 0x10, 0x34, 0x07, 0x00, 0x00, 0x50, 0x00, 0 };
        sliderAwmR2E1.setMidiSysex(sysexAwmR2E1);
        sliderAwmR2E1.setRangeAndRound(0, 62, 0);

        int sysexAwmR2E3[9] = { 0x43, 0x10, 0x34, 0x07, 0x40, 0x00, 0x51, 0x00, 0 };
        sliderAwmR2E3.setMidiSysex(sysexAwmR2E3);
        sliderAwmR2E3.setRangeAndRound(0, 63, 0);

        // Register Value listeners AFTER referTo()
        sliderL0.getValueObject().addListener(this);
        sliderL1.getValueObject().addListener(this);
        sliderL2.getValueObject().addListener(this);
        sliderL3.getValueObject().addListener(this);
        sliderL4.getValueObject().addListener(this);
        sliderRL1.getValueObject().addListener(this);
        sliderRL2.getValueObject().addListener(this);
        sliderR1.getValueObject().addListener(this);
        sliderR2.getValueObject().addListener(this);
        sliderR3.getValueObject().addListener(this);
        sliderR4.getValueObject().addListener(this);
        sliderRR1.getValueObject().addListener(this);
        sliderRR2.getValueObject().addListener(this);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
        DBG("WaveEg bounds: " + String(getWidth()) + "x" + String(getHeight())
            + "  egWave: " + egWave.getBounds().toString());
    }

    void buttonClicked(Button* /*button*/) override
    {
        setVisible(false);
    }

    void resized() override
    {
        const int w = getWidth();
        const int h = getHeight();

        egWave.setBounds(0, 0, (int)(w * 0.65f), (int)(h * 0.80f));
        egWave.resized();
        egWave.repaint();

        keyDraw.setBoundsRelative(0.0f, 0.85f, 1.0f, 0.09f);
        float sampleWidth = 0.2f + (float)(sliderSlope.getValue() / 100.0);
        samplePathLeft.setBoundsRelative(0.0f, 0.93f, sampleWidth, 0.09f);
        sampleWidth = 0.2f - (float)(sliderSlope.getValue() / 100.0);
        samplePathRight.setBoundsRelative(1.0f - sampleWidth, 0.93f, sampleWidth, 0.09f);

        sliderSlope.setBoundsRelative(0.4f, 0.92f, 0.2f, 0.1f);

        // Levels (top row): L0, L1, L2, L3, L4 | RL1, RL2
        sliderL0.setBoundsRelative (0.65f, 0.06f, 0.04f, 0.3f);
        sliderL1.setBoundsRelative (0.70f, 0.06f, 0.04f, 0.3f);
        sliderL2.setBoundsRelative (0.75f, 0.06f, 0.04f, 0.3f);
        sliderL3.setBoundsRelative (0.80f, 0.06f, 0.04f, 0.3f);
        sliderL4.setBoundsRelative (0.84f, 0.06f, 0.04f, 0.3f);
        sliderRL1.setBoundsRelative(0.90f, 0.06f, 0.04f, 0.3f);
        sliderRL2.setBoundsRelative(0.95f, 0.06f, 0.04f, 0.3f);

        // Rates (bottom row): R1, R2, R3, R4 | RR1, RR2
        sliderR1.setBoundsRelative (0.67f, 0.40f, 0.04f, 0.3f);
        sliderR2.setBoundsRelative (0.72f, 0.40f, 0.04f, 0.3f);
        sliderR3.setBoundsRelative (0.77f, 0.40f, 0.04f, 0.3f);
        sliderR4.setBoundsRelative (0.82f, 0.40f, 0.04f, 0.3f);
        sliderRR1.setBoundsRelative(0.87f, 0.40f, 0.04f, 0.3f);
        sliderRR2.setBoundsRelative(0.92f, 0.40f, 0.04f, 0.3f);

        // AWM EG R2 E1/E3 — bottom-right strip between rates and keyDraw
        sliderAwmR2E1.setBoundsRelative(0.65f, 0.74f, 0.12f, 0.08f);
        sliderAwmR2E3.setBoundsRelative(0.80f, 0.74f, 0.12f, 0.08f);
    }

private:
    bool isUpdating = false;
    SADSR egWave;

    Label      labelSlope {"test", "Slope"};
    MidiKeyDraw keyDraw;
    MidiSlider  sliderSlope;
    MidiPath    samplePathLeft;
    MidiPath    samplePathRight;

    Label labelL0  {"", "L0"};
    Label labelL1  {"", "L1"};
    Label labelL2  {"", "L2"};
    Label labelL3  {"", "L3"};
    Label labelL4  {"", "L4"};
    Label labelRL1 {"", "RL1"};
    Label labelRL2 {"", "RL2"};

    Label labelR1  {"", "R1"};
    Label labelR2  {"", "R2"};
    Label labelR3  {"", "R3"};
    Label labelR4  {"", "R4"};
    Label labelRR1 {"", "RR1"};
    Label labelRR2 {"", "RR2"};

    MidiSlider sliderL0;
    MidiSlider sliderL1;
    MidiSlider sliderL2;
    MidiSlider sliderL3;
    MidiSlider sliderL4;
    MidiSlider sliderRL1;
    MidiSlider sliderRL2;

    MidiSlider sliderR1;
    MidiSlider sliderR2;
    MidiSlider sliderR3;
    MidiSlider sliderR4;
    MidiSlider sliderRR1;
    MidiSlider sliderRR2;

    // AWM EG R2 E1/E3 — confirmed addresses differ per element (not formula-derivable)
    MidiSlider sliderAwmR2E1;
    MidiSlider sliderAwmR2E3;
    Label      labelAwmR2E1 {"", "AwmR2 E1"};
    Label      labelAwmR2E3 {"", "AwmR2 E3"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveEg)
};

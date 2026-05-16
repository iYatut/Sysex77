/*
  ==============================================================================

    PitchEG.h
    Created: 12 Feb 2019 12:11:34am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include "ADSR.h"

class PitchEg    : public ElementComponent, public TextButton::Listener, Slider::Listener, public ChangeListener, public Value::Listener
{
public:
    PitchEg()
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
        addAndMakeVisible(sliderRL1);
        addAndMakeVisible(sliderR1);
        addAndMakeVisible(sliderR2);
        addAndMakeVisible(sliderR3);
        addAndMakeVisible(sliderRR1);

        addAndMakeVisible(labelL0);
        addAndMakeVisible(labelL1);
        addAndMakeVisible(labelL2);
        addAndMakeVisible(labelL3);
        addAndMakeVisible(labelRL);
        addAndMakeVisible(labelR1);
        addAndMakeVisible(labelR2);
        addAndMakeVisible(labelR3);
        addAndMakeVisible(labelRR);
        labelL0.attachToComponent(&sliderL0,  false);
        labelL1.attachToComponent(&sliderL1,  false);
        labelL2.attachToComponent(&sliderL2,  false);
        labelL3.attachToComponent(&sliderL3,  false);
        labelRL.attachToComponent(&sliderRL1, false);
        labelR1.attachToComponent(&sliderR1,  false);
        labelR2.attachToComponent(&sliderR2,  false);
        labelR3.attachToComponent(&sliderR3,  false);
        labelRR.attachToComponent(&sliderRR1, false);

        addAndMakeVisible(egPitch);
        egPitch.addChangeListener(this);
        egPitch.setModeHold(true);
        egPitch.setName("EG Pitch");
    }

    ~PitchEg()
    {
        sliderSlope.removeListener(this);
        sliderL0.getValueObject().removeListener(this);
        sliderL1.getValueObject().removeListener(this);
        sliderL2.getValueObject().removeListener(this);
        sliderL3.getValueObject().removeListener(this);
        sliderRL1.getValueObject().removeListener(this);
        sliderR1.getValueObject().removeListener(this);
        sliderR2.getValueObject().removeListener(this);
        sliderR3.getValueObject().removeListener(this);
        sliderRR1.getValueObject().removeListener(this);
    }

    void changeListenerCallback(ChangeBroadcaster* source) override
    {
        if (isUpdating) return;
        if (source == &egPitch && egPitch.getSegmentCount() >= 4)
        {
            isUpdating = true;
            // Inverse of toPctL: pct = (val+64)*100/128 → val = pct*128/100 - 64
            auto fromPctL = [](int pct) { return (int)(pct * 128.0 / 100.0 - 64.0); };
            // Inverse of toPctR: pct = (64-val)*100/64 → val = 64 - pct*64/100
            auto fromPctR = [](int pct) { return (int)(64.0 - pct * 64.0 / 100.0); };
            sliderL0.setValue (fromPctL(egPitch.getSegmentLevel(0)), dontSendNotification);
            sliderR1.setValue (fromPctR(egPitch.getSegmentRate(0)),  dontSendNotification);
            sliderL1.setValue (fromPctL(egPitch.getSegmentLevel(1)), dontSendNotification);
            sliderR2.setValue (fromPctR(egPitch.getSegmentRate(1)),  dontSendNotification);
            sliderL2.setValue (fromPctL(egPitch.getSegmentLevel(2)), dontSendNotification);
            sliderR3.setValue (fromPctR(egPitch.getSegmentRate(2)),  dontSendNotification);
            sliderL3.setValue (fromPctL(egPitch.getSegmentLevel(3)), dontSendNotification);
            sliderRR1.setValue(fromPctR(egPitch.getSegmentRate(3)),  dontSendNotification);
            isUpdating = false;
        }
    }

    void sliderValueChanged(Slider* slider) override
    {
        if (isUpdating) return;

        if (slider == &sliderSlope)
            resized();

        if (egPitch.getSegmentCount() >= 4)
        {
            isUpdating = true;
            // Levels: SY99 signed -64..+63 → 0-100% display (centre=50%)
            auto toPctL = [](double val) { return (int)((val + 64.0) * 100.0 / 128.0); };
            // Rates:  SY99 unsigned 0..63 → inverted 0-100% display
            auto toPctR = [](double val) { return (int)((64.0 - val) * 100.0 / 64.0); };
            if      (slider == &sliderL0)  egPitch.setSegmentLevel(0, toPctL(sliderL0.getValue()));
            else if (slider == &sliderR1)  egPitch.setSegmentRate (0, toPctR(sliderR1.getValue()));
            else if (slider == &sliderL1)  egPitch.setSegmentLevel(1, toPctL(sliderL1.getValue()));
            else if (slider == &sliderR2)  egPitch.setSegmentRate (1, toPctR(sliderR2.getValue()));
            else if (slider == &sliderL2)  egPitch.setSegmentLevel(2, toPctL(sliderL2.getValue()));
            else if (slider == &sliderR3)  egPitch.setSegmentRate (2, toPctR(sliderR3.getValue()));
            else if (slider == &sliderL3)  egPitch.setSegmentLevel(3, toPctL(sliderL3.getValue()));
            else if (slider == &sliderRR1)
            {
                egPitch.setSegmentRate(3, toPctR(sliderRR1.getValue()));
                egPitch.setRelease(toPctR(sliderRR1.getValue()));
            }
            else if (slider == &sliderRL1) egPitch.setReleaseLevel(toPctL(sliderRL1.getValue()));
            egPitch.repaint();
            isUpdating = false;
        }
    }

    void syncSegmentsFromSliders()
    {
        if (egPitch.getSegmentCount() < 4) return;
        isUpdating = true;
        auto toPctL = [](double v) { return (int)((v + 64.0) * 100.0 / 128.0); };
        auto toPctR = [](double v) { return (int)((64.0 - v) * 100.0 / 64.0); };
        egPitch.setSegmentLevel(0, toPctL(sliderL0.getValue()));
        egPitch.setSegmentRate (0, toPctR(sliderR1.getValue()));
        egPitch.setSegmentLevel(1, toPctL(sliderL1.getValue()));
        egPitch.setSegmentRate (1, toPctR(sliderR2.getValue()));
        egPitch.setSegmentLevel(2, toPctL(sliderL2.getValue()));
        egPitch.setSegmentRate (2, toPctR(sliderR3.getValue()));
        egPitch.setSegmentLevel(3, toPctL(sliderL3.getValue()));
        egPitch.setSegmentRate (3, toPctR(sliderRR1.getValue()));
        egPitch.setRelease(toPctR(sliderRR1.getValue()));
        egPitch.setReleaseLevel(toPctL(sliderRL1.getValue()));
        isUpdating = false;
    }

    void visibilityChanged() override
    {
        if (!isVisible()) return;
        juce::MessageManager::callAsync([this]()
        {
            if (!isVisible()) return;
            syncSegmentsFromSliders();
            egPitch.setBounds(0, 0,
                (int)(getWidth()  * 0.65f),
                (int)(getHeight() * 0.80f));
            egPitch.resized();
            egPitch.repaint();
        });
    }

    void valueChanged(Value& /*v*/) override
    {
        if (isUpdating) return;
        syncSegmentsFromSliders();
        egPitch.repaint();
    }

    void setElementNumber(int element, UndoManager& um) override
    {
        Logger::writeToLog("PitchEg setElement");
        egPitch.clearSegments();

        int sysexdata[9]  = { 0x43, 0x10, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        int sysexdata2[9] = { 0x43, 0x10, 0x34, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00 };

        int elemOffset = 0;
        if      (element == 2) elemOffset = 0x20;
        else if (element == 3) elemOffset = 0x40;
        else if (element == 4) elemOffset = 0x60;
        sysexdata[4]  = elemOffset;
        sysexdata2[4] = elemOffset;

        String e = String(element);
        sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue("EGPITCH_SLOPE_E" + e, &um));
        sliderL0.getValueObject().referTo   (valueTreeVoice.getPropertyAsValue("EGPITCH_L0_E"   + e, &um));
        sliderL1.getValueObject().referTo   (valueTreeVoice.getPropertyAsValue("EGPITCH_L1_E"   + e, &um));
        sliderL2.getValueObject().referTo   (valueTreeVoice.getPropertyAsValue("EGPITCH_L2_E"   + e, &um));
        sliderL3.getValueObject().referTo   (valueTreeVoice.getPropertyAsValue("EGPITCH_L3_E"   + e, &um));
        sliderRL1.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPITCH_RL_E"   + e, &um));
        sliderR1.getValueObject().referTo   (valueTreeVoice.getPropertyAsValue("EGPITCH_R1_E"   + e, &um));
        sliderR2.getValueObject().referTo   (valueTreeVoice.getPropertyAsValue("EGPITCH_R2_E"   + e, &um));
        sliderR3.getValueObject().referTo   (valueTreeVoice.getPropertyAsValue("EGPITCH_R3_E"   + e, &um));
        sliderRR1.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPITCH_RR_E"   + e, &um));

        sysexdata[6] = 0x10;
        sliderSlope.setMidiSysex(sysexdata);

        auto toPctL = [](double val) { return (int)((val + 64.0) * 100.0 / 128.0); };
        auto toPctR = [](double val) { return (int)((64.0 - val) * 100.0 / 64.0); };

        // Segment 0: L0 → R1 → L1
        sysexdata[6] = 0x09; sysexdata2[6] = 0x03;
        egPitch.addSegment(toPctL(sliderL0.getValue()), toPctR(sliderR1.getValue()), "L0\xe2\x86\x92L1", 63, sysexdata, 63, sysexdata2);
        sliderL0.setMidiSysex(sysexdata);  sliderL0.setRangeAndRound(-64, 63, 0);
        sliderR1.setMidiSysex(sysexdata2); sliderR1.setRangeAndRound(0, 63, 0);

        // Segment 1: L1 → R2 → L2
        sysexdata[6] = 0x0a; sysexdata2[6] = 0x04;
        egPitch.addSegment(toPctL(sliderL1.getValue()), toPctR(sliderR2.getValue()), "L1\xe2\x86\x92L2", 63, sysexdata, 63, sysexdata2);
        sliderL1.setMidiSysex(sysexdata);  sliderL1.setRangeAndRound(-64, 63, 0);
        sliderR2.setMidiSysex(sysexdata2); sliderR2.setRangeAndRound(0, 63, 0);

        // Segment 2: L2 → R3 → L3
        sysexdata[6] = 0x0b; sysexdata2[6] = 0x05;
        egPitch.addSegment(toPctL(sliderL2.getValue()), toPctR(sliderR3.getValue()), "L2\xe2\x86\x92L3", 63, sysexdata, 63, sysexdata2);
        sliderL2.setMidiSysex(sysexdata);  sliderL2.setRangeAndRound(-64, 63, 0);
        sliderR3.setMidiSysex(sysexdata2); sliderR3.setRangeAndRound(0, 63, 0);

        // Segment 3 (release): L3 → RR → RL
        sysexdata[6] = 0x0c; sysexdata2[6] = 0x06;
        egPitch.addSegment(toPctL(sliderL3.getValue()), toPctR(sliderRR1.getValue()), "L3\xe2\x86\x92RL", 63, sysexdata, 63, sysexdata2);
        sliderL3.setMidiSysex(sysexdata);   sliderL3.setRangeAndRound(-64, 63, 0);
        sliderRR1.setMidiSysex(sysexdata2); sliderRR1.setRangeAndRound(0, 63, 0);

        // Release level (RL) — endpoint after note-off
        sysexdata[6] = 0x0d;
        sliderRL1.setMidiSysex(sysexdata);
        sliderRL1.setRangeAndRound(-64, 63, 0);
        egPitch.setKeyOffSegment(3);
        egPitch.setRelease(toPctR(sliderRR1.getValue()));
        egPitch.setReleaseLevel(toPctL(sliderRL1.getValue()));
        egPitch.repaint();

        // Register Value listeners AFTER referTo() — fires when ValueTree updates sliders
        sliderL0.getValueObject().addListener(this);
        sliderL1.getValueObject().addListener(this);
        sliderL2.getValueObject().addListener(this);
        sliderL3.getValueObject().addListener(this);
        sliderRL1.getValueObject().addListener(this);
        sliderR1.getValueObject().addListener(this);
        sliderR2.getValueObject().addListener(this);
        sliderR3.getValueObject().addListener(this);
        sliderRR1.getValueObject().addListener(this);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
        DBG("PitchEg bounds: " + String(getWidth()) + "x" + String(getHeight())
            + "  egPitch: " + egPitch.getBounds().toString());
    }

    void buttonClicked(Button* /*button*/) override
    {
        setVisible(false);
    }

    void resized() override
    {
        const int w = getWidth();
        const int h = getHeight();

        egPitch.setBounds(0, 0, (int)(w * 0.65f), (int)(h * 0.80f));
        egPitch.resized();
        egPitch.repaint();

        keyDraw.setBoundsRelative(0.0f, 0.85f, 1.0f, 0.09f);
        float sampleWidth = 0.2f + (float)(sliderSlope.getValue() / 100.0);
        samplePathLeft.setBoundsRelative(0.0f, 0.93f, sampleWidth, 0.09f);
        sampleWidth = 0.2f - (float)(sliderSlope.getValue() / 100.0);
        samplePathRight.setBoundsRelative(1.0f - sampleWidth, 0.93f, sampleWidth, 0.09f);

        sliderSlope.setBoundsRelative(0.4f, 0.92f, 0.2f, 0.1f);

        // Levels (top row): L0, L1, L2, L3 | gap | RL
        sliderL0.setBoundsRelative (0.65f, 0.06f, 0.04f, 0.3f);
        sliderL1.setBoundsRelative (0.71f, 0.06f, 0.04f, 0.3f);
        sliderL2.setBoundsRelative (0.77f, 0.06f, 0.04f, 0.3f);
        sliderL3.setBoundsRelative (0.83f, 0.06f, 0.04f, 0.3f);
        sliderRL1.setBoundsRelative(0.93f, 0.06f, 0.04f, 0.3f);

        // Rates (bottom row): R1, R2, R3 | gap | RR
        sliderR1.setBoundsRelative (0.68f, 0.40f, 0.04f, 0.3f);
        sliderR2.setBoundsRelative (0.74f, 0.40f, 0.04f, 0.3f);
        sliderR3.setBoundsRelative (0.80f, 0.40f, 0.04f, 0.3f);
        sliderRR1.setBoundsRelative(0.90f, 0.40f, 0.04f, 0.3f);
    }

private:
    bool isUpdating = false;
    SADSR egPitch;

    Label      labelSlope {"test", "Slope"};
    MidiKeyDraw keyDraw;
    MidiSlider  sliderSlope;
    MidiPath    samplePathLeft;
    MidiPath    samplePathRight;

    // Level labels
    Label labelL0  {"", "L0"};
    Label labelL1  {"", "L1"};
    Label labelL2  {"", "L2"};
    Label labelL3  {"", "L3"};
    Label labelRL  {"", "RL"};

    // Rate labels
    Label labelR1  {"", "R1"};
    Label labelR2  {"", "R2"};
    Label labelR3  {"", "R3"};
    Label labelRR  {"", "RR"};

    // Level sliders: L0..L3 (attack/decay), RL (release level)
    MidiSlider sliderL0;
    MidiSlider sliderL1;
    MidiSlider sliderL2;
    MidiSlider sliderL3;
    MidiSlider sliderRL1;

    // Rate sliders: R1..R3 (attack/decay rates), RR (release rate)
    MidiSlider sliderR1;
    MidiSlider sliderR2;
    MidiSlider sliderR3;
    MidiSlider sliderRR1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchEg)
};

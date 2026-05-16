/*
  ==============================================================================

    Pan.h
    Created: 25 Nov 2018 8:30:27pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include "ADSR.h"

class Pan    : public ElementComponent, public TextButton::Listener, Slider::Listener, public ChangeListener, public Value::Listener
{
public:
    Pan()
    {
        addAndMakeVisible(keyDraw);

        addAndMakeVisible(sliderHT);
        addAndMakeVisible(labelHT);
        labelHT.attachToComponent(&sliderHT, true);
        sliderHT.setRangeAndRound(0, 63, 0);
        sliderHT.setPopupDisplayEnabled(true, true, this);
        sliderHT.addListener(this);

        addAndMakeVisible(sliderSLP);
        addAndMakeVisible(labelSLP);
        labelSLP.attachToComponent(&sliderSLP, true);
        // SLP (SY99): SysEx raw 0…3 = LCD S1…S4 → EG bracket L1…L4 (see MIDI_MAP_OBSERVATIONS.md)
        sliderSLP.setRangeAndRound(0, 3, 0);
        sliderSLP.setPopupDisplayEnabled(true, true, this);
        sliderSLP.addListener(this);

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

        addAndMakeVisible(egPan);
        egPan.addChangeListener(this);
        egPan.setModeHold(true);
        egPan.setKeyOffSegment(4);
        egPan.setName("EG Pan");
    }

    ~Pan()
    {
        sliderHT.removeListener(this);
        sliderSLP.removeListener(this);
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
        if (source == &egPan && egPan.getSegmentCount() >= 6)
        {
            isUpdating = true;
            // Inverse: pct = (val+32)*100/64 → val = pct*64/100 - 32
            auto fromPctL = [](int pct) { return (int)(pct * 64.0 / 100.0 - 32.0); };
            // Pan EG rates on Rate tab: raw 0…63 (journal); legacy R1 also had 0x03/0x07 path to ~95
            auto fromPctR  = [](int pct) { return (int)(64.0 - pct * 64.0 / 100.0); };
            sliderL0.setValue (fromPctL(egPan.getSegmentLevel(0)), dontSendNotification);
            sliderR1.setValue (fromPctR(egPan.getSegmentRate(0)),  dontSendNotification);
            sliderL1.setValue (fromPctL(egPan.getSegmentLevel(1)), dontSendNotification);
            sliderR2.setValue (fromPctR(egPan.getSegmentRate(1)),  dontSendNotification);
            sliderL2.setValue (fromPctL(egPan.getSegmentLevel(2)), dontSendNotification);
            sliderR3.setValue (fromPctR(egPan.getSegmentRate(2)),  dontSendNotification);
            sliderL3.setValue (fromPctL(egPan.getSegmentLevel(3)), dontSendNotification);
            sliderR4.setValue (fromPctR(egPan.getSegmentRate(3)),  dontSendNotification);
            sliderL4.setValue (fromPctL(egPan.getSegmentLevel(4)), dontSendNotification);
            sliderRR1.setValue(fromPctR(egPan.getSegmentRate(4)),  dontSendNotification);
            sliderRL1.setValue(fromPctL(egPan.getSegmentLevel(5)), dontSendNotification);
            sliderRR2.setValue(fromPctR(egPan.getSegmentRate(5)),  dontSendNotification);
            sliderRL2.setValue(fromPctL(egPan.getReleaseLevel()),  dontSendNotification);
            isUpdating = false;
        }
    }

    void sliderValueChanged(Slider* slider) override
    {
        if (isUpdating) return;

        // HT and SLP are independent of the EG segment array
        if (slider == &sliderHT)
        {
            egPan.setHoldTime((int)sliderHT.getValue());
            return;
        }
        if (slider == &sliderSLP)
        {
            egPan.setLoopPoint((int)sliderSLP.getValue() + 1); // raw 0…3 → L1…L4
            return;
        }

        if (egPan.getSegmentCount() >= 6)
        {
            isUpdating = true;
            // Pan levels: -32..+32 → 0-100% (centre=50%)
            auto toPctL = [](double val) { return (int)((val + 32.0) * 100.0 / 64.0); };
            auto toPctR  = [](double val) { return (int)((64.0 - val) * 100.0 / 64.0); };
            if      (slider == &sliderL0)  egPan.setSegmentLevel(0, toPctL(sliderL0.getValue()));
            else if (slider == &sliderR1)  egPan.setSegmentRate (0, toPctR(sliderR1.getValue()));
            else if (slider == &sliderL1)  egPan.setSegmentLevel(1, toPctL(sliderL1.getValue()));
            else if (slider == &sliderR2)  egPan.setSegmentRate (1, toPctR(sliderR2.getValue()));
            else if (slider == &sliderL2)  egPan.setSegmentLevel(2, toPctL(sliderL2.getValue()));
            else if (slider == &sliderR3)  egPan.setSegmentRate (2, toPctR(sliderR3.getValue()));
            else if (slider == &sliderL3)  egPan.setSegmentLevel(3, toPctL(sliderL3.getValue()));
            else if (slider == &sliderR4)  egPan.setSegmentRate (3, toPctR(sliderR4.getValue()));
            else if (slider == &sliderL4)  egPan.setSegmentLevel(4, toPctL(sliderL4.getValue()));
            else if (slider == &sliderRR1) egPan.setSegmentRate (4, toPctR(sliderRR1.getValue()));
            else if (slider == &sliderRL1) egPan.setSegmentLevel(5, toPctL(sliderRL1.getValue()));
            else if (slider == &sliderRL2) egPan.setReleaseLevel(toPctL(sliderRL2.getValue()));
            else if (slider == &sliderRR2)
            {
                egPan.setSegmentRate(5, toPctR(sliderRR2.getValue()));
                egPan.setRelease(toPctR(sliderRR2.getValue()));
            }
            egPan.repaint();
            isUpdating = false;
        }
    }

    void syncSegmentsFromSliders()
    {
        if (egPan.getSegmentCount() < 6) return;
        isUpdating = true;
        auto toPctL = [](double v) { return (int)((v + 32.0) * 100.0 / 64.0); };
        auto toPctR = [](double v) { return (int)((64.0 - v) * 100.0 / 64.0); };
        egPan.setSegmentLevel(0, toPctL(sliderL0.getValue()));
        egPan.setSegmentRate (0, toPctR(sliderR1.getValue()));
        egPan.setSegmentLevel(1, toPctL(sliderL1.getValue()));
        egPan.setSegmentRate (1, toPctR(sliderR2.getValue()));
        egPan.setSegmentLevel(2, toPctL(sliderL2.getValue()));
        egPan.setSegmentRate (2, toPctR(sliderR3.getValue()));
        egPan.setSegmentLevel(3, toPctL(sliderL3.getValue()));
        egPan.setSegmentRate (3, toPctR(sliderR4.getValue()));
        egPan.setSegmentLevel(4, toPctL(sliderL4.getValue()));
        egPan.setSegmentRate (4, toPctR(sliderRR1.getValue()));
        egPan.setSegmentLevel(5, toPctL(sliderRL1.getValue()));
        egPan.setSegmentRate (5, toPctR(sliderRR2.getValue()));
        egPan.setRelease(toPctR(sliderRR2.getValue()));
        egPan.setReleaseLevel(toPctL(sliderRL2.getValue()));
        // HT and SLP are independent of isUpdating guard — always sync
        egPan.setHoldTime ((int)sliderHT.getValue());
        egPan.setLoopPoint((int)sliderSLP.getValue() + 1);
        isUpdating = false;
    }

    void visibilityChanged() override
    {
        if (!isVisible()) return;
        juce::MessageManager::callAsync([this]()
        {
            if (!isVisible()) return;
            syncSegmentsFromSliders();
            egPan.setBounds(0, 0,
                (int)(getWidth()  * 0.65f),
                (int)(getHeight() * 0.80f));
            egPan.resized();
            egPan.repaint();
        });
    }

    void valueChanged(Value& /*v*/) override
    {
        if (isUpdating) return;
        syncSegmentsFromSliders();
        egPan.repaint();
    }

    void setElementNumber(int element, UndoManager& um) override
    {
        Logger::writeToLog("Pan setElement");
        egPan.clearSegments();

        int sysexRate[9]  = { 0x43, 0x10, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        int sysexLevel[9] = { 0x43, 0x10, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        int elemOffset = 0;
        if      (element == 2) elemOffset = 0x20;
        else if (element == 3) elemOffset = 0x40;
        else if (element == 4) elemOffset = 0x60;

        auto applyElem = [&elemOffset](int templ[9])
        {
            templ[4] = elemOffset;
        };

        String e = String(element);
        sliderHT.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_HT_E"  + e, &um));
        sliderL0.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_L0_E"  + e, &um));
        sliderL1.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_L1_E"  + e, &um));
        sliderL2.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_L2_E"  + e, &um));
        sliderL3.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_L3_E"  + e, &um));
        sliderL4.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_L4_E"  + e, &um));
        sliderRL1.getValueObject().referTo (valueTreeVoice.getPropertyAsValue("EGPAN_RL1_E" + e, &um));
        sliderRL2.getValueObject().referTo (valueTreeVoice.getPropertyAsValue("EGPAN_RL2_E" + e, &um));
        sliderR1.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_R1_E"  + e, &um));
        sliderR2.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_R2_E"  + e, &um));
        sliderR3.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_R3_E"  + e, &um));
        sliderR4.getValueObject().referTo  (valueTreeVoice.getPropertyAsValue("EGPAN_R4_E"  + e, &um));
        sliderRR1.getValueObject().referTo (valueTreeVoice.getPropertyAsValue("EGPAN_RR1_E" + e, &um));
        sliderRR2.getValueObject().referTo (valueTreeVoice.getPropertyAsValue("EGPAN_RR2_E" + e, &um));

        auto toPctL   = [](double val) { return (int)((val + 32.0) * 100.0 / 64.0); };
        auto toPctR63 = [](double val) { return (int)((64.0 - val) * 100.0 / 64.0); };

        // Pan EG HT — post-fix capture: block 0x0A param 0x02 (Rate page); legacy also 0x07/0x06
        applyElem (sysexRate);
        sysexRate[3] = 0x0A;
        sysexRate[6] = 0x02;
        sliderHT.setMidiSysex (sysexRate);

        // SLP — block 0x0A, param 0x10
        applyElem (sysexRate);
        sysexRate[3] = 0x0A;
        sysexRate[6] = 0x10;
        sliderSLP.setMidiSysex (sysexRate);
        sliderSLP.setRangeAndRound (0, 3, 0);

        // addSegment: (rate Sysex, level Sysex) per ADSR.h — sysexRate / sysexLevel

        // Segment 0: L0 (0x0A/09) → R1 — Rate tab journal: 0x0A/0x03 (legacy path also 0x03/0x07)
        applyElem (sysexRate);
        sysexRate[3] = 0x0A;
        sysexRate[6] = 0x03;
        applyElem (sysexLevel);
        sysexLevel[3] = 0x0A;
        sysexLevel[6] = 0x09;
        egPan.addSegment (toPctL (sliderL0.getValue()), toPctR63 (sliderR1.getValue()),
                         "L0\xe2\x86\x92L1", 63, sysexRate, 63, sysexLevel);
        sliderL0.setMidiSysex (sysexLevel);  sliderL0.setRangeAndRound (-32, 31, 0);
        sliderR1.setMidiSysex (sysexRate);   sliderR1.setRangeAndRound (0, 63, 0);

        // Segment 1: L1 (0x0A/0A) → Pan R2 candidate (0x0A/04)
        applyElem (sysexRate);
        sysexRate[3] = 0x0A;
        sysexRate[6] = 0x04;
        applyElem (sysexLevel);
        sysexLevel[3] = 0x0A;
        sysexLevel[6] = 0x0A;
        egPan.addSegment (toPctL (sliderL1.getValue()), toPctR63 (sliderR2.getValue()),
                         "L1\xe2\x86\x92L2", 63, sysexRate, 63, sysexLevel);
        sliderL1.setMidiSysex (sysexLevel);  sliderL1.setRangeAndRound (-32, 31, 0);
        sliderR2.setMidiSysex (sysexRate);   sliderR2.setRangeAndRound (0, 63, 0);

        // Segment 2: L2 (0x0B) → Pan R3 (0x0A/05)
        applyElem (sysexRate);
        sysexRate[3] = 0x0A;
        sysexRate[6] = 0x05;
        applyElem (sysexLevel);
        sysexLevel[3] = 0x0A;
        sysexLevel[6] = 0x0B;
        egPan.addSegment (toPctL (sliderL2.getValue()), toPctR63 (sliderR3.getValue()),
                         "L2\xe2\x86\x92L3", 63, sysexRate, 63, sysexLevel);
        sliderL2.setMidiSysex (sysexLevel);  sliderL2.setRangeAndRound (-32, 31, 0);
        sliderR3.setMidiSysex (sysexRate);   sliderR3.setRangeAndRound (0, 63, 0);

        // Segment 3: L3 (0x0C) → Pan R4 (0x0A/06)
        applyElem (sysexRate);
        sysexRate[3] = 0x0A;
        sysexRate[6] = 0x06;
        applyElem (sysexLevel);
        sysexLevel[3] = 0x0A;
        sysexLevel[6] = 0x0C;
        egPan.addSegment (toPctL (sliderL3.getValue()), toPctR63 (sliderR4.getValue()),
                         "L3\xe2\x86\x92L4", 63, sysexRate, 63, sysexLevel);
        sliderL3.setMidiSysex (sysexLevel);  sliderL3.setRangeAndRound (-32, 31, 0);
        sliderR4.setMidiSysex (sysexRate);   sliderR4.setRangeAndRound (0, 63, 0);

        // Segment 4: L4 (0x0D) → RR1 — Rate tab journal: 0x0A/0x07 (legacy guess was 0x29)
        applyElem (sysexRate);
        sysexRate[3] = 0x0A;
        sysexRate[6] = 0x07;
        applyElem (sysexLevel);
        sysexLevel[3] = 0x0A;
        sysexLevel[6] = 0x0D;
        egPan.addSegment (toPctL (sliderL4.getValue()), toPctR63 (sliderRR1.getValue()),
                         "L4\xe2\x86\x92RL1", 63, sysexRate, 63, sysexLevel);
        sliderL4.setMidiSysex (sysexLevel);   sliderL4.setRangeAndRound (-32, 31, 0);
        sliderRR1.setMidiSysex (sysexRate);   sliderRR1.setRangeAndRound (0, 63, 0);

        // Segment 5: RL1 (0x0E) → RR2 — Rate tab journal: 0x0A/0x08 (legacy guess was 0x2C)
        applyElem (sysexRate);
        sysexRate[3] = 0x0A;
        sysexRate[6] = 0x08;
        applyElem (sysexLevel);
        sysexLevel[3] = 0x0A;
        sysexLevel[6] = 0x0E;
        egPan.addSegment (toPctL (sliderRL1.getValue()), toPctR63 (sliderRR2.getValue()),
                         "RL1\xe2\x86\x92RL2", 63, sysexRate, 63, sysexLevel);
        sliderRL1.setMidiSysex (sysexLevel);  sliderRL1.setRangeAndRound (-32, 31, 0);
        sliderRR2.setMidiSysex (sysexRate);   sliderRR2.setRangeAndRound (0, 63, 0);

        applyElem (sysexRate);
        sysexRate[3] = 0x0A;
        sysexRate[6] = 0x0F;
        sliderRL2.setMidiSysex (sysexRate);
        sliderRL2.setRangeAndRound (-32, 31, 0);
        egPan.setRelease (toPctR63 (sliderRR2.getValue()));
        egPan.setReleaseLevel (toPctL (sliderRL2.getValue()));

        // Apply HT and SLP to the EG visualisation
        egPan.setHoldTime ((int)sliderHT.getValue());
        egPan.setLoopPoint((int)sliderSLP.getValue() + 1);

        egPan.repaint();

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
        DBG("Pan bounds: " + String(getWidth()) + "x" + String(getHeight())
            + "  egPan: " + egPan.getBounds().toString());
    }

    void buttonClicked(Button* /*button*/) override
    {
        setVisible(false);
    }

    void resized() override
    {
        const int w = getWidth();
        const int h = getHeight();

        egPan.setBounds(0, 0, (int)(w * 0.65f), (int)(h * 0.80f));
        egPan.resized();
        egPan.repaint();

        keyDraw.setBoundsRelative(0.0f, 0.85f, 1.0f, 0.09f);

        // HT and SLP sliders in the bottom control strip
        sliderHT.setBoundsRelative (0.65f, 0.85f, 0.08f, 0.08f);
        sliderSLP.setBoundsRelative(0.75f, 0.85f, 0.08f, 0.08f);

        // Levels (top row): L0..L4 | RL1, RL2
        sliderL0.setBoundsRelative (0.65f, 0.06f, 0.04f, 0.3f);
        sliderL1.setBoundsRelative (0.70f, 0.06f, 0.04f, 0.3f);
        sliderL2.setBoundsRelative (0.75f, 0.06f, 0.04f, 0.3f);
        sliderL3.setBoundsRelative (0.80f, 0.06f, 0.04f, 0.3f);
        sliderL4.setBoundsRelative (0.84f, 0.06f, 0.04f, 0.3f);
        sliderRL1.setBoundsRelative(0.90f, 0.06f, 0.04f, 0.3f);
        sliderRL2.setBoundsRelative(0.95f, 0.06f, 0.04f, 0.3f);

        // Rates (bottom row): R1..R4 | RR1, RR2
        sliderR1.setBoundsRelative (0.67f, 0.40f, 0.04f, 0.3f);
        sliderR2.setBoundsRelative (0.72f, 0.40f, 0.04f, 0.3f);
        sliderR3.setBoundsRelative (0.77f, 0.40f, 0.04f, 0.3f);
        sliderR4.setBoundsRelative (0.82f, 0.40f, 0.04f, 0.3f);
        sliderRR1.setBoundsRelative(0.87f, 0.40f, 0.04f, 0.3f);
        sliderRR2.setBoundsRelative(0.92f, 0.40f, 0.04f, 0.3f);
    }

private:
    bool isUpdating = false;
    SADSR egPan;

    MidiKeyDraw keyDraw;

    Label labelHT  {"", "HT"};
    Label labelSLP {"", "SLP"};
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

    MidiSlider sliderHT;
    MidiSlider sliderSLP;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pan)
};

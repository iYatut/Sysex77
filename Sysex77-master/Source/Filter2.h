/*
  ==============================================================================

    Filter2.h
    Created: 10 Feb 2019 11:26:20am
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include "ADSR.h"

class Filter2    : public ElementComponent, public TextButton::Listener, Slider::Listener, public ChangeListener, public Value::Listener
{
public:
    Filter2()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        
        addAndMakeVisible(keyDraw);
        addAndMakeVisible(samplePathLeft);
        addAndMakeVisible(samplePathRight);
        addAndMakeVisible(labelSlope);
        labelSlope.setJustificationType(Justification::centred);
        labelSlope.attachToComponent(&sliderSlope, true);
        
        sliderSlope.setRangeAndRound(-7,7,0);
        sliderSlope.setPopupDisplayEnabled(true, true, this);
        addAndMakeVisible(sliderSlope);
        sliderSlope.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
        sliderSlope.addListener(this);
        
        addAndMakeVisible(sliderL0);
        addAndMakeVisible(sliderL1);
        addAndMakeVisible(sliderL2);
        addAndMakeVisible(sliderL3);
        addAndMakeVisible(sliderL4);
        addAndMakeVisible(sliderR0);
        addAndMakeVisible(sliderR1);
        addAndMakeVisible(sliderR2);
        addAndMakeVisible(sliderR3);
        addAndMakeVisible(sliderR4);
        addAndMakeVisible(sliderRL1);
        addAndMakeVisible(sliderRL2);
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
        labelL0.attachToComponent(&sliderL0, false);
        labelL1.attachToComponent(&sliderL1, false);
        labelL2.attachToComponent(&sliderL2, false);
        labelL3.attachToComponent(&sliderL3, false);
        labelL4.attachToComponent(&sliderL4, false);
        labelRL1.attachToComponent(&sliderRL1, false);
        labelRL2.attachToComponent(&sliderRL2, false);
        labelR1.attachToComponent(&sliderR1, false);
        labelR2.attachToComponent(&sliderR2, false);
        labelR3.attachToComponent(&sliderR3, false);
        labelR4.attachToComponent(&sliderR4, false);
        labelRR1.attachToComponent(&sliderRR1, false);
        labelRR2.attachToComponent(&sliderRR2, false);
        
        addAndMakeVisible(egFilter);
        egFilter.addChangeListener(this);
        egFilter.setModeHold(true);
        egFilter.setName("EG Filter 2");
    }
    
    ~Filter2()
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
        if (source == &egFilter && egFilter.getSegmentCount() >= 6)
        {
            isUpdating = true;
            auto fromPctL = [](int pct) { return (int)(pct * 128.0 / 100.0 - 64.0); };
            auto fromPctR = [](int pct) { return (int)(64.0 - pct * 64.0 / 100.0); };
            sliderL0.setValue (fromPctL(egFilter.getSegmentLevel(0)), dontSendNotification);
            sliderR1.setValue (fromPctR(egFilter.getSegmentRate(0)),  dontSendNotification);
            sliderL1.setValue (fromPctL(egFilter.getSegmentLevel(1)), dontSendNotification);
            sliderR2.setValue (fromPctR(egFilter.getSegmentRate(1)),  dontSendNotification);
            sliderL2.setValue (fromPctL(egFilter.getSegmentLevel(2)), dontSendNotification);
            sliderR3.setValue (fromPctR(egFilter.getSegmentRate(2)),  dontSendNotification);
            sliderL3.setValue (fromPctL(egFilter.getSegmentLevel(3)), dontSendNotification);
            sliderR4.setValue (fromPctR(egFilter.getSegmentRate(3)),  dontSendNotification);
            sliderL4.setValue (fromPctL(egFilter.getSegmentLevel(4)), dontSendNotification);
            sliderRR1.setValue(fromPctR(egFilter.getSegmentRate(4)),  dontSendNotification);
            sliderRL1.setValue(fromPctL(egFilter.getSegmentLevel(5)), dontSendNotification);
            sliderRR2.setValue(fromPctR(egFilter.getSegmentRate(5)),  dontSendNotification);
            isUpdating = false;
        }
    }
    void sliderValueChanged (Slider *slider) override
    {
        if (isUpdating) return;

        if (slider == &sliderSlope)
            resized();

        if (egFilter.getSegmentCount() >= 6)
        {
            isUpdating = true;
            auto toPctL = [](double val) { return (int)((val + 64.0) * 100.0 / 128.0); };
            auto toPctR = [](double val) { return (int)((64.0 - val) * 100.0 / 64.0); };
            if      (slider == &sliderL0)  egFilter.setSegmentLevel(0, toPctL(sliderL0.getValue()));
            else if (slider == &sliderR1)  egFilter.setSegmentRate (0, toPctR(sliderR1.getValue()));
            else if (slider == &sliderL1)  egFilter.setSegmentLevel(1, toPctL(sliderL1.getValue()));
            else if (slider == &sliderR2)  egFilter.setSegmentRate (1, toPctR(sliderR2.getValue()));
            else if (slider == &sliderL2)  egFilter.setSegmentLevel(2, toPctL(sliderL2.getValue()));
            else if (slider == &sliderR3)  egFilter.setSegmentRate (2, toPctR(sliderR3.getValue()));
            else if (slider == &sliderL3)  egFilter.setSegmentLevel(3, toPctL(sliderL3.getValue()));
            else if (slider == &sliderR4)  egFilter.setSegmentRate (3, toPctR(sliderR4.getValue()));
            else if (slider == &sliderL4)  egFilter.setSegmentLevel(4, toPctL(sliderL4.getValue()));
            else if (slider == &sliderRR1) egFilter.setSegmentRate (4, toPctR(sliderRR1.getValue()));
            else if (slider == &sliderRL1) egFilter.setSegmentLevel(5, toPctL(sliderRL1.getValue()));
            else if (slider == &sliderRL2) egFilter.setReleaseLevel(toPctL(sliderRL2.getValue()));
            else if (slider == &sliderRR2)
            {
                egFilter.setSegmentRate(5, toPctR(sliderRR2.getValue()));
                egFilter.setRelease(toPctR(sliderRR2.getValue()));
            }
            egFilter.repaint();
            isUpdating = false;
        }
    }

    void syncSegmentsFromSliders()
    {
        if (egFilter.getSegmentCount() < 6) return;
        isUpdating = true;
        auto toPctL = [](double v) { return (int)((v + 64.0) * 100.0 / 128.0); };
        auto toPctR = [](double v) { return (int)((64.0 - v) * 100.0 / 64.0); };
        egFilter.setSegmentLevel(0, toPctL(sliderL0.getValue()));
        egFilter.setSegmentRate (0, toPctR(sliderR1.getValue()));
        egFilter.setSegmentLevel(1, toPctL(sliderL1.getValue()));
        egFilter.setSegmentRate (1, toPctR(sliderR2.getValue()));
        egFilter.setSegmentLevel(2, toPctL(sliderL2.getValue()));
        egFilter.setSegmentRate (2, toPctR(sliderR3.getValue()));
        egFilter.setSegmentLevel(3, toPctL(sliderL3.getValue()));
        egFilter.setSegmentRate (3, toPctR(sliderR4.getValue()));
        egFilter.setSegmentLevel(4, toPctL(sliderL4.getValue()));
        egFilter.setSegmentRate (4, toPctR(sliderRR1.getValue()));
        egFilter.setSegmentLevel(5, toPctL(sliderRL1.getValue()));
        egFilter.setSegmentRate (5, toPctR(sliderRR2.getValue()));
        egFilter.setRelease(toPctR(sliderRR2.getValue()));
        egFilter.setReleaseLevel(toPctL(sliderRL2.getValue()));
        isUpdating = false;
    }

    void visibilityChanged() override
    {
        if (!isVisible()) return;
        juce::MessageManager::callAsync([this]()
        {
            if (!isVisible()) return;
            syncSegmentsFromSliders();
            egFilter.setBounds(0, 0,
                (int)(getWidth() * 0.65f),
                (int)(getHeight() * 0.80f));
            egFilter.resized();
            egFilter.repaint();
        });
    }

    void valueChanged(Value& v) override
    {
        if (isUpdating) return;
        syncSegmentsFromSliders();
        egFilter.repaint();
    }

    void setElementNumber ( int element, UndoManager& um) override
    {
        Logger::writeToLog( "Filter2 setElement");
        egFilter.clearSegments();
        int sysexdata2[9] = { 0x43, 0X10, 0x34, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00 };
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        
        if(element == 1)
        {
            
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1SLOPEFILTRE2, &um));
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2LEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2RL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2RL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2R1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2R2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2R3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2R4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2RR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT1EGFILTRE2RR2 , &um));
        }
        else if (element ==2)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2SLOPEFILTRE2, &um));
            sysexdata[4] = 0x20;
            sysexdata2[4] = 0x20;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2LEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2RL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2RL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2R1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2R2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2R3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2R4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2RR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT2EGFILTRE2RR2 , &um));
        }
        else if (element == 3)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3SLOPEFILTRE2, &um));
            sysexdata[4] = 0x40;
            sysexdata2[4] = 0x40;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2LEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2RL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2RL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2R1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2R2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2R3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2R4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2RR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT3EGFILTRE2RR2 , &um));
        }
        else if (element == 4)
        {
            sliderSlope.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4SLOPEFILTRE2, &um));
            sysexdata[4] = 0x60;
            sysexdata2[4] = 0x60;
            
            sliderL0.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL0, &um));
            sliderL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL1, &um));
            sliderL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL2, &um));
            sliderL3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL3, &um));
            sliderL4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2LEVEL4, &um));
            sliderRL1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2RL1 , &um));
            sliderRL2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2RL2 , &um));
            sliderR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2R1 , &um));
            sliderR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2R2 , &um));
            sliderR3.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2R3 , &um));
            sliderR4.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2R4 , &um));
            sliderRR1.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2RR1 , &um));
            sliderRR2.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::ELEMENT4EGFILTRE2RR2 , &um));
        }
        sysexdata[6] = 0x10;
        sliderSlope.setMidiSysex(sysexdata);
        
        sysexdata[4] +=0x01;  // filtre 2
        
        auto toPctL = [](double val) { return (int)((val + 64.0) * 100.0 / 128.0); };
        auto toPctR = [](double val) { return (int)((64.0 - val) * 100.0 / 64.0); };

        sysexdata[6] = 0x09;
        sysexdata2[6] = 0x03;
        egFilter.addSegment(toPctL(sliderL0.getValue()), toPctR(sliderR1.getValue()), "L0→L1", 63, sysexdata, 63, sysexdata2);
        sliderL0.setMidiSysex(sysexdata);
        sliderL0.setRangeAndRound(-64, 63, 0);
        sliderR1.setMidiSysex(sysexdata2);
        sliderR1.setRangeAndRound(0, 63, 0);

        sysexdata[6] = 0x0a;
        sysexdata2[6] = 0x04;
        egFilter.addSegment(toPctL(sliderL1.getValue()), toPctR(sliderR2.getValue()), "L1→L2", 63, sysexdata, 63, sysexdata2);
        sliderL1.setMidiSysex(sysexdata);
        sliderL1.setRangeAndRound(-64, 63, 0);
        sliderR2.setMidiSysex(sysexdata2);
        sliderR2.setRangeAndRound(0, 63, 0);

        sysexdata[6] = 0x0b;
        sysexdata2[6] = 0x05;
        egFilter.addSegment(toPctL(sliderL2.getValue()), toPctR(sliderR3.getValue()), "L2→L3", 63, sysexdata, 63, sysexdata2);
        sliderL2.setMidiSysex(sysexdata);
        sliderL2.setRangeAndRound(-64, 63, 0);
        sliderR3.setMidiSysex(sysexdata2);
        sliderR3.setRangeAndRound(0, 63, 0);

        sysexdata[6] = 0x0c;
        sysexdata2[6] = 0x06;
        egFilter.addSegment(toPctL(sliderL3.getValue()), toPctR(sliderR4.getValue()), "L3→L4", 63, sysexdata, 63, sysexdata2);
        sliderL3.setMidiSysex(sysexdata);
        sliderL3.setRangeAndRound(-64, 63, 0);
        sliderR4.setMidiSysex(sysexdata2);
        sliderR4.setRangeAndRound(0, 63, 0);

        sysexdata[6] = 0x0d;
        sysexdata2[6] = 0x07;
        egFilter.addSegment(toPctL(sliderL4.getValue()), toPctR(sliderRR1.getValue()), "L4→RL1", 63, sysexdata, 63, sysexdata2);
        sliderL4.setMidiSysex(sysexdata);
        sliderL4.setRangeAndRound(-64, 63, 0);
        sliderRR1.setMidiSysex(sysexdata2);
        sliderRR1.setRangeAndRound(0, 63, 0);

        sysexdata[6] = 0x0e;
        sysexdata2[6] = 0x0f;
        // Segment 5: start=RL1, rate=RR2, end=RL2
        egFilter.addSegment(toPctL(sliderRL1.getValue()), toPctR(sliderRR2.getValue()), "RL1→RL2", 63, sysexdata, 63, sysexdata2);
        sliderRL1.setMidiSysex(sysexdata);
        sliderRL1.setRangeAndRound(-64, 63, 0);
        sliderRL2.setMidiSysex(sysexdata2);
        sliderRL2.setRangeAndRound(-64, 63, 0);

        sysexdata2[6] = 0x08;
        sliderRR2.setMidiSysex(sysexdata2);
        sliderRR2.setRangeAndRound(0, 63, 0);
        egFilter.setRelease(toPctR(sliderRR2.getValue()));
        egFilter.repaint();

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
    void paint (Graphics& g) override
    {
        g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
        DBG("Filter2 bounds: " + String(getWidth()) + "x" + String(getHeight())
            + "  egFilter: " + egFilter.getBounds().toString());
    }
    void buttonClicked (Button* button) override
    {
        setVisible(false);
    }
    
    void resized() override
    {
        const int w = getWidth();
        const int h = getHeight();

        egFilter.setBounds(0, 0, (int)(w * 0.65f), (int)(h * 0.80f));
        egFilter.resized();
        egFilter.repaint();

        keyDraw.setBoundsRelative(0.0f, 0.85f, 1.0f, 0.09f);
        float sampleWidth = 0.2f + (float)(sliderSlope.getValue() / 100.0);
        samplePathLeft.setBoundsRelative(0.0f, 0.93f, sampleWidth, 0.09f);
        sampleWidth = 0.2f - (float)(sliderSlope.getValue() / 100.0);
        samplePathRight.setBoundsRelative(1.0f - sampleWidth, 0.93f, sampleWidth, 0.09f);

        sliderSlope.setBoundsRelative(0.4f, 0.92f, 0.2f, 0.1f);

        sliderL0.setBoundsRelative(0.65f, 0.06f, 0.04f, 0.3f);
        sliderL1.setBoundsRelative(0.70f, 0.06f, 0.04f, 0.3f);
        sliderL2.setBoundsRelative(0.75f, 0.06f, 0.04f, 0.3f);
        sliderL3.setBoundsRelative(0.80f, 0.06f, 0.04f, 0.3f);
        sliderL4.setBoundsRelative(0.85f, 0.06f, 0.04f, 0.3f);
        sliderRL1.setBoundsRelative(0.90f, 0.06f, 0.04f, 0.3f);
        sliderRL2.setBoundsRelative(0.95f, 0.06f, 0.04f, 0.3f);

        sliderR1.setBoundsRelative(0.70f, 0.40f, 0.04f, 0.3f);
        sliderR2.setBoundsRelative(0.75f, 0.40f, 0.04f, 0.3f);
        sliderR3.setBoundsRelative(0.80f, 0.40f, 0.04f, 0.3f);
        sliderR4.setBoundsRelative(0.85f, 0.40f, 0.04f, 0.3f);
        sliderRR1.setBoundsRelative(0.90f, 0.40f, 0.04f, 0.3f);
        sliderRR2.setBoundsRelative(0.95f, 0.40f, 0.04f, 0.3f);
    }
    
private:
    bool isUpdating = false;
    SADSR egFilter;
    
    Label   labelSlope {"test", "Slope"};
    MidiKeyDraw keyDraw;
    MidiSlider  sliderSlope;
    MidiPath    samplePathLeft;
    MidiPath    samplePathRight;
    
    Label labelL0 {"","L0"};
    Label labelL1 {"","L1"};
    Label labelL2 {"","L2"};
    Label labelL3 {"","L3"};
    Label labelL4 {"","L4"};
    Label labelRL1 {"","RL1"};
    Label labelRL2 {"","RL2"};
    Label labelR1 {"","R1"};
    Label labelR2 {"","R2"};
    Label labelR3 {"","R3"};
    Label labelR4 {"","R4"};
    Label labelRR1 {"","RR1"};
    Label labelRR2 {"","RR2"};
    
    MidiSlider sliderL0;
    MidiSlider sliderL1;
    MidiSlider sliderL2;
    MidiSlider sliderL3;
    MidiSlider  sliderL4;
    MidiSlider  sliderRL1;
    MidiSlider  sliderRL2;
    
    MidiSlider  sliderR0;
    MidiSlider  sliderR1;
    MidiSlider  sliderR2;
    MidiSlider  sliderR3;
    MidiSlider  sliderR4;
    MidiSlider  sliderRR1;
    MidiSlider  sliderRR2;
    
    UndoManager um;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Filter2)
};

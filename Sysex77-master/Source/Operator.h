/*
  ==============================================================================

    Operator.h
    Created: 12 Feb 2019 11:48:10pm
    Author:  Sébastien Portrait

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "AlgoDraw.h"

//==============================================================================
/*
 */
class Operator    : public ElementComponent, public TextButton::Listener
{
public:
    Operator()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        //   setBounds(getBoundsInParent());
        
        addAndMakeVisible(algoFm);
        addAndMakeVisible(sliderAlgo);
        sliderAlgo.setSliderStyle(Slider::LinearHorizontal);
//        sliderAlgo.setRange(1, 45);
        sliderAlgo.setRangeAndRound(1, 45, 1);
  //      sliderAlgo.setNumDecimalPlacesToDisplay(0);
        sliderAlgo.setPopupDisplayEnabled(true, true, this);
        sliderAlgo.setTextBoxStyle(Slider::NoTextBox, true, 10, 10);
        sliderAlgo.setColour(Slider::ColourIds::thumbColourId, Colours::darkorange);
        sliderAlgo.onValueChange = [this] {setAlgorythm();};
        addAndMakeVisible(labelAlgo);
        labelAlgo.attachToComponent(&sliderAlgo, false);

        setAlgorythm();

        // AFM Op EG sliders — block 0x56, b4 updated per element in setElementNumber.
        // b6 = 0x00 (R1), 0x01 (R2), 0x0D (HT) per 09_confirmed_addresses.md.
        int sysexAfm[9] = { 0x43, 0x10, 0x34, 0x56, 0x00, 0x00, 0x00, 0x00, 0 };
        sliderAfmR1.setMidiSysex(sysexAfm);
        sliderAfmR1.setRangeAndRound(0, 63, 0);
        addAndMakeVisible(sliderAfmR1);
        labelAfmR1.attachToComponent(&sliderAfmR1, false);

        sysexAfm[6] = 0x01;
        sliderAfmR2.setMidiSysex(sysexAfm);
        sliderAfmR2.setRangeAndRound(0, 63, 0);
        addAndMakeVisible(sliderAfmR2);
        labelAfmR2.attachToComponent(&sliderAfmR2, false);

        sysexAfm[6] = 0x0D;
        sliderAfmHT.setMidiSysex(sysexAfm);
        sliderAfmHT.setRangeAndRound(0, 63, 0);
        addAndMakeVisible(sliderAfmHT);
        labelAfmHT.attachToComponent(&sliderAfmHT, false);
    }
    
    ~Operator()
    {
        
    }
    void setElementNumber ( int element, UndoManager& um) override
    {
        
        int sysexdata[9] = { 0x43, 0X10, 0x34, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00 };
        
        if(element == 1)
        {
        sliderAlgo.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMALGOELEMENT1, &um));
        }
        if(element == 2)
        {
            
        sliderAlgo.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMALGOELEMENT2, &um));
        
        sysexdata[4] = 0x20;
        }
        if(element == 3)
        {
        sliderAlgo.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMALGOELEMENT3, &um));
        sysexdata[4] = 0x40;
        }
        if(element == 4)
        {
        sliderAlgo.getValueObject().referTo(valueTreeVoice.getPropertyAsValue(IDs::AFMALGOELEMENT4, &um));
        sysexdata[4] = 0x60;
        }
        sliderAlgo.setMidiSysex(sysexdata);

        // Update element offset (b4) for AFM Op EG sliders using the same offset.
        int sysexAfm[9] = { 0x43, 0x10, 0x34, 0x56, sysexdata[4], 0x00, 0x00, 0x00, 0 };
        sliderAfmR1.setMidiSysex(sysexAfm);
        sysexAfm[6] = 0x01;
        sliderAfmR2.setMidiSysex(sysexAfm);
        sysexAfm[6] = 0x0D;
        sliderAfmHT.setMidiSysex(sysexAfm);
    }
    
    void setAlgorythm()
    {
        algoFm.setAlgo(sliderAlgo.getValue());
        repaint();
    }
    void buttonClicked (Button* button) override
    {
        
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
        g.setColour (Colours::darkorange);
        
     //   g.drawText ("AFM operator a implementer", getLocalBounds(),
      //              Justification::centred, true);   // draw some placeholder text
    }
    
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        algoFm.setBoundsRelative(0.01f, 0.15f, 0.4f, 0.84f);
        sliderAlgo.setBoundsRelative(0.01f, 0.06f, 0.4f, 0.08f);

        // AFM Op EG R1/R2/HT — right column, unused space beside algo diagram
        sliderAfmR1.setBoundsRelative(0.45f, 0.20f, 0.06f, 0.55f);
        sliderAfmR2.setBoundsRelative(0.55f, 0.20f, 0.06f, 0.55f);
        sliderAfmHT.setBoundsRelative(0.65f, 0.20f, 0.06f, 0.55f);
        repaint();
    }
    
private:
    AlgoDraw algoFm;
    MidiSlider sliderAlgo;
    Label labelAlgo { "algo", TRANS("AFM Algorithm")};

    // AFM Op EG — block 0x56, element-aware (b4 set in setElementNumber)
    MidiSlider sliderAfmR1;
    MidiSlider sliderAfmR2;
    MidiSlider sliderAfmHT;
    Label labelAfmR1 {"", "AfmR1"};
    Label labelAfmR2 {"", "AfmR2"};
    Label labelAfmHT {"", "AfmHT"};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Operator)
};

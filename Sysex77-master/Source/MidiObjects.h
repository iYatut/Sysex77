/*
  ==============================================================================

    MidiObjects.h
    Created: 1 Dec 2018 11:05:41am
    Author:  Sébastien Portrait
 
 Librairie d'objets MIDI / OSC

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
class ElementComponent    : public Component
{
public:
    ElementComponent()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
    }
    
    ~ElementComponent()
    {
        
    }
   virtual void setElementNumber ( int element, UndoManager& um)
    {
        Logger::writeToLog( "ElementComponent setElement");
    }
    void paint (Graphics& g) override
    {
        /* This demo code just fills the component's background and
         draws some placeholder text to get you started.
         
         You should replace everything in this method with your own
         drawing code..
         */
        
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
        
    }

    
    void resized() override
    {
        
    }
    
private:

    
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ElementComponent)
};


//==============================================================================
/*
 Midi slider herite from slider and had
 OSC Sender
 OSC receiver
 And a midi sysex by OSC
 And a pan mode by a negative min range
 
 int sysexdata[9] = { 0x43, 0X10, 0x34, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 };
 sliderTest.setMidiSysex(sysexdata);
 sliderTest.setRangeAndRound(0, 127,100);
 
 addAndMakeVisible(sliderTest);
 
*/


class MidiSlider   : public Slider , public Slider::Listener, public Value::Listener
{
    
public:
    MidiSlider()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        
        setColour(Slider::ColourIds::thumbColourId, Colours::darkorange);
        setPopupDisplayEnabled(true, true, this);
        setSliderStyle(SliderStyle::LinearVertical);
        setTextBoxStyle(TextEntryBoxPosition::NoTextBox, false, getWidth(), 20);
        Slider::addListener(this);
        //connect by default
        valueSysexIn.addListener(this); //Listen to the sysex in
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        

    }
    
    ~MidiSlider()
    {
        Slider::removeListener(this);
        setLookAndFeel(nullptr);
        valueSysexIn.removeListener(this);
        
    }

// setRange

    void valueChanged(Value & value) override
    {
    
        Logger::writeToLog("midislider value change");
        int val = value.getValue()[0];
        if(sysexData[3] == val)
        {
            val = value.getValue()[1];
            if(sysexData[4] ==  val)
            {
                val = value.getValue()[2];
                if(sysexData[5] ==  val)
                {
                    val = value.getValue()[3];
                    if(sysexData[6] ==  val)
                    {
                        val = value.getValue()[5];
                       // if(sysexData[7] ==  val)
                        
                Logger::writeToLog ("Slider rexeive val " + String(val));
                        if (encodeMixerEffectSendSigned7)
                        {
                            Slider::setValue (uiFromMixerEffectSendSigned7Sysex (val), dontSendNotification);
                        }
                        else if (encodeElementNoteShiftSigned)
                        {
                            Slider::setValue (uiFromElementNoteShiftSysex (val), dontSendNotification);
                        }
                        else if (encodeYamahaSymmetric12ATPBR && boolNegative)
                        {
                            int setVal;
                            if (val == 0)
                                setVal = 0;
                            else if (val >= 1 && val <= 0x0C)
                                setVal = val;
                            else if (val >= 0x11 && val <= 0x1C)
                                setVal = 0x10 - val;
                            else
                                setVal = val;

                            Slider::setValue (setVal, dontSendNotification);
                        }
                        else
                        {
                        if(boolNegative)
                        {
                            if(val > (intNegativeDelta -2))
                            {
                             val -= intNegativeDelta;
                            if(!boolInvert)
                            val = ~val;
                                
                            }
                Logger::writeToLog ("signed minus  " + String(val));
                        }
                                Slider::setValue (val, dontSendNotification);
                        }
                                
                            }
                        }
                    }
        }
        
        
       
   
    }
  
    void setBoolInvert(bool boolSigned)
    {
        boolInvert = boolSigned;
    }

    /** When set (ATPBR −12…+12 only): outbound/inbound SysEx data byte follows Yamaha Voice Common
        ladder 0x1C…0x11, 0x00, 0x01…0x0C instead of MidiSlider bitwise legacy (~v + max+2). */
    void setVoiceCommonSymmetric12SysexEncode (bool on) noexcept
    {
        encodeYamahaSymmetric12ATPBR = on;
    }

    /** Mixer Effect Send Vel Sens / Scaling (03 NN 0B/0C): UI −7…+7 ↔ SY99 VV ladder. */
    void setMixerEffectSendSigned7SysexEncode (bool on) noexcept
    {
        encodeMixerEffectSendSigned7 = on;
    }

    /** Element Note Shift (03 EE 00 02): UI −64…+63 ↔ VV 0…127 via VV = UI + 64. */
    void setElementNoteShiftSignedSysexEncode (bool on) noexcept
    {
        encodeElementNoteShiftSigned = on;
    }

    /** MIDI note number 0…127: text box / popup show note names (e.g. C3), not raw integers. */
    void setMidiNoteNumberTextDisplay()
    {
        setNumDecimalPlacesToDisplay (0);
        textFromValueFunction = [] (double value)
        {
            const int note = (int) jlimit (0.0, 127.0, value);
            return MidiMessage::getMidiNoteName (note, true, true, 3);
        };
        valueFromTextFunction = [] (const String& text)
        {
            const String t = text.trim();
            for (int n = 0; n <= 127; ++n)
                if (MidiMessage::getMidiNoteName (n, true, true, 3).equalsIgnoreCase (t))
                    return (double) n;
            return 0.0;
        };
    }
    
    void setRangeAndRound (int min, int max, int defValue)
    {
        if(min<0)
        {
            boolNegative = true;
            intNegativeDelta = max +2;  //correction for sysex
            setLookAndFeel(&lookPan);  // Look and feel for pan
            setSliderStyle(SliderStyle::Rotary);
        }
        else
        {
            setLookAndFeel(&lookNormal);
        }
        setRange(min, max, 1);
        setNumDecimalPlacesToDisplay(0);
        setDoubleClickReturnValue(true, defValue);
    }
    // Initialise le sender OSC
    bool initOscSender(String adress, int port)
    {
        sender.disconnect();
        return (! sender.connect (adress, port)); // [4]
   
    }
    
// Send values
    void sliderValueChanged (Slider* slider) override
    {
     
        int value = (int) slider->getValue();
        Logger::writeToLog ("Slider Value changed " + String(value));

        if (encodeMixerEffectSendSigned7)
        {
            value = sysexFromMixerEffectSendSigned7Ui (value);
        }
        else if (encodeElementNoteShiftSigned)
        {
            value = sysexFromElementNoteShiftUi (value);
        }
        else if (encodeYamahaSymmetric12ATPBR)
        {
            if (value < 0)
                value = 0x1C + ((-12) - value);  // −12→0x1C … −1→0x11 ; 0 unchanged
        }
        else if (value < 0)
        {
            if (! boolInvert)
                value = ~value;

            value += intNegativeDelta;
            Logger::writeToLog ("Signed minus " + String(value));
        }
        
       // uint8 data = slider->getValue();
        sysexData[8] = (uint8) value;
        
        sender.send(oscAddressPatern, (uint8) sysexData[0], sysexData[1], sysexData[2], sysexData[3], sysexData[4], sysexData[5], sysexData[6], sysexData[7], sysexData[8]);
        
    }
 
    
    void setMidiSysex (int sysexdata[0])
    {
        for(int i = 0; i < 9 ;i++)
            sysexData[i] = sysexdata[i];
    }
    void setOscAdress ( String oscAdress)
    {
        strOscAdress = oscAdress;
    }

    void sendSysexPacket (const uint8 data[9])
    {
        sender.send (oscAddressPatern, (uint8) data[0], data[1], data[2], data[3],
                     data[4], data[5], data[6], data[7], data[8]);
    }

private:
    bool boolNegative = false;   // set if neg value like pan
    bool boolInvert = false;
    /** Voice Common ATPBR symmetric −12…+12 (addr 02 00 00 29) Yamaha byte ladder. Off for all other sliders (Pan/Pitch/filter etc.). */
    bool encodeYamahaSymmetric12ATPBR { false };
    bool encodeMixerEffectSendSigned7 { false };
    bool encodeElementNoteShiftSigned { false };
    int intNegativeDelta; // correction for sysex

    static int uiFromElementNoteShiftSysex (int sysexVal) noexcept
    {
        return jlimit (-64, 63, jlimit (0, 127, sysexVal) - 64);
    }

    static int sysexFromElementNoteShiftUi (int uiVal) noexcept
    {
        return jlimit (0, 127, jlimit (-64, 63, uiVal) + 64);
    }

    static int uiFromMixerEffectSendSigned7Sysex (int sysexVal) noexcept
    {
        switch ((uint8) jlimit (0, 127, sysexVal))
        {
            case 0x00: return 0;
            case 0x01: return 1;
            case 0x02: return 2;
            case 0x03: return 3;
            case 0x04: return 4;
            case 0x05: return 5;
            case 0x06: return 6;
            case 0x07: return 7;
            case 0x09: return -1;
            case 0x0A: return -2;
            case 0x0B: return -3;
            case 0x0C: return -4;
            case 0x0D: return -5;
            case 0x0E: return -6;
            case 0x0F: return -7;
            case 0x08: return 0;  // gap in SY99 ladder; treat as centre
            default:   return 0;  // unknown VV: never expose raw byte as UI
        }
    }

    static int sysexFromMixerEffectSendSigned7Ui (int uiVal) noexcept
    {
        switch (jlimit (-7, 7, uiVal))
        {
            case -7: return 0x0F;
            case -6: return 0x0E;
            case -5: return 0x0D;
            case -4: return 0x0C;
            case -3: return 0x0B;
            case -2: return 0x0A;
            case -1: return 0x09;
            case  0: return 0x00;
            case  1: return 0x01;
            case  2: return 0x02;
            case  3: return 0x03;
            case  4: return 0x04;
            case  5: return 0x05;
            case  6: return 0x06;
            case  7: return 0x07;
            default: return 0x00;
        }
    }
    String oscAddressPatern {"/SYSEX"};
    String strOscAdress;
//    MidiMessage midiMessage;
    CustomLookAndFeel lookPan;
    CustomLookAndFeelV2 lookNormal;

    OSCSender sender;
    uint8 sysexData[9];
    uint8 inputData[9];
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSlider)
};


//==============================================================================
/*
 Midi Button herite from TextButton and had
 OSC Sender
 OSC receiver
 And a midi sysex by OSC
 mode on off
 
 int sysexdata[9] = { 0x43, 0X10, 0x34, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 };
 sliderTest.setMidiSysex(sysexdata);
 sliderTest.setRangeAndRound(0, 127,100);
 
 addAndMakeVisible(sliderTest);
 
 */

class MidiButton : public TextButton , public TextButton::Listener, public Value::Listener
{
public:
    MidiButton()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        
        setColour(TextButton::ColourIds::buttonOnColourId, Colours::darkorange);
        setClickingTogglesState(true);
        TextButton::addListener(this);
        valueSysexIn.addListener(this); //Listen to the sysex in
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        
    }
    
    ~MidiButton()
    {
        valueSysexIn.removeListener(this); //Listen to the sysex in
        TextButton::removeListener(this);
       // setLookAndFeel(nullptr);
    }

 
    // Initialise le sender OSC
    bool initOscSender(String adress, int port)
    {
        sender.disconnect();
        return (! sender.connect (adress, port)); // [4]
        
    }
    void setTextOnOff(String stringOn, String stringOff)
    {
        setButtonText(stringOff);
        strOn = stringOn;
        strOff = stringOff;
    }
    // clic override
    void valueChanged(Value & value) override
    {
        
        Logger::writeToLog("midislider value change");
        int val = value.getValue()[0];
        if(sysexData[3] == val)
        {
            val = value.getValue()[1];
            if(sysexData[4] ==  val)
            {
                val = value.getValue()[2];
                if(sysexData[5] ==  val)
                {
                    val = value.getValue()[3];
                    if(sysexData[6] ==  val)
                    {
                        // val = value.getValue()[4];
                        // if(sysexData[7] ==  val)
                        setToggleState(value.getValue()[5], dontSendNotification);
                        if(getToggleState())
                        {
                            setButtonText(strOn);
                        }
                        else
                        {
                            setButtonText(strOff);
                        }
                    }
                }
            }
        }
        

        
        
    }

    void buttonClicked (Button* button) override
    {

        sysexData[8] = button->getToggleState();
        
        sender.send(oscAddressPatern, (uint8) sysexData[0], sysexData[1], sysexData[2], sysexData[3], sysexData[4], sysexData[5], sysexData[6], sysexData[7], sysexData[8]);
            if(getToggleState())
            {
                setButtonText(strOn);
            }
        else
        {
            setButtonText(strOff);
        }
    }
    
    void setMidiSysex (int sysexdata[0])
    {
        for(int i = 0; i < 9 ;i++)
            sysexData[i] = sysexdata[i];
    }
    void setOscAdress ( String oscAdress)
    {
        strOscAdress = oscAdress;
    }
    
private:
//    bool boolNegative = false;   // set if neg value like pan

    String oscAddressPatern {"/SYSEX"};
    String strOscAdress;
 //   MidiMessage midiMessage;
    OSCSender sender;
    uint8 sysexData[9];
    uint8 inputData[9];
    String  strOn;
    String  strOff;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiButton)
};
//==============================================================================
class MidiRadio : public Component, private TextButton::Listener, private Value::Listener
{
public:
    MidiRadio()
    {
        valueRadio.addListener(this);
        valueRadio.setValue(0);
    }
    ~MidiRadio()
    {
        valueRadio.removeListener(this);
    }
    void valueChanged(Value & value) override
    {
        int val = value.getValue();
        if(!arrayButton[val]->getToggleState())
            arrayButton[val]->setToggleState(true, NotificationType::dontSendNotification);
               Logger::writeToLog("MidiRadio Value change"); 
    }
void buttonClicked (Button* button) override
    {
        Logger::writeToLog("MidiRadio Button clic");
        for(auto i =0; i < arrayButton.size();i++)
            if(arrayButton[i]->getToggleState())
                valueRadio.setValue(i);
    }
void addRadio(String text, int radioId)
    {
        auto bt = new TextButton;
        bt->setButtonText(text);
        bt->setColour(TextButton::ColourIds::buttonOnColourId, Colours::darkorange);
        bt->setClickingTogglesState(radioId);
        bt->addListener(this);
        bt->setRadioGroupId(radioId);
        addAndMakeVisible(bt);
//           arrayButton[arrayButton.size()-1]->setConnectedEdges(Button::ConnectedOnRight);
        Logger::writeToLog("size: " + String(arrayButton.size()));
        if(arrayButton.size()==1)
        {
            arrayButton[arrayButton.size()-1]->setConnectedEdges(Button::ConnectedOnRight);
            
        }
        if(arrayButton.size()>1)
        {
            arrayButton[arrayButton.size()-1]->setConnectedEdges(Button::ConnectedOnRight|Button::ConnectedOnLeft);
            bt->setConnectedEdges(Button::ConnectedOnLeft);
        }
        
        arrayButton.add(bt);
 //       bt->setConnectedEdges(Button::ConnectedOnLeft);
 //       if(arrayButton.size()>2)

        resized();
    }
int getValue()
    {
        return int(valueRadio.getValue());
    }
void setValue(int value)
    {
        valueRadio.setValue(value);
    }
    
Value& getValueObject()
    {
        return valueRadio;
    }
void resized() override
    {
        for(auto i = 0; i<arrayButton.size();i++)
        {
            arrayButton[i]->setBoundsRelative(1.0f/arrayButton.size()*i, 0.0f, 1.0f/arrayButton.size(), 1.0f);
            
        }
    }
private:
    OwnedArray<TextButton> arrayButton;
    Value   valueRadio;
};

//==============================================================================
/*
 Midi Combo herite from comboBox and had
 OSC Sender
 And a midi sysex by OSC
 Syxex in by value listening
 
 
 */

class MidiCombo : public ComboBox , public ComboBox::Listener

{
public:
    MidiCombo()
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.
        
        ComboBox::addListener(this);
        valueSysexIn.addListener(this); //Listen to the sysex in
        if (! sender.connect ("127.0.0.1", 9001)) // [4]
            Logger::writeToLog ("Error: could not connect to UDP port 9001.");
        
    }
    
    ~MidiCombo()
    {
        valueSysexIn.removeListener(this); //Listen to the sysex in
        ComboBox::removeListener(this);
        // setLookAndFeel(nullptr);
    }
    
    
    // Initialise le sender OSC
    bool initOscSender(String adress, int port)
    {
        sender.disconnect();
        return (! sender.connect (adress, port)); // [4]
        
    }
    
   void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override
    {
             Logger::writeToLog("midiCombo  changed");
        sysexData[8] = getSelectedId() -1;
        
        sender.send(oscAddressPatern, (uint8) sysexData[0], sysexData[1], sysexData[2], sysexData[3], sysexData[4], sysexData[5], sysexData[6], sysexData[7], sysexData[8]);
        
    }
    void valueChanged(Value & value) override
    {
        
        Logger::writeToLog("midislider value change");
        if(value.refersToSameSourceAs(valueSysexIn))
        {
        int val = value.getValue()[0];
        if(sysexData[3] == val)
        {
            val = value.getValue()[1];
            if(sysexData[4] ==  val)
            {
                val = value.getValue()[2];
                if(sysexData[5] ==  val)
                {
                    val = value.getValue()[3];
                    if(sysexData[6] ==  val)
                    {
                        val =value.getValue()[5];
                        setSelectedId (val + 1, dontSendNotification);
                    }
                }
            }
        }}}

    
    void setMidiSysex (int sysexdata[0])
    {
        for(int i = 0; i < 9 ;i++)
            sysexData[i] = sysexdata[i];
    }
    void setOscAdress ( String oscAdress)
    {
        strOscAdress = oscAdress;
    }
    
private:
    //    bool boolNegative = false;   // set if neg value like pan
    
    String oscAddressPatern {"/SYSEX"};
    String strOscAdress;

    OSCSender sender;
    uint8 sysexData[9];
    uint8 inputData[9];
    String  strOn;
    String  strOff;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiCombo)
};


//==============================================================================
class MidiKeyDraw   : public Component
{
public:
    //==============================================================================
    MidiKeyDraw()
    {
  
        
    }
    
    ~MidiKeyDraw()
    {
        
    }
    
    //==============================================================================
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        
        /*
         Draw a keyBoard C-2 to G8
         
         */
        g.setColour(Colours::white);
        
        const int intOctave = 10;
        
        float fgrid = (getWidth()*100) /((intOctave *7)+5);
        fgrid /= 100;
        float fHeight = getHeight() * 0.8;
        float fY = getHeight() - fHeight;
        
        g.fillRect(0.0f,fY, float(getWidth()), fHeight);
        
        for(int i = 0 ; i < intOctave +1;i++)
        {
            g.setFont(fHeight*0.3f);
            g.drawFittedText("C" + String(i-2), (i * 7) *  fgrid,0, fgrid * 5, getHeight() -fHeight, Justification::centredLeft ,1);
            
        }
        
        g.setColour(Colours::black);
        
        for(int i =0; i < (7*intOctave)+5 ; i++)
        {
            if(i %7)
            {
                
            }
            else
            {
                g.setColour(Colours::darkorange);
                g.fillRect(i * fgrid, fY , fgrid, fHeight);
                g.setColour(Colours::black);
                
            }
            g.drawRect(i * fgrid, fY , fgrid, fHeight);
        }
        float a =0.0f;
        
        fHeight *= 0.7; //black key height rate
        
        for(int i =0; i < intOctave +1 ; i++)
        {
            a = i *7.0f * fgrid;
            g.fillRect(a  + (fgrid / 2.0f), fY, fgrid * 0.9f, fHeight);
            g.fillRect(a + fgrid + (fgrid / 2.0f),fY, fgrid * 0.9f, fHeight);
            g.fillRect(a + ((fgrid * 3.0f)) + (fgrid / 2.0f),fY, fgrid * 0.9f,fHeight);
            g.fillRect(a + (fgrid * 4.0f )+ (fgrid / 2.0f),fY, fgrid * 0.9f,  fHeight);
            g.fillRect(a + (fgrid * 5.0f )+ (fgrid / 2.0f),fY, fgrid * 0.9f,  fHeight);
        }
        
       
    }
    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
   
    }
    
private:
    //==============================================================================
    // Your private member variables go here...

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiKeyDraw)
};
//==============================================================================
class MidiPath   : public Component
{
public:
    //==============================================================================
    MidiPath()
    {
        
        
    }
    
    ~MidiPath()
    {
        
    }
    
    //==============================================================================
    void paint (Graphics& g) override
    {
        // Draw a sample path
        g.setColour (Colours::darkorange);
        Path myPath;    // create the path
        
        myPath.startNewSubPath (0, getHeight()+2);
        myPath.lineTo(getWidth() * 0.2, getHeight() * 0.1);
        myPath.lineTo(getWidth() * 0.3, getHeight() * 0.3);
        myPath.lineTo(getWidth() * 0.5, getHeight() * 0.2);
        myPath.lineTo(getWidth() * 0.9, getHeight()+2);
        myPath.closeSubPath();
        g.strokePath (myPath, PathStrokeType (3.0f));
        
        
    }
    
    
private:
    //==============================================================================
    // Your private member variables go here...
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiPath)
};


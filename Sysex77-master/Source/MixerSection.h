/*
  ==============================================================================

    MixerSection.h
    Placeholder layout for per-element mixer strips (Group 0x03) on Common tab.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "MidiObjects.h"
#include "FilterControls.h"
#include "VoicesTableModel.h"

//==============================================================================
/** One underlying OUTSEL byte (Group 0x03 NN 0x08) bound to two toggle buttons. */
class MixerOutputGroupBinding : private Button::Listener,
                                private Value::Listener
{
public:
    MixerOutputGroupBinding() = default;

    ~MixerOutputGroupBinding() override
    {
        detach();
    }

    void attachForElement (uint8 elementOffset, TextButton& group1, TextButton& group2,
                           MidiSlider& outselSlider)
    {
        detach();

        btGroup1 = &group1;
        btGroup2 = &group2;
        sendViaSlider = &outselSlider;

        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x08, 0x00, 0x00 };
        sendViaSlider->setMidiSysex (sysex);

        sendViaSlider->getValueObject().addListener (this);

        btGroup1->addListener (this);
        btGroup2->addListener (this);

        applyRawByteToButtons ((uint8) (int) sendViaSlider->getValue());
    }

    void detach()
    {
        if (sendViaSlider != nullptr)
            sendViaSlider->getValueObject().removeListener (this);
        if (btGroup1 != nullptr)
            btGroup1->removeListener (this);
        if (btGroup2 != nullptr)
            btGroup2->removeListener (this);
        btGroup1 = nullptr;
        btGroup2 = nullptr;
        sendViaSlider = nullptr;
    }

private:
    static constexpr uint8 out1Bit = 0x02;
    static constexpr uint8 out2Bit = 0x04;
    static constexpr uint8 outselMask = out1Bit | out2Bit;

    void applyRawByteToButtons (uint8 raw)
    {
        if (btGroup1 != nullptr)
            btGroup1->setToggleState ((raw & out1Bit) != 0, dontSendNotification);
        if (btGroup2 != nullptr)
            btGroup2->setToggleState ((raw & out2Bit) != 0, dontSendNotification);
    }

    uint8 rawByteFromButtons() const
    {
        const uint8 current = sendViaSlider != nullptr ? (uint8) (int) sendViaSlider->getValue()
                                                       : (uint8) 0;
        uint8 raw = (uint8) (current & ~outselMask);
        if (btGroup1 != nullptr && btGroup1->getToggleState())
            raw = (uint8) (raw | out1Bit);
        if (btGroup2 != nullptr && btGroup2->getToggleState())
            raw = (uint8) (raw | out2Bit);
        return raw;
    }

    void buttonClicked (Button*) override
    {
        if (sendViaSlider == nullptr)
            return;

        sendViaSlider->setValue (rawByteFromButtons(), sendNotification);
        markEditorPatchDirty();
    }

    void valueChanged (Value& value) override
    {
        if (sendViaSlider == nullptr
            || ! value.refersToSameSourceAs (sendViaSlider->getValueObject()))
            return;

        applyRawByteToButtons ((uint8) (int) value.getValue());
    }

    TextButton* btGroup1 = nullptr;
    TextButton* btGroup2 = nullptr;
    MidiSlider* sendViaSlider = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerOutputGroupBinding)
};

//==============================================================================
/** One underlying Effect Send Select byte (Group 0x03 NN 0x09) bound to Send 1 / Send 3 toggles. */
class MixerEffectSendSelectBinding : private Button::Listener,
                                     private Value::Listener
{
public:
    MixerEffectSendSelectBinding() = default;

    ~MixerEffectSendSelectBinding() override
    {
        detach();
    }

    void attachForElement (uint8 elementOffset, TextButton& send1, TextButton& send3,
                           MidiSlider& sendVia, MidiSlider* effectModeVia = nullptr)
    {
        detach();

        btSend1 = &send1;
        btSend3 = &send3;
        sendViaSlider = &sendVia;
        efmodeViaSlider = effectModeVia;

        int sysex[9] = { 0x43, sysexEngine, 0x34, 0x03, elementOffset, 0x00, 0x09, 0x00, 0x00 };
        sendViaSlider->setMidiSysex (sysex);

        sendViaSlider->getValueObject().addListener (this);

        if (efmodeViaSlider != nullptr)
            efmodeViaSlider->getValueObject().addListener (this);

        btSend1->addListener (this);
        btSend3->addListener (this);

        applyRawByteToButtons ((uint8) (int) sendViaSlider->getValue());

        if (efmodeViaSlider != nullptr)
            applyEffectSendSelectVisibilityFromEffectMode (btSend1, btSend3,
                                                           (uint8) (int) efmodeViaSlider->getValue());
    }

    void detach()
    {
        if (sendViaSlider != nullptr)
            sendViaSlider->getValueObject().removeListener (this);
        if (efmodeViaSlider != nullptr)
            efmodeViaSlider->getValueObject().removeListener (this);
        if (btSend1 != nullptr)
            btSend1->removeListener (this);
        if (btSend3 != nullptr)
            btSend3->removeListener (this);
        btSend1 = nullptr;
        btSend3 = nullptr;
        sendViaSlider = nullptr;
        efmodeViaSlider = nullptr;
    }

private:
    static constexpr uint8 send1Bit = 0x01;
    static constexpr uint8 send3Bit = 0x04;
    static constexpr uint8 sendSelMask = send1Bit | send3Bit;

    static void applyEffectSendSelectVisibilityFromEffectMode (TextButton* send1,
                                                               TextButton* send3,
                                                               uint8 efmodeRaw) noexcept
    {
        // EFMODE @ 08 00 00 20: 0x00 off → hide both; 0x01 serial → Send 1 only; 0x02 parallel → both.
        const bool showSend1 = (efmodeRaw == 0x01 || efmodeRaw == 0x02);
        const bool showSend3 = (efmodeRaw == 0x02);

        if (send1 != nullptr)
            send1->setVisible (showSend1);
        if (send3 != nullptr)
            send3->setVisible (showSend3);
    }

    void applyRawByteToButtons (uint8 raw)
    {
        if (btSend1 != nullptr)
            btSend1->setToggleState ((raw & send1Bit) != 0, dontSendNotification);
        if (btSend3 != nullptr)
            btSend3->setToggleState ((raw & send3Bit) != 0, dontSendNotification);
    }

    uint8 rawByteFromButtons() const
    {
        const uint8 current = sendViaSlider != nullptr ? (uint8) (int) sendViaSlider->getValue()
                                                       : (uint8) 0;
        uint8 raw = (uint8) (current & ~sendSelMask);
        if (btSend1 != nullptr && btSend1->getToggleState())
            raw = (uint8) (raw | send1Bit);
        if (btSend3 != nullptr && btSend3->getToggleState())
            raw = (uint8) (raw | send3Bit);
        return raw;
    }

    void buttonClicked (Button*) override
    {
        if (sendViaSlider == nullptr)
            return;

        sendViaSlider->setValue (rawByteFromButtons(), sendNotification);
    }

    void valueChanged (Value& value) override
    {
        if (efmodeViaSlider != nullptr
            && value.refersToSameSourceAs (efmodeViaSlider->getValueObject()))
        {
            applyEffectSendSelectVisibilityFromEffectMode (btSend1, btSend3,
                                                           (uint8) (int) value.getValue());
            return;
        }

        if (sendViaSlider == nullptr
            || ! value.refersToSameSourceAs (sendViaSlider->getValueObject()))
            return;

        applyRawByteToButtons ((uint8) (int) value.getValue());
    }

    TextButton* btSend1 = nullptr;
    TextButton* btSend3 = nullptr;
    MidiSlider* sendViaSlider = nullptr;
    MidiSlider* efmodeViaSlider = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerEffectSendSelectBinding)
};

//==============================================================================
/** One vertical strip: El.N header + v1 parameter rows + reserved effect rows. */
class MixerElementStrip : public Component
{
public:
    static constexpr int kEffectSendBlockH = 18 + 2 + 10 + 22;

    explicit MixerElementStrip (const String& elementBaseName)
        : baseName (elementBaseName)
    {
        header.setText (elementBaseName, dontSendNotification);
        header.setJustificationType (Justification::centred);
        header.setColour (Label::textColourId, Colours::darkorange);
        addAndMakeVisible (header);

        setupPlaceholderSlider (level);
        addAndMakeVisible (level);

        for (auto* s : { &detune, &noteShift, &dynamicPan,
                         &noteLimitLow, &noteLimitHigh, &velocityLimitLow, &velocityLimitHigh })
        {
            setupPlaceholderSlider (*s);
            addAndMakeVisible (*s);
        }

        setupOutputGroupButton (btOutputGroup1, "Group 1");
        setupOutputGroupButton (btOutputGroup2, "Group 2");
        addAndMakeVisible (btOutputGroup1);
        addAndMakeVisible (btOutputGroup2);

        setupOutputGroupButton (btEffectSendLine1, "Send 1");
        setupOutputGroupButton (btEffectSendLine3, "Send 3");
        addAndMakeVisible (btEffectSendLine1);
        addChildComponent (btEffectSendLine2);
        addAndMakeVisible (btEffectSendLine3);
        addChildComponent (btEffectSendLine4);
        btEffectSendLine2.setVisible (false);
        btEffectSendLine4.setVisible (false);

        for (auto* lab : { &lblEffectSendLvl, &lblEffectSendVel, &lblEffectSendScl })
        {
            lab->setJustificationType (Justification::centred);
            lab->setColour (Label::textColourId, Colours::darkorange);
            addAndMakeVisible (*lab);
        }
        lblEffectSendLvl.setText ("Lvl", dontSendNotification);
        lblEffectSendVel.setText ("Vel", dontSendNotification);
        lblEffectSendScl.setText ("Scl", dontSendNotification);

        setupEffectSendKnob (effectSendLevelPh);
        setupEffectSendKnob (effectSendVelSensPh);
        setupEffectSendKnob (effectSendScalingPh);
        addAndMakeVisible (effectSendLevelPh);
        addAndMakeVisible (effectSendVelSensPh);
        addAndMakeVisible (effectSendScalingPh);
    }

    void attachLiveLevelControl (MidiSlider& liveLevel)
    {
        level.setVisible (false);
        liveLevelControl = &liveLevel;
        addAndMakeVisible (liveLevel);
    }

    void attachLiveNoteLimitLowControl (MidiSlider& liveNoteLimitLow)
    {
        noteLimitLow.setVisible (false);
        liveNoteLimitLowControl = &liveNoteLimitLow;

        if (auto* oldParent = liveNoteLimitLow.getParentComponent())
            oldParent->removeChildComponent (&liveNoteLimitLow);

        addAndMakeVisible (liveNoteLimitLow);
        styleMixerStripSlider (liveNoteLimitLow);
        liveNoteLimitLow.setBounds (0, 0, getWidth(), 18);
        resized();
    }

    void attachLiveNoteLimitHighControl (MidiSlider& liveNoteLimitHigh)
    {
        noteLimitHigh.setVisible (false);
        liveNoteLimitHighControl = &liveNoteLimitHigh;

        if (auto* oldParent = liveNoteLimitHigh.getParentComponent())
            oldParent->removeChildComponent (&liveNoteLimitHigh);

        addAndMakeVisible (liveNoteLimitHigh);
        styleMixerStripSlider (liveNoteLimitHigh);
        liveNoteLimitHigh.setBounds (0, 0, getWidth(), 18);
        resized();
    }

    void attachLiveVelocityLimitLowControl (MidiSlider& liveVelocityLimitLow)
    {
        velocityLimitLow.setVisible (false);
        liveVelocityLimitLowControl = &liveVelocityLimitLow;

        if (auto* oldParent = liveVelocityLimitLow.getParentComponent())
            oldParent->removeChildComponent (&liveVelocityLimitLow);

        addAndMakeVisible (liveVelocityLimitLow);
        styleMixerStripSlider (liveVelocityLimitLow);
        liveVelocityLimitLow.setBounds (0, 0, getWidth(), 18);
        resized();
    }

    void attachLiveVelocityLimitHighControl (MidiSlider& liveVelocityLimitHigh)
    {
        velocityLimitHigh.setVisible (false);
        liveVelocityLimitHighControl = &liveVelocityLimitHigh;

        if (auto* oldParent = liveVelocityLimitHigh.getParentComponent())
            oldParent->removeChildComponent (&liveVelocityLimitHigh);

        addAndMakeVisible (liveVelocityLimitHigh);
        styleMixerStripSlider (liveVelocityLimitHigh);
        liveVelocityLimitHigh.setBounds (0, 0, getWidth(), 18);
        resized();
    }

    void attachLiveDetuneControl (MidiSlider& liveDetune)
    {
        detune.setVisible (false);
        liveDetuneControl = &liveDetune;

        if (auto* oldParent = liveDetune.getParentComponent())
            oldParent->removeChildComponent (&liveDetune);

        addAndMakeVisible (liveDetune);
        styleMixerStripSlider (liveDetune);
        liveDetune.setBounds (0, 0, getWidth(), 18);
        resized();
    }

    void detachLiveDetuneControl()
    {
        if (liveDetuneControl != nullptr)
        {
            removeChildComponent (liveDetuneControl);
            liveDetuneControl = nullptr;
            detune.setVisible (true);
        }
    }

    void attachLiveNoteShiftControl (MidiSlider& liveNoteShift)
    {
        noteShift.setVisible (false);
        liveNoteShiftControl = &liveNoteShift;

        if (auto* oldParent = liveNoteShift.getParentComponent())
            oldParent->removeChildComponent (&liveNoteShift);

        addAndMakeVisible (liveNoteShift);
        styleMixerStripSlider (liveNoteShift);
        liveNoteShift.setBounds (0, 0, getWidth(), 18);
        resized();
    }

    void detachLiveNoteShiftControl()
    {
        if (liveNoteShiftControl != nullptr)
        {
            removeChildComponent (liveNoteShiftControl);
            liveNoteShiftControl = nullptr;
            noteShift.setVisible (true);
        }
    }

    void attachLiveEffectSendControls (MidiSlider& liveLevel,
                                       MidiSlider& liveVelSens,
                                       MidiSlider& liveScaling)
    {
        effectSendLevelPh.setVisible (false);
        effectSendVelSensPh.setVisible (false);
        effectSendScalingPh.setVisible (false);
        liveEffectSendLevelControl = &liveLevel;
        liveEffectSendVelSensControl = &liveVelSens;
        liveEffectSendScalingControl = &liveScaling;
        addAndMakeVisible (liveLevel);
        addAndMakeVisible (liveVelSens);
        addAndMakeVisible (liveScaling);
        setupEffectSendKnob (liveLevel);
        setupEffectSendKnob (liveVelSens);
        setupEffectSendKnob (liveScaling);
    }

    void setEngineSuffix (const String& engine)
    {
        header.setText (engine.isEmpty() ? baseName : baseName + " " + engine,
                        dontSendNotification);
    }

    TextButton& getOutputGroup1Button() noexcept { return btOutputGroup1; }
    TextButton& getOutputGroup2Button() noexcept { return btOutputGroup2; }
    TextButton& getEffectSendLine1Button() noexcept { return btEffectSendLine1; }
    TextButton& getEffectSendLine2Button() noexcept { return btEffectSendLine2; }
    TextButton& getEffectSendLine3Button() noexcept { return btEffectSendLine3; }
    TextButton& getEffectSendLine4Button() noexcept { return btEffectSendLine4; }

    void resized() override
    {
        const int rowH = 18;
        const int gap = 2;
        const int headerH = 20;

        auto bounds = getLocalBounds();
        header.setBounds (bounds.removeFromTop (headerH));

        auto placeRow = [&] (Component& c)
        {
            c.setBounds (bounds.removeFromTop (rowH));
            bounds.removeFromTop (gap);
        };

        placeRow (levelRowComponent());
        placeRow (detuneRowComponent());
        placeRow (noteShiftRowComponent());
        placeRow (dynamicPan);
        placeRow (noteLimitLowRowComponent());
        placeRow (noteLimitHighRowComponent());
        placeRow (velocityLimitLowRowComponent());
        placeRow (velocityLimitHighRowComponent());
        placeOutputGroupRow (bounds.removeFromTop (rowH));
        bounds.removeFromTop (gap);
        placeEffectSendBlock (bounds.removeFromTop (effectSendBlockH));
    }

private:
    Component& levelRowComponent()
    {
        return liveLevelControl != nullptr ? static_cast<Component&> (*liveLevelControl)
                                           : static_cast<Component&> (level);
    }

    Component& detuneRowComponent()
    {
        return liveDetuneControl != nullptr ? static_cast<Component&> (*liveDetuneControl)
                                          : static_cast<Component&> (detune);
    }

    Component& noteShiftRowComponent()
    {
        return liveNoteShiftControl != nullptr ? static_cast<Component&> (*liveNoteShiftControl)
                                             : static_cast<Component&> (noteShift);
    }

    Component& noteLimitLowRowComponent()
    {
        return liveNoteLimitLowControl != nullptr ? static_cast<Component&> (*liveNoteLimitLowControl)
                                                  : static_cast<Component&> (noteLimitLow);
    }

    Component& noteLimitHighRowComponent()
    {
        return liveNoteLimitHighControl != nullptr ? static_cast<Component&> (*liveNoteLimitHighControl)
                                                   : static_cast<Component&> (noteLimitHigh);
    }

    Component& velocityLimitLowRowComponent()
    {
        return liveVelocityLimitLowControl != nullptr ? static_cast<Component&> (*liveVelocityLimitLowControl)
                                                      : static_cast<Component&> (velocityLimitLow);
    }

    Component& velocityLimitHighRowComponent()
    {
        return liveVelocityLimitHighControl != nullptr ? static_cast<Component&> (*liveVelocityLimitHighControl)
                                                       : static_cast<Component&> (velocityLimitHigh);
    }

    Component& effectSendLevelRowComponent()
    {
        return liveEffectSendLevelControl != nullptr ? static_cast<Component&> (*liveEffectSendLevelControl)
                                                     : static_cast<Component&> (effectSendLevelPh);
    }

    Component& effectSendVelSensRowComponent()
    {
        return liveEffectSendVelSensControl != nullptr ? static_cast<Component&> (*liveEffectSendVelSensControl)
                                                       : static_cast<Component&> (effectSendVelSensPh);
    }

    Component& effectSendScalingRowComponent()
    {
        return liveEffectSendScalingControl != nullptr ? static_cast<Component&> (*liveEffectSendScalingControl)
                                                         : static_cast<Component&> (effectSendScalingPh);
    }

    void placeEffectSendBlock (Rectangle<int> block)
    {
        auto sendRow = block.removeFromTop (outputGroupRowH);
        const int btnGap = 2;
        const int btnW = (sendRow.getWidth() - btnGap) / 2;
        btEffectSendLine1.setBounds (sendRow.getX(), sendRow.getY(), btnW, sendRow.getHeight());
        btEffectSendLine3.setBounds (sendRow.getX() + btnW + btnGap, sendRow.getY(), btnW, sendRow.getHeight());

        block.removeFromTop (effectSendInnerGap);

        const int lblH = effectSendLblH;
        const int knobSz = effectSendKnobSz;
        const int colGap = 2;
        const int colW = (block.getWidth() - 2 * colGap) / 3;
        int x = block.getX();

        auto placeKnobCol = [&] (Label& lbl, Component& knob)
        {
            lbl.setBounds (x, block.getY(), colW, lblH);
            knob.setBounds (x + (colW - knobSz) / 2, block.getY() + lblH,
                            knobSz, knobSz);
            x += colW + colGap;
        };

        placeKnobCol (lblEffectSendLvl, effectSendLevelRowComponent());
        placeKnobCol (lblEffectSendVel, effectSendVelSensRowComponent());
        placeKnobCol (lblEffectSendScl, effectSendScalingRowComponent());
    }

    static void setupOutputGroupButton (TextButton& bt, const String& label)
    {
        bt.setButtonText (label);
        bt.setClickingTogglesState (true);
        bt.setToggleState (false, dontSendNotification);
        bt.setColour (TextButton::buttonColourId, Colours::grey);
        bt.setColour (TextButton::buttonOnColourId, Colours::yellow);
        bt.setColour (TextButton::textColourOffId, Colours::white);
        bt.setColour (TextButton::textColourOnId, Colours::black);
    }

    void placeOutputGroupRow (Rectangle<int> row)
    {
        const int btnGap = 2;
        const int btnW = (row.getWidth() - btnGap) / 2;
        btOutputGroup1.setBounds (row.getX(), row.getY(), btnW, row.getHeight());
        btOutputGroup2.setBounds (row.getX() + btnW + btnGap, row.getY(), btnW, row.getHeight());
    }

    void setupEffectSendKnob (Slider& s)
    {
        s.setLookAndFeel (&effectSendKnobLF);
        s.setSliderStyle (Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        s.setColour (Slider::thumbColourId, Colours::darkorange);
    }

    static void setupPlaceholderSlider (Slider& s)
    {
        s.setEnabled (false);
        s.setSliderStyle (Slider::LinearHorizontal);
        s.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);
        s.setRange (0, 127, 1);
    }

    static void styleMixerStripSlider (MidiSlider& s)
    {
        s.setSliderStyle (Slider::LinearHorizontal);
        s.setTextBoxStyle (Slider::TextBoxRight, false, 36, 16);
    }

    static void setupReservedPlaceholder (Label& lab)
    {
        lab.setText ("—", dontSendNotification);
        lab.setJustificationType (Justification::centred);
        lab.setColour (Label::textColourId, Colours::grey);
    }

    String baseName;
    Label header;

    Slider level;
    MidiSlider* liveLevelControl = nullptr;
    MidiSlider* liveNoteLimitLowControl = nullptr;
    MidiSlider* liveNoteLimitHighControl = nullptr;
    MidiSlider* liveVelocityLimitLowControl = nullptr;
    MidiSlider* liveVelocityLimitHighControl = nullptr;
    MidiSlider* liveDetuneControl = nullptr;
    MidiSlider* liveNoteShiftControl = nullptr;
    MidiSlider* liveEffectSendLevelControl = nullptr;
    MidiSlider* liveEffectSendVelSensControl = nullptr;
    MidiSlider* liveEffectSendScalingControl = nullptr;
    Slider detune, noteShift, dynamicPan,
           noteLimitLow, noteLimitHigh, velocityLimitLow, velocityLimitHigh;

    TextButton btOutputGroup1 { "Group 1" };
    TextButton btOutputGroup2 { "Group 2" };

    TextButton btEffectSendLine1 { "Send 1" };
    TextButton btEffectSendLine2 { "2" };
    TextButton btEffectSendLine3 { "Send 3" };
    TextButton btEffectSendLine4 { "4" };

    Label lblEffectSendLvl;
    Label lblEffectSendVel;
    Label lblEffectSendScl;
    FiltreControlLookAndFeel effectSendKnobLF;

    static constexpr int outputGroupRowH = 18;
    static constexpr int effectSendInnerGap = 2;
    static constexpr int effectSendLblH = 10;
    static constexpr int effectSendKnobSz = 22;
    static constexpr int effectSendBlockH = kEffectSendBlockH;

    Slider effectSendLevelPh;
    Slider effectSendVelSensPh;
    Slider effectSendScalingPh;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerElementStrip)
};

//==============================================================================
/** Four element strips with shared row labels (Mixer v1 layout only). */
class MixerSection : public Component
{
public:
    MixerSection()
        : strip1 ("El.1"), strip2 ("El.2"), strip3 ("El.3"), strip4 ("El.4")
    {
        static const char* rowNames[numRows] =
        {
            "Level",
            "Detune",
            "Note Shift",
            "Dynamic Pan",
            "Note Limit Low",
            "Note Limit High",
            "Velocity Limit Low",
            "Velocity Limit High",
            "Output Group",
            "Effect Send"
        };

        for (int i = 0; i < numRows; ++i)
        {
            rowLabels[i].setText (rowNames[i], dontSendNotification);
            rowLabels[i].setJustificationType (Justification::centredLeft);
            rowLabels[i].setColour (Label::textColourId,
                                    i < numParamRows ? Colours::darkorange : Colours::grey);
            addAndMakeVisible (rowLabels[i]);
        }

        for (auto* strip : { &strip1, &strip2, &strip3, &strip4 })
            addAndMakeVisible (strip);
    }

    void setActiveStripCount (int count)
    {
        activeStripCount = jlimit (0, 4, count);
        strip1.setVisible (activeStripCount > 0);
        strip2.setVisible (activeStripCount > 1);
        strip3.setVisible (activeStripCount > 2);
        strip4.setVisible (activeStripCount > 3);
        if (isShowing())
            resized();
    }

    int getActiveStripCount() const noexcept { return activeStripCount; }

    static int preferredContentWidth (int visibleStripCount, int hostInnerWidth)
    {
        const int strips = jmax (1, visibleStripCount);
        const int availableForStrips = jmax (0, hostInnerWidth - labelColumnWidth);
        const int stripGap = stripGapPx;
        const int totalGap = stripGap * jmax (0, strips - 1);

        if (strips >= 4)
            return labelColumnWidth + availableForStrips;

        return labelColumnWidth + strips * naturalStripWidth + totalGap;
    }

    void setStripEngineType (int stripIndex, int opMode)
    {
        const String engine = engineLabelForOpMode (opMode);
        if (auto* strip = getStrip (stripIndex))
            strip->setEngineSuffix (engine);
    }

    void attachStripLevelControl (int stripIndex, MidiSlider& liveLevel)
    {
        if (auto* strip = getStrip (stripIndex))
            strip->attachLiveLevelControl (liveLevel);
    }

    void attachStripNoteLimitLowControl (int stripIndex, MidiSlider& liveNoteLimitLow)
    {
        if (auto* strip = getStrip (stripIndex))
        {
            strip->attachLiveNoteLimitLowControl (liveNoteLimitLow);
            strip->resized();
        }
    }

    void attachStripNoteLimitHighControl (int stripIndex, MidiSlider& liveNoteLimitHigh)
    {
        if (auto* strip = getStrip (stripIndex))
        {
            strip->attachLiveNoteLimitHighControl (liveNoteLimitHigh);
            strip->resized();
        }
    }

    void attachStripVelocityLimitLowControl (int stripIndex, MidiSlider& liveVelocityLimitLow)
    {
        if (auto* strip = getStrip (stripIndex))
        {
            strip->attachLiveVelocityLimitLowControl (liveVelocityLimitLow);
            strip->resized();
        }
    }

    void attachStripVelocityLimitHighControl (int stripIndex, MidiSlider& liveVelocityLimitHigh)
    {
        if (auto* strip = getStrip (stripIndex))
        {
            strip->attachLiveVelocityLimitHighControl (liveVelocityLimitHigh);
            strip->resized();
        }
    }

    void attachStripDetuneControl (int stripIndex, MidiSlider& liveDetune)
    {
        if (auto* strip = getStrip (stripIndex))
        {
            strip->attachLiveDetuneControl (liveDetune);
            strip->resized();
        }
    }

    void relayoutLiveStripRows()
    {
        for (auto* strip : { &strip1, &strip2, &strip3, &strip4 })
            if (strip->isVisible())
                strip->resized();
    }

    void attachStripNoteShiftControl (int stripIndex, MidiSlider& liveNoteShift)
    {
        if (auto* strip = getStrip (stripIndex))
        {
            strip->attachLiveNoteShiftControl (liveNoteShift);
            strip->resized();
        }
    }

    void detachStripDetuneControl (int stripIndex)
    {
        if (auto* strip = getStrip (stripIndex))
            strip->detachLiveDetuneControl();
    }

    void detachStripNoteShiftControl (int stripIndex)
    {
        if (auto* strip = getStrip (stripIndex))
            strip->detachLiveNoteShiftControl();
    }

    void attachStripEffectSendControls (int stripIndex, MidiSlider& liveLevel,
                                        MidiSlider& liveVelSens, MidiSlider& liveScaling)
    {
        if (auto* strip = getStrip (stripIndex))
            strip->attachLiveEffectSendControls (liveLevel, liveVelSens, liveScaling);
    }

    void bindOutputGroupForStrip (int stripIndex, MixerOutputGroupBinding& binding, MidiSlider& sendVia)
    {
        if (auto* strip = getStrip (stripIndex))
        {
            const uint8 elementOffset = (uint8) (stripIndex * 0x20);
            binding.attachForElement (elementOffset,
                                      strip->getOutputGroup1Button(),
                                      strip->getOutputGroup2Button(),
                                      sendVia);
        }
    }

    void bindEffectSendSelectForStrip (int stripIndex, MixerEffectSendSelectBinding& binding,
                                       MidiSlider& sendVia, MidiSlider* effectModeVia = nullptr)
    {
        if (auto* strip = getStrip (stripIndex))
        {
            const uint8 elementOffset = (uint8) (stripIndex * 0x20);
            binding.attachForElement (elementOffset,
                                      strip->getEffectSendLine1Button(),
                                      strip->getEffectSendLine3Button(),
                                      sendVia,
                                      effectModeVia);
        }
    }

    void resized() override
    {
        const int rowH = 18;
        const int gap = 2;
        const int headerH = 20;

        int visibleCount = 0;
        for (auto* strip : { &strip1, &strip2, &strip3, &strip4 })
            if (strip->isVisible())
                ++visibleCount;

        if (visibleCount < 1)
            visibleCount = 1;

        const int availableForStrips = jmax (0, getWidth() - labelColumnWidth);
        const int stripGap = stripGapPx;
        const int totalGap = stripGap * jmax (0, visibleCount - 1);
        const int stripColW = stripWidthForLayout (visibleCount, availableForStrips, totalGap);

        int xStrip = labelColumnWidth;

        for (auto* strip : { &strip1, &strip2, &strip3, &strip4 })
        {
            if (! strip->isVisible())
                continue;

            strip->setBounds (xStrip, 0, stripColW, getHeight());
            xStrip += stripColW + stripGap;
        }

        int labelY = headerH;
        for (int i = 0; i < numRows; ++i)
        {
            const int labelH = (i == numRows - 1) ? MixerElementStrip::kEffectSendBlockH : rowH;
            rowLabels[i].setBounds (0, labelY, labelColumnWidth - 4, labelH);
            labelY += labelH + gap;
        }
    }

    static int preferredHeight()
    {
        const int rowH = 18;
        const int gap = 2;
        const int headerH = 20;
        return headerH + (numRows - 1) * (rowH + gap) + MixerElementStrip::kEffectSendBlockH;
    }

private:
    static constexpr int numParamRows = 10;
    static constexpr int numReservedRows = 0;
    static constexpr int numRows = numParamRows + numReservedRows;
    static constexpr int labelColumnWidth = 112;
    static constexpr int naturalStripWidth = 88;
    static constexpr int stripGapPx = 4;

    static int stripWidthForLayout (int visibleCount, int availableForStrips, int totalGap)
    {
        if (visibleCount >= 4)
            return jmax (48, (availableForStrips - totalGap) / visibleCount);

        return jmin (naturalStripWidth, jmax (48, availableForStrips));
    }

    static String engineLabelForOpMode (int opMode)
    {
        switch (opMode)
        {
            case 1: // Element::AFMmono
            case 2: // Element::AFMPoly
                return "AFM";
            case 3: // Element::AWM
                return "AWM";
            case 4: // Element::DrumSet
                return "Drum";
            default:
                return {};
        }
    }

    MixerElementStrip* getStrip (int index)
    {
        switch (index)
        {
            case 0: return &strip1;
            case 1: return &strip2;
            case 2: return &strip3;
            case 3: return &strip4;
            default: return nullptr;
        }
    }

    Label rowLabels[numRows];
    MixerElementStrip strip1, strip2, strip3, strip4;
    int activeStripCount = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerSection)
};

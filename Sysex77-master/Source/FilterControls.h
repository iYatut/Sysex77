/*
  ==============================================================================

    FilterControls.h
    Custom programmatic JUCE components for the Filter section.
    No PNG dependencies — all drawing is done via the JUCE Graphics API.

    Components:
      - FiltreControlLookAndFeel  : dark rotary knob with orange gradient arc
      - ButtonFilterLookAndFeel   : rounded toggle button with glow effect
      - SwitchFilter              : 3-state segmented LP / BP / HP selector

    Design tokens:
      Background  #1a1a1a   (Colour 0xff1a1a1a)
      Orange accent #FF8C00  (Colour 0xffFF8C00)

  ==============================================================================
*/

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"


//==============================================================================
// FiltreControlLookAndFeel
// Apply to any MidiSlider set to Rotary style.
// Replaces the PNG filmstrip approach with a fully scalable vector knob.
//==============================================================================
class FiltreControlLookAndFeel : public LookAndFeel_V4
{
public:
    FiltreControlLookAndFeel() = default;

    void drawRotarySlider (Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle,
                           Slider& /*slider*/) override
    {
        const float cx = x + width  * 0.5f;
        const float cy = y + height * 0.5f;
        const float outerR = jmin (width * 0.5f, height * 0.5f) - 3.0f;

        if (outerR <= 0.0f)
            return;

        const float angle = rotaryStartAngle
                            + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // ── Drop shadow ────────────────────────────────────────────────────
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (Colour (0xff000000).withAlpha (0.06f * i));
            const float sr = outerR + i * 1.5f;
            g.fillEllipse (cx - sr, cy - sr, sr * 2.0f, sr * 2.0f);
        }

        // ── Outer rim / bezel ──────────────────────────────────────────────
        g.setColour (Colour (0xff0d0d0d));
        g.fillEllipse (cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f);

        g.setColour (Colour (0xff3a3a3a));
        g.drawEllipse (cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f, 1.0f);

        // ── Track arc ──────────────────────────────────────────────────────
        const float trackR = outerR - 5.0f;
        {
            Path track;
            track.addCentredArc (cx, cy, trackR, trackR, 0.0f,
                                 rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (Colour (0xff272727));
            g.strokePath (track, PathStrokeType (3.5f,
                                                 PathStrokeType::curved,
                                                 PathStrokeType::rounded));
        }

        // ── Value arc (orange gradient) ────────────────────────────────────
        if (sliderPos > 0.001f)
        {
            Path valueArc;
            valueArc.addCentredArc (cx, cy, trackR, trackR, 0.0f,
                                    rotaryStartAngle, angle, true);
            ColourGradient arcGrad (Colour (0xffFF8C00), cx, cy - trackR,
                                    Colour (0xffFF4400), cx, cy + trackR,
                                    false);
            g.setGradientFill (arcGrad);
            g.strokePath (valueArc, PathStrokeType (3.5f,
                                                    PathStrokeType::curved,
                                                    PathStrokeType::rounded));
        }

        // ── Inner knob body ────────────────────────────────────────────────
        const float knobR = outerR * 0.60f;
        {
            ColourGradient kg (Colour (0xff383838),
                               cx - knobR * 0.4f, cy - knobR * 0.4f,
                               Colour (0xff141414),
                               cx + knobR * 0.4f, cy + knobR * 0.4f,
                               false);
            g.setGradientFill (kg);
            g.fillEllipse (cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f);
        }

        g.setColour (Colour (0xff484848));
        g.drawEllipse (cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f, 1.0f);

        // ── Pointer dot ────────────────────────────────────────────────────
        const float dotDist = knobR * 0.68f;
        const float dotR    = jmax (1.5f, knobR * 0.11f);
        const float dotX    = cx + dotDist * std::sin (angle);
        const float dotY    = cy - dotDist * std::cos (angle);

        g.setColour (Colour (0xffFF8C00));
        g.fillEllipse (dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f);

        // Subtle glow around pointer dot
        g.setColour (Colour (0xffFF8C00).withAlpha (0.25f));
        g.fillEllipse (dotX - dotR * 2.2f, dotY - dotR * 2.2f,
                       dotR * 4.4f, dotR * 4.4f);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FiltreControlLookAndFeel)
};


//==============================================================================
// ButtonFilterLookAndFeel
// Apply to MidiButton for a modern rounded toggle with glow.
//==============================================================================
class ButtonFilterLookAndFeel : public LookAndFeel_V4
{
public:
    ButtonFilterLookAndFeel() = default;

    void drawButtonBackground (Graphics& g, Button& button,
                               const Colour& /*backgroundColour*/,
                               bool isMouseOverButton,
                               bool isButtonDown) override
    {
        const auto  bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const float cr     = bounds.getHeight() * 0.32f;
        const bool  isOn   = button.getToggleState();

        if (isOn)
        {
            // Outer glow rings
            for (int i = 3; i >= 1; --i)
            {
                g.setColour (Colour (0xffFF8C00).withAlpha (0.07f * i));
                g.fillRoundedRectangle (bounds.expanded (i * 1.8f), cr + i * 1.8f);
            }

            // Active fill
            ColourGradient grad (Colour (0xffD07200), bounds.getCentreX(), bounds.getY(),
                                 Colour (0xff8C4400), bounds.getCentreX(), bounds.getBottom(),
                                 false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (bounds, cr);

            // Orange rim
            g.setColour (Colour (0xffFF8C00).withAlpha (0.85f));
            g.drawRoundedRectangle (bounds, cr, 1.0f);
        }
        else
        {
            const Colour base = isButtonDown   ? Colour (0xff2d2d2d)
                              : isMouseOverButton ? Colour (0xff252525)
                                                  : Colour (0xff1a1a1a);
            g.setColour (base);
            g.fillRoundedRectangle (bounds, cr);

            g.setColour (Colour (0xff3a3a3a));
            g.drawRoundedRectangle (bounds, cr, 1.0f);
        }
    }

    void drawButtonText (Graphics& g, TextButton& button,
                         bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        const bool isOn = button.getToggleState();
        g.setColour (isOn ? Colour (0xffffffff) : Colour (0xff888888));
        g.setFont (Font (button.getHeight() * 0.40f, Font::bold));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds().reduced (4, 0),
                          Justification::centred, 1);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ButtonFilterLookAndFeel)
};


//==============================================================================
// SwitchFilter
//
// Three-state segmented control: LP | BP | HP
// Drop-in replacement for MidiRadio — exposes the same getValueObject() API.
//
// Value mapping:  0 = LP   1 = BP   2 = HP
//==============================================================================
class SwitchFilter : public Component, private Value::Listener
{
public:
    SwitchFilter()
    {
        stateValue.addListener (this);
        stateValue.setValue (0);
    }

    ~SwitchFilter() override
    {
        stateValue.removeListener (this);
    }

    //──────────────────────────────────────────────────────────────────────────
    // MidiRadio-compatible interface
    //──────────────────────────────────────────────────────────────────────────

    /** Returns a reference to the internal Value so it can be bound via referTo(). */
    Value& getValueObject()  { return stateValue; }

    int  getValue()  const   { return (int) stateValue.getValue(); }
    void setValue (int v)    { stateValue.setValue (jlimit (0, 2, v)); }

    /** No-op — kept for drop-in compatibility with addRadio() call sites. */
    void addRadio (const String& /*label*/, int /*radioGroupId*/) {}

    //──────────────────────────────────────────────────────────────────────────
    // Component
    //──────────────────────────────────────────────────────────────────────────

    void paint (Graphics& g) override
    {
        const auto  bounds = getLocalBounds().toFloat();
        const float h      = bounds.getHeight();
        const float segW   = bounds.getWidth() / 3.0f;
        const float cr     = h * 0.28f;
        const int   active = getValue();

        // Background pill
        g.setColour (Colour (0xff141414));
        g.fillRoundedRectangle (bounds, cr);

        // Active segment
        {
            const Rectangle<float> seg (bounds.getX() + active * segW,
                                        bounds.getY(), segW, h);

            // Glow halo behind segment
            g.setColour (Colour (0xffFF8C00).withAlpha (0.15f));
            g.fillRoundedRectangle (seg.expanded (3.0f), cr + 3.0f);

            // Clip to pill so corners stay clean
            g.saveState();
            Path pill;
            pill.addRoundedRectangle (bounds, cr);
            g.reduceClipRegion (pill);

            ColourGradient fg (Colour (0xffFF8C00), seg.getCentreX(), seg.getY(),
                               Colour (0xffBF5500), seg.getCentreX(), seg.getBottom(),
                               false);
            g.setGradientFill (fg);
            g.fillRect (seg.reduced (2.0f, 2.0f));

            g.restoreState();
        }

        // Labels + dividers
        const StringArray labels { "LP", "BP", "HP" };
        const Font  font (h * 0.50f, Font::bold);

        for (int i = 0; i < 3; ++i)
        {
            const Rectangle<float> seg (bounds.getX() + i * segW,
                                        bounds.getY(), segW, h);

            g.setFont (font);
            g.setColour (i == active ? Colour (0xffffffff) : Colour (0xff777777));
            g.drawText (labels[i], seg.toNearestInt(), Justification::centred, false);

            // Divider between segments
            if (i < 2)
            {
                g.setColour (Colour (0xff353535));
                g.drawLine (bounds.getX() + (i + 1) * segW,
                            bounds.getY() + h * 0.18f,
                            bounds.getX() + (i + 1) * segW,
                            bounds.getY() + h * 0.82f,
                            1.0f);
            }
        }

        // Outer border
        g.setColour (Colour (0xff3a3a3a));
        g.drawRoundedRectangle (bounds.reduced (0.5f), cr, 1.0f);
    }

    void mouseDown (const MouseEvent& e) override
    {
        const int newState = jlimit (0, 2,
                                     (int) ((float) e.x / ((float) getWidth() / 3.0f)));
        stateValue.setValue (newState);
        repaint();
    }

    void resized() override {}

private:
    void valueChanged (Value&) override { repaint(); }

    Value stateValue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwitchFilter)
};

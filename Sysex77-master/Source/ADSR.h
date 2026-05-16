/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

  name:             ADSR

  dependencies:     juce_core, juce_data_structures, juce_events, juce_graphics, juce_gui_basics
  exporters:        xcode_mac

  moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

  type:             Component
  mainClass:        MainComponent

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once
#include "Hook.h"

//==============================================================================
class SADSR   : public Component, public ChangeListener, public ChangeBroadcaster
{
public:
    //==============================================================================
    SADSR()
    {
      //  setSize (600, 400);
    /*
        addSegment(0,10,"R1"); // add a segment of envelope
        addSegment(80,50, "R2");
        addSegment(20,30, "R3");
        addSegment(20,30, "R4");
        setModeHold(true);
        setRelease(50);
        resized();
        repaint();
*/

        
    }

    ~SADSR()
    {
        for (int i = 0; i < arraySegment.size();i++)
        {
            arraySegment[i]->removeChangeListener(this);
        }
    }

    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        Logger::writeToLog("change message"); //Notify a change from a hook
        repaint();
        sendChangeMessage(); // notify Filter1/Filter2 only when a segment was actually dragged
    }
    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll(Colour(0xff1a1a1a));

        if (arraySegment.size() == 0)
        {
            g.setColour(Colour(0x40808080));
            g.drawText(strName, getLocalBounds(), Justification::centred, true);
            return;
        }

        g.setColour(Colour(0x10000000));
        for (int i = 0; i < 17; i++)
        {
            auto a = getHeight()/16;
            auto b = getWidth()/16;
            g.drawLine(0, a*i, getWidth(), a*i);
            g.drawLine(b*i, 0, b*i, getHeight());
        }

        // Dotted center line: SY99 level=0 is at 50% height (Y axis center)
        {
            g.setColour(Colour(0x70ff8c00));
            float cy = getHeight() * 0.5f;
            float dashLen = 6.0f, gapLen = 4.0f;
            for (float x = 0; x < getWidth(); x += dashLen + gapLen)
                g.drawLine(x, cy, jmin(x + dashLen, (float)getWidth()), cy, 1.5f);
            g.setColour(Colour(0x90ff8c00));
            g.drawFittedText("L=0", 2, (int)cy - 10, 28, 10, Justification::centredLeft, 1);
        }

        // HT (Key-On Delay) region — flat hold at L0 before EG starts moving.
        // Only shown when boolModeHold is active and HT > 0.
        if (boolModeHold && intHoldTime > 0)
        {
            const int segCount    = arraySegment.size();
            const int segWidthFull = getWidth() / (segCount + 1);
            const int htX = (intHoldTime * segWidthFull) / 63;
            if (htX > 1)
            {
                // Shaded background strip
                g.setColour(Colour(0x20ffcc44));
                g.fillRect(0, 0, htX, getHeight());
                // Right border of HT zone
                g.setColour(Colour(0x70ffcc44));
                g.drawLine((float)htX, 0.0f, (float)htX, (float)getHeight(), 1.5f);
                // "HT" label just above the L0 line
                float L0_Y = arraySegment[0]->getPositionY() + 8.0f + arraySegment[0]->getY();
                g.setColour(Colour(0xc0ffcc44));
                g.drawFittedText("HT", 2, jmax(2, (int)L0_Y - 20),
                                 jmax(6, htX - 4), 14,
                                 Justification::centred, 1);
            }
        }

        // Envelope path — Y increases downward (screen coords)
        // intLevel 50% = SY99 level 0 (center line), 0% = bottom, 100% = top
        g.setColour(Colours::darkorange);
        Path myPath;
        if (boolModeHold)
        {
            myPath.startNewSubPath(-8, getHeight() + 8);
            myPath.lineTo(0, arraySegment[0]->getPositionY() + 8 + arraySegment[0]->getY());
        }
        else
        {
            myPath.startNewSubPath(-4, getHeight() + 4);
            myPath.lineTo(0, getHeight() - 8);
        }

        for (int i = 0; i < arraySegment.size(); i++)
        {
            int w = arraySegment[i]->getPositionX() + 8 + arraySegment[i]->getX();
            int y = arraySegment[i]->getPositionY() + 8 + arraySegment[i]->getY();
            myPath.lineTo(w, y);
        }

        int wgrid = (arraySegment[arraySegment.size()-1]->getPositionX()
                     + arraySegment[arraySegment.size()-1]->getX()) + 8;
        int releaseEndX = wgrid + ((getWidth() / (arraySegment.size() + 1) * intRelease) / 100);
        float releaseEndY = (float)(getHeight() * (100 - intReleaseLevel) / 100);
        myPath.lineTo(releaseEndX, releaseEndY);
        myPath.lineTo(releaseEndX, (float)(getHeight() + 4));
        myPath.closeSubPath();

        // Fill area under the curve — gradient bright orange at top, fade down
        ColourGradient fillGrad { Colour(0xc0ff8c00), static_cast<float>(getWidth() / 2), 0,
                                  Colour(0x20804000), static_cast<float>(getWidth() / 2),
                                  static_cast<float>(getHeight()), false };
        g.setGradientFill(fillGrad);
        g.fillPath(myPath);

        // Stroke on top
        g.setColour(Colour(0xe0ff8c00));
        g.strokePath(myPath, PathStrokeType(2.5f));

        // Vertical dashed line at key-off boundary
        if (keyOffSegmentIdx >= 0 && keyOffSegmentIdx < arraySegment.size())
        {
            int koffX = arraySegment[keyOffSegmentIdx]->getPositionX() + 8 + arraySegment[keyOffSegmentIdx]->getX();
            g.setColour(Colour(0x90ffff44));
            float dashH = 5.0f, gapH = 4.0f;
            for (float ky = 0; ky < getHeight(); ky += dashH + gapH)
                g.drawLine((float)koffX, ky, (float)koffX, jmin(ky + dashH, (float)getHeight()), 1.5f);
            g.setColour(Colour(0xb0ffff44));
            g.drawFittedText("KeyOff", koffX - 24, 16, 48, 11, Justification::centred, 1);
        }

        // SLP sustain loop bracket: spans from seg[intLoopPoint] → seg[keyOffSegmentIdx] (L4)
        // Drawn as a coloured bar at the top of the widget with a return arrow.
        if (intLoopPoint >= 1
            && keyOffSegmentIdx > intLoopPoint
            && intLoopPoint < (int)arraySegment.size()
            && keyOffSegmentIdx < (int)arraySegment.size())
        {
            int slpX  = arraySegment[intLoopPoint]->getPositionX()
                        + 8 + arraySegment[intLoopPoint]->getX();
            int koffX = arraySegment[keyOffSegmentIdx]->getPositionX()
                        + 8 + arraySegment[keyOffSegmentIdx]->getX();

            if (slpX < koffX)
            {
                const int barY = 30, barH = 9;

                // Fill
                g.setColour(Colour(0x3500b8ff));
                g.fillRect(slpX, barY, koffX - slpX, barH);
                // Border
                g.setColour(Colour(0xa000b8ff));
                g.drawRect((float)slpX, (float)barY,
                           (float)(koffX - slpX), (float)barH, 1.5f);
                // Vertical ticks at loop boundary
                g.drawLine((float)slpX,  (float)barY, (float)slpX,  (float)(barY + barH + 5), 1.5f);
                g.drawLine((float)koffX, (float)barY, (float)koffX, (float)(barY + barH + 5), 1.5f);
                // Return arrow pointing from L4 back to SLP
                const float midY = barY + barH * 0.5f;
                g.drawArrow({ (float)koffX, midY, (float)slpX, midY }, 1.0f, 5.0f, 5.0f);
                // Label
                if (koffX - slpX > 22)
                    g.drawFittedText(CharPointer_UTF8("\xe2\x86\xba"),
                                     slpX + 2, barY, koffX - slpX - 4, barH,
                                     Justification::centred, 1);
            }
        }

        g.setColour(Colours::white);
        g.drawText(strName, getLocalBounds(), Justification::topRight, true);

        g.setColour(Colour(0x80ffffff));
        g.drawFittedText("Rel: " + String(intRelease),
                         getWidth() - 60, 4, 56, 14, Justification::right, 1);
    }
    void setR4 (bool enable, int rate)
    {
        
    }
    void setL1 (int level)
    {
        arraySegment[0]->setLevel(level);
    }
    int getL1 ()
    {
        return arraySegment[0]->getLevel();
    }
    void setRelease(int release)
    {
    //    Logger::writeToLog("setRelease " + String(release));
        intRelease = release;
        repaint();
    }
    int getRelease ()
    {
        return intRelease;
    }
    void setReleaseLevel(int level)
    {
        intReleaseLevel = level;
        repaint();
    }
    int getReleaseLevel()
    {
        return intReleaseLevel;
    }
    void setName(String name)
    {
        strName = name;
    }
    String getName()
    {
        return strName;
    }
    void setEasyDraw (bool easyDraw)
    {
        boolEasyDraw = easyDraw;
        resized();
    }
    void setModeHold(bool trueOrFalse)
    {
        boolModeHold = trueOrFalse;
        repaint();
    }
    void setKeyOffSegment(int idx)
    {
        keyOffSegmentIdx = idx;
        repaint();
    }

    // HT = Key-On Delay Time: flat hold at L0 before the EG starts moving.
    // Visually shifts all segments right by a proportional amount when boolModeHold=true.
    void setHoldTime(int ht)
    {
        intHoldTime = jmax(0, ht);
        resized();
        repaint();
    }
    int getHoldTime() const { return intHoldTime; }

    // SLP = Sustain Loop Point: loop from L4 back to L(slp) while key is held.
    // slp in 1..4 draws a loop bracket; 0 or negative disables the indicator.
    void setLoopPoint(int slp)
    {
        intLoopPoint = (slp >= 1) ? slp : -1;
        repaint();
    }
    int getLoopPoint() const { return intLoopPoint; }

    void clearSegments()
    {
        for (int i = 0; i < arraySegment.size(); i++)
            arraySegment[i]->removeChangeListener(this);
        arraySegment.clear();
        repaint();
    }
    int getSegmentCount() const { return arraySegment.size(); }
    int getSegmentLevel(int idx) const
    {
        if (idx >= 0 && idx < arraySegment.size())
            return arraySegment[idx]->getLevel();
        return 0;
    }
    int getSegmentRate(int idx) const
    {
        if (idx >= 0 && idx < arraySegment.size())
            return arraySegment[idx]->getRate();
        return 0;
    }
    void setSegmentLevel(int idx, int level)
    {
        if (idx >= 0 && idx < arraySegment.size())
            arraySegment[idx]->setLevel(level);
    }
    void setSegmentRate(int idx, int rate)
    {
        if (idx >= 0 && idx < arraySegment.size())
            arraySegment[idx]->setValue(rate, arraySegment[idx]->getLevel());
    }
    void addSegment(int level, int rate, String name,int rateRange, int sysexRate[0],int levelRange, int sysexLevel[0])
    {
        auto* segment = arraySegment.add (new Segment);
        
        addAndMakeVisible(segment);
        segment->addChangeListener(this);
        segment->setMidiSysexRate(sysexRate);
        segment->setMidiSysexLevel(sysexLevel);
        segment->setValue(rate, level);
        segment->setRange (rateRange, levelRange); // 100 % = Range
        segment->setName(name);
        
  //      arrayHook[arrayHook.size()]->setTopRightPosition(getWidth(), getHeight());
/*
        int wgrid = getWidth()/arraySegment.size();
        for(int i = 0; i < arraySegment.size();i++)
        {
     
        arraySegment[i]->setBounds(getWidth()-wgrid, 0, wgrid, getHeight());
        }
*/        
    }
    void resized() override
    {
        DBG("SADSR resized: " + getBounds().toString()
            + "  segments=" + String(arraySegment.size()));

        if (arraySegment.isEmpty()) return;

        const int segCount = arraySegment.size();

        // HT pre-delay: reserve a proportional strip on the left.
        // At HT=63 (max) the strip equals one segment-width.
        // Only applied when boolModeHold is active (Pan EG).
        const int segWidthFull = getWidth() / (segCount + 1);
        const int htX = (intHoldTime > 0 && boolModeHold)
                            ? (intHoldTime * segWidthFull) / 63
                            : 0;

        // Reduce per-segment width so total content fits within getWidth()
        const int segWidth = (htX > 0) ? (getWidth() - htX) / (segCount + 1)
                                        : segWidthFull;

        int w = htX;
        for (int i = 0; i < segCount; i++)
        {
            if (i > 0)
                w = w + arraySegment[i - 1]->getPositionX() + 16;
            arraySegment[i]->setBounds(w, 0, segWidth, getHeight());
        }
    }


private:
    //==============================================================================
    // Your private member variables go here...
    String strName {"Name "};
    bool boolModeHold = false;
    bool boolR4Enable = false;
    bool boolEasyDraw = false;
    int intR4Rate;
    int intRelease = 0;
    int intReleaseLevel = 50;
    int keyOffSegmentIdx = 4;
    int intHoldTime  = 0;   // HT: Key-On Delay Time (0..63), shifts segments right when boolModeHold
    int intLoopPoint = -1;  // SLP: sustain loop start segment index (1-4 for L1..L4, -1 = off)

    OwnedArray<Segment> arraySegment;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SADSR)
};


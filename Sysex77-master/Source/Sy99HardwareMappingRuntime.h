/*
  ==============================================================================

    Sy99HardwareMappingRuntime.h
    Runtime: MIDI CC ↔ ControllerTemplate slot ↔ SY99 SysEx parameter.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <functional>

namespace Sy99HardwareMappingRuntime
{
    using SendSysExFn = std::function<void (const uint8_t frame[9])>;
    using SendCcFn    = std::function<void (int channel, int cc, int value,
                                            const juce::String& feedbackPortMatch)>;

    void initializeAtStartup();

    void setHandlers (SendSysExFn sendSysEx, SendCcFn sendCc);

    void reloadFromStore();

    /** MIDI input thread / async: CC from external controller. */
    void handleIncomingCc (const juce::String& inputPortName,
                           int channel, int cc, int value, uint8_t sysexDeviceByte);

    /** Inbound SY99 parameter frame or resolved live value → CC/LED feedback. */
    void onLiveParameterSysex (uint8 b3, uint8 b4, uint8 b5, uint8 b6, uint8 b8);

    /** Send 9-byte parameter frame (e.g. focus editor page on SY99). Returns false if no MIDI handler. */
    bool sendParameterSysexFrame (const uint8 frame[9]) noexcept;
}

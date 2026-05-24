/*
  Sy99Ui.h — user-visible UI strings.

  v1: Russian only (UTF-8 literals via String::fromUTF8).
  Future: read locale from settings and map keys → translated text.
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace Sy99Ui
{
    /** All UI copy goes through here — safe UTF-8 on Windows. */
    inline juce::String tr (const char* utf8Literal) noexcept
    {
        if (utf8Literal == nullptr || utf8Literal[0] == '\0')
            return {};

        return juce::String::fromUTF8 (utf8Literal);
    }

    inline juce::String navMultiModeHintSuffix() noexcept
    {
        return tr (u8" — SY-99 в режиме Multi; recall голоса может не менять звук");
    }

    inline juce::String midiInOutNotOpen() noexcept
    {
        return tr (u8"MIDI In и Out не открыты — вкладка Midi Setting, выберите порты SY99");
    }

    inline juce::String midiOutNotOpen() noexcept
    {
        return tr (u8"MIDI Out не открыт — без него dump request не уйдёт на SY99");
    }

    inline juce::String midiInNotOpen() noexcept
    {
        return tr (u8"MIDI In не открыт — ответ SY99 не будет принят");
    }

    inline juce::String dumpRequestNoReply (const juce::String& caseTag, bool variantB) noexcept
    {
        juce::String msg;
        msg << tr (u8"SY99 не ответил на dump request (case ") << caseTag;
        msg << tr (u8", варианты A");

        if (variantB)
            msg << tr (u8"+B");

        msg << tr (u8"). Bulk Protect=OFF, Engine=01, MIDI In/Out открыты. ");
        msg << tr (u8"Смотрите [TX] в MIDI monitor и [DumpReq] в логе.");
        return msg;
    }

    inline juce::String dumpRequest8101Sent (const juce::String& caseTag) noexcept
    {
        juce::String msg;
        msg << tr (u8"8101 dump request отправлен (case ") << caseTag;
        msg << tr (u8") — смотрите [TX] в MIDI monitor, ждём ответ SY99…");
        return msg;
    }

    inline juce::String dumpRequestVariantASent (const juce::String& caseTag) noexcept
    {
        juce::String msg;
        msg << tr (u8"Dump request вариант A отправлен (case ") << caseTag;
        msg << tr (u8") — смотрите [TX] в MIDI monitor, ждём ответ SY99…");
        return msg;
    }

    inline juce::String dumpRetryVariantB() noexcept
    {
        return tr (u8"Нет ответа на вариант A — повтор dump request вариант B…");
    }
}

/*
  ==============================================================================

    YamahaLmVoiceDump.h
    Parse SY99 single-voice bulk blocks LM 8101VC / 0040VC (07:1 Voice dump).

    Offset map: _agent_context/lm_8101_offsets.md
    Fixtures:  _agent_context/fixtures/01..03_*_07x1_voice.syx

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <cstring>

namespace YamahaLmVoiceDump
{
    constexpr int kLmTagOffsetFromLm = 4;   // "8101VC" / "0040VC" at LM+4
    constexpr int kLm8101VcNameOffset = 33;
    constexpr int kLm8101VcNameLength = 10;

    /** ELMODE in LM 8101VC header: same raw 0…10 as parameter change `02 00 00 00` (fixtures 01–03 @+32). */
    constexpr int kLm8101VcElmodeOffset = 32;

    /** Typical WOL / ELVL E1 when `02 00` anchor is at 71 (fixture 01). Fixture 02 is −1 byte. */
    constexpr int kLm8101VcWolOffsetTypical = 94;
    constexpr int kLm8101VcElvlE1OffsetTypical = 97;

    /** OUTSEL El.1 packed byte @ ELVL E1 + 1 (diff 04→05 ANONIM both→G1 @+98). */
    constexpr int kLm8101VcOutselE1OffsetTypical = 98;

    constexpr int kMin8101VcFrameSize = 100;
    constexpr int kMin0040VcFrameSize = 100;

    /** 0040VC group-02 tail offsets (lm_8101_offsets.md). */
    constexpr int kLm0040WpbrOffset = 41;
    constexpr int kLm0040AtpbrOffset = 42;
    constexpr int kLm0040PmrngOffset = 40;
    constexpr int kLm0040PmasnOffset = 44;
    constexpr int kLm0040AmasnOffset = 48;
    constexpr int kLm0040AmrngOffset = 50;
    constexpr int kLm0040FmasnOffset = 47;
    constexpr int kLm0040FmrngOffset = 48;
    constexpr int kLm0040PnlasnOffset = 49;
    constexpr int kLm0040PnlrngOffset = 50;
    constexpr int kLm0040CoasnOffset = 51;
    constexpr int kLm0040CorngOffset = 52;
    constexpr int kLm0040PnbasnOffset = 53;
    constexpr int kLm0040PnbrngOffset = 54;
    constexpr int kLm0040EgbasnOffset = 46;
    constexpr int kLm0040EgbrngOffset = 54;
    constexpr int kLm0040WlasnOffset = 56;
    constexpr int kLm0040WllmlOffset = 55;
    constexpr int kLm0040MctunOffset = 90;
    constexpr int kLm0040RndpOffset = 52;
    constexpr int kLm0040AftmdOffset = 100;
    constexpr int kLm0040SptpntOffset = 98;

    inline int uiFromElementNoteShiftSysex (int sysexVal) noexcept
    {
        return juce::jlimit (-64, 63, juce::jlimit (0, 127, sysexVal) - 64);
    }

    inline int uiFromMixerEffectSendSigned7Sysex (int sysexVal) noexcept
    {
        switch ((uint8) juce::jlimit (0, 127, sysexVal))
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
            default:   return 0;
        }
    }

    inline int uiFromVoiceCommonAtpbrSysex (int sysexVal) noexcept
    {
        sysexVal &= 0x7f;
        if (sysexVal == 0)
            return 0;
        if (sysexVal >= 1 && sysexVal <= 0x0c)
            return sysexVal;
        if (sysexVal >= 0x11 && sysexVal <= 0x1c)
            return 0x10 - sysexVal;
        return sysexVal;
    }

    inline int uiFromVoiceCommonAftmdSysex (int sysexVal) noexcept
    {
        return juce::jlimit (0, 4, sysexVal);
    }

    inline int efln1ElOffsetFromElvlWol (int elvlOff, int wolOff) noexcept
    {
        if (wolOff == 93)
            return elvlOff + 37;
        if (wolOff == 95)
            return elvlOff + 35;
        return elvlOff + 36;
    }

    inline int findFirstMixerAnchor (const uint8* frame, int frameSize) noexcept
    {
        if (frame == nullptr)
            return -1;

        const int searchEnd = juce::jmin (frameSize - 7, 125);

        for (int a = 90; a <= searchEnd; ++a)
        {
            if (frame[a] == 0x7f && frame[a + 1] == 0x01 && frame[a + 2] == 0x7f)
                return a;
        }

        return -1;
    }

    inline int findNextMixerAnchor (const uint8* frame, int frameSize, int afterAnchor) noexcept
    {
        if (frame == nullptr || afterAnchor < 0)
            return -1;

        const int searchStart = afterAnchor + 1;
        const int searchEnd = juce::jmin (frameSize - 7, 125);

        for (int a = searchStart; a <= searchEnd; ++a)
        {
            if (frame[a] == 0x7f && frame[a + 1] == 0x01 && frame[a + 2] == 0x7f)
                return a;
        }

        return -1;
    }

    /** Anchor strips for ELNS E2..E4 (E1 uses outsel strip, not anchors). */
    inline int maxElnsAnchorSlotsFromElmodeRaw (int elmodeRaw) noexcept
    {
        switch (elmodeRaw)
        {
            case 2:  // AFMMono4
            case 7:  // AWMPoly4
            case 9:  // AFM2AWM2
                return 3;
            case 1:  // AFMMono2
            case 4:  // AFMPoly2
            case 6:  // AWMPoly2
            case 8:  // AFM1AWM1
                return 1;
            default:
                return 0;
        }
    }

    struct LmBlockView
    {
        const uint8* data = nullptr;
        int size = 0;
    };

    struct Lm8101VcMinimal
    {
        char name[11] {};
        int elmodeRaw = -1;
        int wolRaw = -1;
        int elvlE1Raw = -1;
        int lmOutselE1Raw = -1;
        int elvlRaw[4] { -1, -1, -1, -1 };
        int lmOutselRaw[4] { -1, -1, -1, -1 };
        int lmEldtRaw[4] { -1, -1, -1, -1 };
        int lmElnsRaw[4] { -1, -1, -1, -1 };
        int lmEvllRaw[4] { -1, -1, -1, -1 };
        int lmEvlhRaw[4] { -1, -1, -1, -1 };
        int lmEfln1ElRaw = -1;
        int lmEfsdlvRaw = -1;
        int lmEfsdvlRaw = -1;
        int elmodeOffset = -1;
        int wolOffset = -1;
        int elvlE1Offset = -1;
        int lmOutselE1Offset = -1;
        int elvlOffset[4] { -1, -1, -1, -1 };
        bool found8101 = false;
    };

    struct Lm0040VcMinimal
    {
        bool found0040 = false;
        int wpbrRaw = -1;
        int atpbrRaw = -1;
        int pmasnRaw = -1;
        int pmrngRaw = -1;
        int amasnRaw = -1;
        int amrngRaw = -1;
        int fmasnRaw = -1;
        int fmrngRaw = -1;
        int pnlasnRaw = -1;
        int pnlrngRaw = -1;
        int coasnRaw = -1;
        int corngRaw = -1;
        int pnbasnRaw = -1;
        int pnbrngRaw = -1;
        int egbasnRaw = -1;
        int egbrngRaw = -1;
        int wlasnRaw = -1;
        int wllmlRaw = -1;
        int mctunRaw = -1;
        int rndpRaw = -1;
        int aftmdRaw = -1;
        int sptpntRaw = -1;
    };

    inline bool tagsEqual6 (const uint8* at, const char* tag6) noexcept
    {
        for (int i = 0; i < 6; ++i)
            if (at[i] != (uint8) tag6[i])
                return false;

        return true;
    }

    /** Scan [data, data+dataSize) for `LM` + tag6 (e.g. "8101VC"). Returns view of full F0…F7 frame. */
    inline LmBlockView findLmBlock (const void* data, size_t dataSize, const char* tag6) noexcept
    {
        LmBlockView out;
        const auto* bytes = static_cast<const uint8*> (data);
        const size_t tagLen = 6;

        if (bytes == nullptr || dataSize < 16 || tag6 == nullptr)
            return out;

        for (size_t i = 0; i + 10 < dataSize; ++i)
        {
            if (bytes[i] != 'L' || bytes[i + 1] != 'M')
                continue;

            if (! tagsEqual6 (bytes + i + kLmTagOffsetFromLm, tag6))
                continue;

            size_t start = i;

            while (start > 0 && bytes[start] != 0xf0)
                --start;

            if (bytes[start] != 0xf0)
                continue;

            size_t end = start;

            while (end < dataSize && bytes[end] != 0xf7)
                ++end;

            if (end >= dataSize || bytes[end] != 0xf7)
                continue;

            out.data = bytes + start;
            out.size = (int) (end - start + 1);
            return out;
        }

        return out;
    }

    /** LiveRead capture: prefer full MIDI bytes (F0…F7); getSysExData() is body-only (starts 0x43). */
    inline LmBlockView findLmBlockInSysexMessages (const juce::Array<juce::MidiMessage>& messages,
                                                   const char* tag6) noexcept
    {
        for (const auto& m : messages)
        {
            if (! m.isSysEx())
                continue;

            const uint8* raw = m.getRawData();
            const int rawN = m.getRawDataSize();

            if (raw != nullptr && rawN > 0)
            {
                const auto block = findLmBlock (raw, (size_t) rawN, tag6);

                if (block.data != nullptr)
                    return block;
            }

            const auto* d = m.getSysExData();
            const int n = m.getSysExDataSize();

            if (d != nullptr && n > 0 && d[0] == 0xf0)
            {
                const auto block = findLmBlock (d, (size_t) n, tag6);

                if (block.data != nullptr)
                    return block;
            }
        }

        return {};
    }

    inline void copyVoiceName (const uint8* frame, int frameSize, char* dest, int destSize) noexcept
    {
        if (dest == nullptr || destSize <= 0)
            return;

        std::memset (dest, 0, (size_t) destSize);

        if (frame == nullptr || frameSize < kLm8101VcNameOffset + kLm8101VcNameLength)
            return;

        int n = 0;

        for (int i = 0; i < kLm8101VcNameLength && n < destSize - 1; ++i)
        {
            const uint8 c = frame[kLm8101VcNameOffset + i];
            dest[n++] = (char) ((c >= 0x20 && c < 0x7f) ? c : ' ');
        }

        while (n > 0 && dest[n - 1] == ' ')
            --n;

        dest[n] = '\0';
    }

    /** Packed WOL / ELVL E1: prefix `03` or `01` @ i−1, WOL @ i, `00 00` @ i+1…i+2, ELVL E1 @ i+3.
        07:1 fixtures use `03` @ 93, WOL @ 94; 05:64 short 8101 often uses `01` @ 94, WOL @ 95. */
    inline bool findWolElvlE1Pair (const uint8* frame, int frameSize, int& wolOff, int& elvlOff) noexcept
    {
        wolOff = -1;
        elvlOff = -1;

        if (frame == nullptr || frameSize < 48)
            return false;

        const int searchEnd = juce::jmin (frameSize - 4, 130);
        constexpr int kCanonicalWolMin = 93;
        constexpr int kCanonicalWolMax = 96;

        int bestCanonWol = -1;
        int bestCanonElvl = -1;
        int bestCanonScore = 9999;
        int firstWol = -1;
        int firstElvl = -1;

        for (int i = 44; i <= searchEnd; ++i)
        {
            const uint8 prefix = frame[i - 1];

            if (prefix != 0x03 && prefix != 0x01)
                continue;

            if (frame[i + 1] != 0x00 || frame[i + 2] != 0x00)
                continue;

            const uint8 wol = frame[i];
            const uint8 elvl = frame[i + 3];

            if (wol > 127 || elvl > 127)
                continue;

            if (firstWol < 0)
            {
                firstWol = i;
                firstElvl = i + 3;
            }

            if (i >= kCanonicalWolMin && i <= kCanonicalWolMax)
            {
                const int score = (prefix == 0x03 ? 0 : 10) + (kCanonicalWolMax - i);

                if (score < bestCanonScore)
                {
                    bestCanonScore = score;
                    bestCanonWol = i;
                    bestCanonElvl = i + 3;
                }
            }
        }

        if (bestCanonWol >= 0)
        {
            wolOff = bestCanonWol;
            elvlOff = bestCanonElvl;
            return true;
        }

        if (firstWol >= 0)
        {
            wolOff = firstWol;
            elvlOff = firstElvl;
            return true;
        }

        return false;
    }

    /** Elements 2–4: anchor `7F 01 7F` in packed mixer region; ELVL @ anchor+5 (fixtures 01–03, 05:64 short).
        Uses only the first `7F 01 7F` anchor (second blocks are false positives on 2-element voices). */
    inline int maxElvlAnchorSlotsFromElmodeRaw (int elmodeRaw) noexcept
    {
        switch (elmodeRaw)
        {
            case 1:  // AFMMono2
            case 2:  // AFMMono4
            case 4:  // AFMPoly2
            case 6:  // AWMPoly2
            case 7:  // AWMPoly4
            case 8:  // AFM1AWM1
            case 9:  // AFM2AWM2
                return 1;
            default:
                return 0;
        }
    }

    inline void findPackedElvlFromAnchors (const uint8* frame, int frameSize,
                                           int elvlRaw[4], int elvlOffset[4],
                                           int firstElementIndex, int elmodeRaw) noexcept
    {
        if (frame == nullptr || elvlRaw == nullptr || elvlOffset == nullptr || firstElementIndex < 1)
            return;

        if (maxElvlAnchorSlotsFromElmodeRaw (elmodeRaw) <= 0)
            return;

        const int searchEnd = juce::jmin (frameSize - 6, 125);

        for (int a = 98; a <= searchEnd; ++a)
        {
            if (frame[a] != 0x7f || frame[a + 1] != 0x01 || frame[a + 2] != 0x7f)
                continue;

            const int elIdx = firstElementIndex;

            if (elIdx < 4)
            {
                const uint8 v = frame[a + 5];

                if (v <= 127)
                {
                    elvlRaw[elIdx] = (int) v;
                    elvlOffset[elIdx] = a + 5;
                }
            }

            break;
        }
    }

    inline void parseMixerTail8101 (const uint8* frame, int frameSize, Lm8101VcMinimal& out) noexcept
    {
        if (frame == nullptr || ! out.found8101)
            return;

        const int outselOff = out.elvlE1Offset >= 0 ? out.elvlE1Offset + 1 : -1;

        if (outselOff > 0 && outselOff + 4 < frameSize)
        {
            out.lmElnsRaw[0] = uiFromElementNoteShiftSysex ((int) frame[outselOff + 1]);
            out.lmEvlhRaw[0] = (int) frame[outselOff + 3];
            out.lmEvllRaw[0] = (int) frame[outselOff + 4];

            if (out.elmodeRaw == 4)
                out.lmEldtRaw[0] = (int) frame[outselOff];
        }

        if (out.elvlE1Offset >= 0 && out.wolOffset >= 0)
        {
            const int eflnOff = efln1ElOffsetFromElvlWol (out.elvlE1Offset, out.wolOffset);

            if (eflnOff >= 0 && eflnOff < frameSize)
                out.lmEfln1ElRaw = (int) frame[eflnOff];
        }

        const int maxElnsAnchors = maxElnsAnchorSlotsFromElmodeRaw (out.elmodeRaw);

        if (maxElnsAnchors > 0)
        {
            int anchor = findFirstMixerAnchor (frame, frameSize);

            for (int anchorSlot = 0; anchorSlot < maxElnsAnchors && anchor >= 0; ++anchorSlot)
            {
                if (anchor + 10 < frameSize)
                {
                    const int elIdx = 1 + anchorSlot;

                    if (elIdx < 4)
                    {
                        out.lmOutselRaw[elIdx] = (int) (frame[anchor + 4] & 0x06);
                        out.lmElnsRaw[elIdx] = uiFromElementNoteShiftSysex ((int) frame[anchor + 7]);
                        out.lmEvlhRaw[elIdx] = (int) frame[anchor + 9];
                        out.lmEvllRaw[elIdx] = (int) frame[anchor + 10];
                    }

                    if (anchorSlot == 0)
                    {
                        if (out.elvlRaw[1] < 0)
                        {
                            out.elvlRaw[1] = (int) frame[anchor + 5];
                            out.elvlOffset[1] = anchor + 5;
                        }

                        if (out.elmodeRaw == 8)
                            out.lmEldtRaw[1] = (int) frame[anchor + 6];
                    }
                }

                if (anchorSlot + 1 < maxElnsAnchors)
                    anchor = findNextMixerAnchor (frame, frameSize, anchor);
            }
        }
    }

    inline bool readByteIfInRange (const uint8* frame, int frameSize, int off, int& dest) noexcept
    {
        if (frame == nullptr || off < 0 || off >= frameSize)
            return false;

        dest = (int) frame[off];
        return true;
    }

    inline bool parseLm0040VcMinimal (const uint8* frame, int frameSize, Lm0040VcMinimal& out) noexcept
    {
        out = {};

        if (frame == nullptr || frameSize < kMin0040VcFrameSize)
            return false;

        if (frame[0] != 0xf0 || frame[frameSize - 1] != 0xf7)
            return false;

        if (frameSize < 16 || ! tagsEqual6 (frame + 10, "0040VC"))
            return false;

        out.found0040 = true;

        readByteIfInRange (frame, frameSize, kLm0040WpbrOffset, out.wpbrRaw);
        readByteIfInRange (frame, frameSize, kLm0040AtpbrOffset, out.atpbrRaw);
        readByteIfInRange (frame, frameSize, kLm0040PmrngOffset, out.pmrngRaw);
        readByteIfInRange (frame, frameSize, kLm0040PmasnOffset, out.pmasnRaw);
        readByteIfInRange (frame, frameSize, kLm0040AmasnOffset, out.amasnRaw);
        readByteIfInRange (frame, frameSize, kLm0040AmrngOffset, out.amrngRaw);
        readByteIfInRange (frame, frameSize, kLm0040FmasnOffset, out.fmasnRaw);
        readByteIfInRange (frame, frameSize, kLm0040FmrngOffset, out.fmrngRaw);
        readByteIfInRange (frame, frameSize, kLm0040PnlasnOffset, out.pnlasnRaw);
        readByteIfInRange (frame, frameSize, kLm0040PnlrngOffset, out.pnlrngRaw);
        readByteIfInRange (frame, frameSize, kLm0040CoasnOffset, out.coasnRaw);
        readByteIfInRange (frame, frameSize, kLm0040CorngOffset, out.corngRaw);
        readByteIfInRange (frame, frameSize, kLm0040PnbasnOffset, out.pnbasnRaw);
        readByteIfInRange (frame, frameSize, kLm0040PnbrngOffset, out.pnbrngRaw);
        readByteIfInRange (frame, frameSize, kLm0040EgbasnOffset, out.egbasnRaw);
        readByteIfInRange (frame, frameSize, kLm0040EgbrngOffset, out.egbrngRaw);
        readByteIfInRange (frame, frameSize, kLm0040WlasnOffset, out.wlasnRaw);
        readByteIfInRange (frame, frameSize, kLm0040WllmlOffset, out.wllmlRaw);
        readByteIfInRange (frame, frameSize, kLm0040MctunOffset, out.mctunRaw);
        readByteIfInRange (frame, frameSize, kLm0040RndpOffset, out.rndpRaw);
        readByteIfInRange (frame, frameSize, kLm0040AftmdOffset, out.aftmdRaw);
        readByteIfInRange (frame, frameSize, kLm0040SptpntOffset, out.sptpntRaw);

        return true;
    }

    inline bool parseLm0040VcMinimal (const void* data, size_t dataSize, Lm0040VcMinimal& out) noexcept
    {
        const auto block = findLmBlock (data, dataSize, "0040VC");
        return block.data != nullptr && parseLm0040VcMinimal (block.data, block.size, out);
    }

    inline bool parseLm0040VcMinimalFromSysexMessages (const juce::Array<juce::MidiMessage>& messages,
                                                       Lm0040VcMinimal& out) noexcept
    {
        juce::MemoryBlock wrapped;

        for (const auto& m : messages)
        {
            if (! m.isSysEx())
                continue;

            const uint8* raw = m.getRawData();
            const int rawN = m.getRawDataSize();

            if (raw != nullptr && rawN > 0 && raw[0] == 0xf0)
            {
                if (parseLm0040VcMinimal (raw, rawN, out))
                    return true;

                continue;
            }

            const uint8* d = m.getSysExData();
            const int n = m.getSysExDataSize();

            if (d == nullptr || n <= 0)
                continue;

            if (d[0] == 0xf0)
            {
                if (parseLm0040VcMinimal (d, n, out))
                    return true;

                continue;
            }

            if (d[0] != 0x43)
                continue;

            wrapped.setSize ((size_t) n + 2, true);
            auto* w = static_cast<uint8*> (wrapped.getData());
            w[0] = 0xf0;
            std::memcpy (w + 1, d, (size_t) n);
            w[n + 1] = 0xf7;

            if (parseLm0040VcMinimal (w, n + 2, out))
                return true;
        }

        return false;
    }
    inline int findElmodeOffsetLegacyPacked (const uint8* frame, int frameSize) noexcept
    {
        if (frame == nullptr || frameSize < 76)
            return -1;

        const int searchEnd = juce::jmin (frameSize - 3, 95);

        for (int i = 43; i <= searchEnd; ++i)
        {
            if (frame[i] == 0x02 && frame[i + 1] == 0x00)
                return i + 2;
        }

        return -1;
    }

    inline bool parseLm8101VcMinimal (const uint8* frame, int frameSize, Lm8101VcMinimal& out) noexcept
    {
        out = {};

        if (frame == nullptr || frameSize < kMin8101VcFrameSize)
            return false;

        if (frame[0] != 0xf0 || frame[frameSize - 1] != 0xf7)
            return false;

        if (frameSize < 16 || ! tagsEqual6 (frame + 10, "8101VC"))
            return false;

        out.found8101 = true;
        copyVoiceName (frame, frameSize, out.name, (int) sizeof (out.name));

        if (frameSize > kLm8101VcElmodeOffset)
        {
            const uint8 em = frame[kLm8101VcElmodeOffset];

            if (em <= 10)
            {
                out.elmodeOffset = kLm8101VcElmodeOffset;
                out.elmodeRaw = (int) em;
            }
        }

        if (out.elmodeRaw < 0)
        {
            out.elmodeOffset = findElmodeOffsetLegacyPacked (frame, frameSize);

            if (out.elmodeOffset >= 0 && out.elmodeOffset < frameSize)
                out.elmodeRaw = (int) frame[out.elmodeOffset];
        }

        if (findWolElvlE1Pair (frame, frameSize, out.wolOffset, out.elvlE1Offset))
        {
            out.wolRaw = (int) frame[out.wolOffset];
            out.elvlE1Raw = (int) frame[out.elvlE1Offset];
        }
        else if (frameSize > kLm8101VcElvlE1OffsetTypical + 1)
        {
            const uint8 tagAt94 = frame[kLm8101VcWolOffsetTypical];

            if ((tagAt94 == 0x01 || tagAt94 == 0x03)
                && frame[kLm8101VcWolOffsetTypical + 2] == 0x00
                && frame[kLm8101VcElvlE1OffsetTypical] == 0x00
                && frame[kLm8101VcWolOffsetTypical + 1] <= 127)
            {
                out.wolOffset = kLm8101VcWolOffsetTypical + 1;
                out.elvlE1Offset = kLm8101VcWolOffsetTypical + 4;
            }
            else
            {
                out.wolOffset = kLm8101VcWolOffsetTypical;
                out.elvlE1Offset = kLm8101VcElvlE1OffsetTypical;
            }

            out.wolRaw = (int) frame[out.wolOffset];
            out.elvlE1Raw = (int) frame[out.elvlE1Offset];
        }

        if (out.elvlE1Raw >= 0)
        {
            out.elvlRaw[0] = out.elvlE1Raw;
            out.elvlOffset[0] = out.elvlE1Offset;
        }

        findPackedElvlFromAnchors (frame, frameSize, out.elvlRaw, out.elvlOffset, 1, out.elmodeRaw);

        if (out.elvlE1Offset >= 0)
        {
            const int outselOff = out.elvlE1Offset + 1;

            if (outselOff < frameSize)
            {
                const uint8 packed = frame[outselOff];
                out.lmOutselE1Offset = outselOff;
                out.lmOutselE1Raw = (int) (packed & 0x06);
            }
        }
        else if (frameSize > kLm8101VcOutselE1OffsetTypical)
        {
            const uint8 packed = frame[kLm8101VcOutselE1OffsetTypical];
            out.lmOutselE1Offset = kLm8101VcOutselE1OffsetTypical;
            out.lmOutselE1Raw = (int) (packed & 0x06);
        }

        parseMixerTail8101 (frame, frameSize, out);

        return true;
    }

    inline bool parseLm8101VcMinimal (const void* data, size_t dataSize, Lm8101VcMinimal& out) noexcept
    {
        const auto block = findLmBlock (data, dataSize, "8101VC");
        return block.data != nullptr && parseLm8101VcMinimal (block.data, block.size, out);
    }

    inline bool parseLm8101VcMinimalFromSysexMessages (const juce::Array<juce::MidiMessage>& messages,
                                                       Lm8101VcMinimal& out) noexcept
    {
        juce::MemoryBlock wrapped;

        for (const auto& m : messages)
        {
            if (! m.isSysEx())
                continue;

            const uint8* raw = m.getRawData();
            const int rawN = m.getRawDataSize();

            if (raw != nullptr && rawN > 0 && raw[0] == 0xf0)
            {
                if (parseLm8101VcMinimal (raw, rawN, out))
                    return true;

                continue;
            }

            const uint8* d = m.getSysExData();
            const int n = m.getSysExDataSize();

            if (d == nullptr || n <= 0)
                continue;

            if (d[0] == 0xf0)
            {
                if (parseLm8101VcMinimal (d, n, out))
                    return true;

                continue;
            }

            if (d[0] != 0x43)
                continue;

            wrapped.setSize ((size_t) n + 2, true);
            auto* w = static_cast<uint8*> (wrapped.getData());
            w[0] = 0xf0;
            std::memcpy (w + 1, d, (size_t) n);
            w[n + 1] = 0xf7;

            if (parseLm8101VcMinimal (w, n + 2, out))
                return true;
        }

        return false;
    }

    inline juce::String formatLm8101ElnsCoverageLine (const Lm8101VcMinimal& p) noexcept
    {
        juce::String s = "ELNS";

        for (int e = 0; e < 4; ++e)
        {
            if (p.lmElnsRaw[e] >= -64)
                s << " E" << (e + 1) << "=" << p.lmElnsRaw[e];
            else
                s << " E" << (e + 1) << "=--";
        }

        return s;
    }

    inline juce::String formatLm8101MixerTailCoverageLine (const Lm8101VcMinimal& p) noexcept
    {
        juce::String s = "MIXER";

        if (p.lmOutselE1Raw >= 0)
            s << " OUTSEL_E1=0x" << juce::String::toHexString (p.lmOutselE1Raw).toUpperCase();
        else
            s << " OUTSEL_E1=--";

        for (int e = 1; e < 4; ++e)
        {
            if (p.lmOutselRaw[e] >= 0)
                s << " OUTSEL_E" << (e + 1) << "=0x" << juce::String::toHexString (p.lmOutselRaw[e]).toUpperCase();
            else
                s << " OUTSEL_E" << (e + 1) << "=--";
        }

        for (int e = 0; e < 4; ++e)
        {
            if (p.lmEvllRaw[e] >= 0)
                s << " EVLL_E" << (e + 1) << "=" << p.lmEvllRaw[e];
            else
                s << " EVLL_E" << (e + 1) << "=--";

            if (p.lmEvlhRaw[e] >= 0)
                s << " EVLH_E" << (e + 1) << "=" << p.lmEvlhRaw[e];
            else
                s << " EVLH_E" << (e + 1) << "=--";
        }

        return s;
    }

    inline juce::String formatLm8101VcMinimalLogLine (const Lm8101VcMinimal& p) noexcept
    {
        if (! p.found8101)
            return "LM8101=missing";

        juce::String s;
        s << "name=\"" << juce::String (p.name).trimEnd() << "\"";

        if (p.elmodeRaw >= 0)
            s << " ELMODE=" << p.elmodeRaw << "@+" << p.elmodeOffset;

        if (p.wolRaw >= 0)
            s << " WOL=" << p.wolRaw << "@+" << p.wolOffset;

        if (p.elvlE1Raw >= 0)
            s << " ELVL_E1=" << p.elvlE1Raw << "@+" << p.elvlE1Offset;

        if (p.lmOutselE1Raw >= 0)
            s << " OUTSEL_E1=0x" << juce::String::toHexString (p.lmOutselE1Raw).toUpperCase()
              << "@+" << p.lmOutselE1Offset;

        for (int e = 1; e < 4; ++e)
            if (p.elvlRaw[e] >= 0)
                s << " ELVL_E" << (e + 1) << "=" << p.elvlRaw[e] << "@+" << p.elvlOffset[e];

        for (int e = 0; e < 4; ++e)
            if (p.lmElnsRaw[e] >= -64)
                s << " ELNS_E" << (e + 1) << "=" << p.lmElnsRaw[e];

        for (int e = 1; e < 4; ++e)
            if (p.lmOutselRaw[e] >= 0)
                s << " OUTSEL_E" << (e + 1) << "=0x" << juce::String::toHexString (p.lmOutselRaw[e]).toUpperCase();

        for (int e = 0; e < 4; ++e)
        {
            if (p.lmEvllRaw[e] >= 0)
                s << " EVLL_E" << (e + 1) << "=" << p.lmEvllRaw[e];

            if (p.lmEvlhRaw[e] >= 0)
                s << " EVLH_E" << (e + 1) << "=" << p.lmEvlhRaw[e];
        }

        for (int e = 0; e < 2; ++e)
            if (p.lmEldtRaw[e] >= 0)
                s << " ELDT_E" << (e + 1) << "=" << p.lmEldtRaw[e];

        if (p.lmEfln1ElRaw >= 0)
            s << " EFLN1EL=" << p.lmEfln1ElRaw;

        return s;
    }

    inline juce::String formatLm0040VcMinimalLogLine (const Lm0040VcMinimal& p) noexcept
    {
        if (! p.found0040)
            return "LM0040=missing";

        juce::String s = "LM0040";

        if (p.wpbrRaw >= 0)
            s << " WPBR=" << p.wpbrRaw;

        if (p.wllmlRaw >= 0)
            s << " WLLML=" << p.wllmlRaw;

        if (p.sptpntRaw >= 0)
            s << " SPTPNT=" << p.sptpntRaw;

        if (p.atpbrRaw >= 0)
            s << " ATPBR=" << p.atpbrRaw;

        if (p.aftmdRaw >= 0)
            s << " AFTMD=" << p.aftmdRaw;

        return s;
    }

    inline bool frameContainsLmTag (const uint8* frame, int frameSize, const char* tag6) noexcept
    {
        return findLmBlock (frame, (size_t) frameSize, tag6).data != nullptr;
    }

    /** SY99 voice dumps pair 8101VC with a following 0040VC frame in the same .syx. */
    inline bool findPaired0040FrameAfter (const uint8* fileData, size_t fileSize,
                                          int afterFrameEnd, int& outOffset, int& outLength) noexcept
    {
        outOffset = -1;
        outLength = 0;

        if (fileData == nullptr || fileSize < 16 || afterFrameEnd < 0)
            return false;

        size_t i = (size_t) afterFrameEnd;

        while (i < fileSize)
        {
            if (fileData[i] != 0xf0)
            {
                ++i;
                continue;
            }

            size_t j = i + 1;

            while (j < fileSize && fileData[j] != 0xf7)
                ++j;

            if (j >= fileSize)
                break;

            const int frameLen = (int) (j - i + 1);

            if (frameContainsLmTag (fileData + (size_t) i, frameLen, "0040VC"))
            {
                outOffset = (int) i;
                outLength = frameLen;
                return true;
            }

            if (frameContainsLmTag (fileData + (size_t) i, frameLen, "8101VC"))
                break;

            i = j + 1;
        }

        return false;
    }
}

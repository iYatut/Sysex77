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
    enum class Lm8101ParseMode : std::uint8_t
    {
        ingest,
        auditDebug
    };

    inline bool lm8101ParserVerboseLoggingEnabled() noexcept
    {
        static const bool verbose = juce::SystemStats::getEnvironmentVariable ("SY99_PARSER_VERBOSE", {})
                                        .equalsIgnoreCase ("1");
        return verbose;
    }
    constexpr int kLmTagOffsetFromLm = 4;   // "8101VC" / "0040VC" at LM+4
    constexpr int kLm8101VcNameOffset = 33;
    constexpr int kLm8101MuNameOffset = 32;
    constexpr int kLm8101VcNameLength = 10;

    /** ELMODE in LM 8101VC header: same raw 0…10 as parameter change `02 00 00 00` (fixtures 01–03 @+32). */
    constexpr int kLm8101VcElmodeOffset = 32;

    /** Typical WOL / ELVL E1 when `02 00` anchor is at 71 (fixture 01). Fixture 02 is −1 byte. */
    constexpr int kLm8101VcWolOffsetTypical = 94;
    constexpr int kLm8101VcElvlE1OffsetTypical = 97;

    /** OUTSEL El.1 packed byte @ ELVL E1 + 1 (diff 04→05 ANONIM both→G1 @+98). */
    constexpr int kLm8101VcOutselE1OffsetTypical = 98;

    constexpr int kMin8101VcFrameSize = 100;
    constexpr int kMin0040VcFrameSize = 105;

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
    /** Filter Cutoff Range (NN 0x33) — sequential slot @+44, not @+52 (RNDP). */
    constexpr int kLm0040CorngOffset = 44;
    constexpr int kLm0040PnbasnOffset = 53;
    constexpr int kLm0040PnbrngOffset = 54;
    constexpr int kLm0040EgbasnOffset = 46;
    constexpr int kLm0040EgbrngOffset = 54;
    constexpr int kLm0040WlasnOffset = 56;
    constexpr int kLm0040WllmlOffset = 55;
    constexpr int kLm0040MctunOffset = 90;
    /** Random Pitch (NN 0x3B) — sequential slot @+52 (fixtures 01–03). */
    constexpr int kLm0040RndpOffset = 52;
    /** EFSDLV El.1 in 0040VC tail (elmode 8 hardware diff; was mis-tagged AFTMD @+100). */
    constexpr int kLm0040EfsdlvE1Offset = 100;
    /** EFLN1EL wire El.1 in 0040VC — byte before EFSDLV E1 (HW Classic 2026-05-26). */
    constexpr int kLm0040EflnE1Offset = kLm0040EfsdlvE1Offset - 1;
    /** EFSDLV El.2 in 0040VC tail (elmode 8 hardware diff). */
    constexpr int kLm0040EfsdlvE2Offset = 104;
    /** EFLN1EL wire El.2 in 0040VC — byte before EFSDLV E2. */
    constexpr int kLm0040EflnE2Offset = kLm0040EfsdlvE2Offset - 1;
    constexpr int kLm0040SptpntOffset = 98;
    /** Effect mode (live `08 00 00 20`): Off=0 Serial=1 Parallel=2. Confirmed fixtures 06–08 @+33. */
    constexpr int kLm0040EfmodeOffset = 33;

    inline int uiFromElementNoteShiftSysex (int sysexVal) noexcept
    {
        return juce::jlimit (-64, 63, juce::jlimit (0, 127, sysexVal) - 64);
    }

    inline int uiFromMixerEffectSendSigned7Sysex (int sysexVal) noexcept
    {
        const int raw = juce::jlimit (0, 127, sysexVal);

        if (raw >= 0x79 && raw <= 0x7F)
            return raw - 0x80;

        switch ((uint8) raw)
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

    inline int sysexFromVoiceCommonAtpbrUi (int uiVal) noexcept
    {
        uiVal = juce::jlimit (-12, 12, uiVal);

        if (uiVal == 0)
            return 0;

        if (uiVal > 0)
            return uiVal;

        return 0x1C + (-12 - uiVal);
    }

    inline int uiFromElementDetuneSysex (int sysexVal) noexcept
    {
        int raw = sysexVal & 0x7f;
        constexpr int delta = 9;

        if (raw > delta - 2)
        {
            raw -= delta;
            raw = ~raw;
        }

        return juce::jlimit (-7, 7, raw);
    }

    inline int sysexFromElementDetuneUi (int uiVal) noexcept
    {
        uiVal = juce::jlimit (-7, 7, uiVal);

        if (uiVal >= 0)
            return uiVal;

        return (~uiVal) + 9;
    }

    inline int sysexFromElementNoteShiftUi (int uiVal) noexcept
    {
        return juce::jlimit (0, 127, juce::jlimit (-64, 63, uiVal) + 64);
    }

    inline int sysexFromMixerEffectSendSigned7Ui (int uiVal) noexcept
    {
        uiVal = juce::jlimit (-7, 7, uiVal);

        if (uiVal < 0)
            return 0x80 + uiVal;

        return uiVal;
    }

    /** EFLN1EL wire bits: Send 1 = 0x01, Send 3 = 0x04 (no Send 2/4 on SY99).
        Bulk often sets bit 0x02 with Send 3 (0x04) meaning Send 1+3 → 0x05 (EP|76Stage HW). */
    inline int eflnWireValueMasked (int raw) noexcept
    {
        if (raw < 0)
            return -1;

        const uint8 b = (uint8) raw;
        int ui = (int) (b & 0x05);

        if ((b & 0x02) != 0)
            ui |= 0x01;

        return ui & 0x05;
    }

    /** EFLN1EL El.1 in 8101VC mixer strip (HW matrix 2026-05-26).
        ELVL E1==0 → +35; ELVL E1==127 → best of +39/+26/+35 wire score. */
    inline int efln1ElRawFromMixerStrip (const uint8* frame, int frameSize,
                                         int elvlOff, int elvlE1Raw) noexcept
    {
        if (frame == nullptr || elvlOff < 0)
            return -1;

        if (elvlE1Raw == 0)
        {
            const int off35 = elvlOff + 35;

            if (off35 >= 0 && off35 < frameSize)
                return eflnWireValueMasked ((int) frame[off35]);

            return -1;
        }

        const int offs[] = { elvlOff + 39, elvlOff + 26, elvlOff + 35 };
        int bestMasked = -1;
        int bestScore = -1;

        for (int off : offs)
        {
            if (off < 0 || off >= frameSize)
                continue;

            const int masked = eflnWireValueMasked ((int) frame[off]);

            if (masked <= 0)
                continue;

            const int score = masked == 5 ? 3 : masked == 4 ? 2 : masked == 1 ? 1 : 0;

            if (score > bestScore)
            {
                bestScore = score;
                bestMasked = masked;
            }
        }

        if (bestMasked >= 0)
            return bestMasked;

        const int off39 = elvlOff + 39;

        if (off39 >= 0 && off39 < frameSize)
            return eflnWireValueMasked ((int) frame[off39]);

        return -1;
    }

    /** 8101 EFSDLV El.1 fallback @ ELVL E1 + 47 (not efln+12 — offset varies with EFLN layout). */
    constexpr int kLm8101VcEfsdlvDeltaFromElvl = 47;

    /** Legacy name kept for grndual diagnostic (was tied to wrong EFLN base). */
    constexpr int kLm8101VcEfsdlvDeltaFromEfln = 12;

    /** Autosync-only fallback when 0040VC is absent; not authoritative (HW EP:NiteHwk 2026-05-24). */
    inline bool efsendBulk8101FallbackForElmodeRaw (int elmodeRaw) noexcept
    {
        return elmodeRaw == 4;
    }

    inline bool efsendBulk8101TrustedForElmodeRaw (int elmodeRaw) noexcept
    {
        return efsendBulk8101FallbackForElmodeRaw (elmodeRaw);
    }

    /** EFSDLV El.1 in 0040VC @100 — elmode 4 (NiteHwk HW) and 8 (GrnDual). */
    inline bool efsendBulk0040E1TrustedForElmodeRaw (int elmodeRaw) noexcept
    {
        return elmodeRaw == 4 || elmodeRaw == 8;
    }

    inline bool efsendBulk0040TrustedForElmodeRaw (int elmodeRaw) noexcept
    {
        return efsendBulk0040E1TrustedForElmodeRaw (elmodeRaw);
    }

    /** EFSDLV El.2 in 0040VC @104 — elmode 4 and 8. */
    inline bool efsendBulk0040E2TrustedForElmodeRaw (int elmodeRaw) noexcept
    {
        return elmodeRaw == 4 || elmodeRaw == 8;
    }

    /** Live param9 `03 NN 00 0A` — Effect Send Level (all elements). */
    inline bool isEfsendlvlLiveParam9Frame (uint8 b3, uint8 b4, uint8 b5, uint8 b6) noexcept
    {
        if (b3 != 0x03 || b5 != 0x00 || b6 != 0x0a)
            return false;

        switch (b4)
        {
            case 0x00:
            case 0x20:
            case 0x40:
            case 0x60:
                return true;
            default:
                return false;
        }
    }

    /** Live param9 `03 NN 00 09` — Effect Send Lines (per element). */
    inline bool isEflnLiveParam9Frame (uint8 b3, uint8 b4, uint8 b5, uint8 b6) noexcept
    {
        if (b3 != 0x03 || b5 != 0x00 || b6 != 0x09)
            return false;

        switch (b4)
        {
            case 0x00:
            case 0x20:
            case 0x40:
            case 0x60:
                return true;
            default:
                return false;
        }
    }

    inline int elementIndexFromMixerLiveParam9B4 (uint8 b4) noexcept
    {
        switch (b4)
        {
            case 0x00: return 0;
            case 0x20: return 1;
            case 0x40: return 2;
            case 0x60: return 3;
            default:   return -1;
        }
    }

    inline bool eldtE1UsesOutselStrip (int elmodeRaw) noexcept
    {
        switch (elmodeRaw)
        {
            case 1:  // AFMMono2
            case 4:  // AFMPoly2
            case 6:  // AWMPoly2
            case 8:  // AFM1AWM1
                return true;
            default:
                return false;
        }
    }

    inline bool eldtE2UsesFirstAnchor (int elmodeRaw) noexcept
    {
        switch (elmodeRaw)
        {
            case 4:  // AFMPoly2
            case 8:  // AFM1AWM1
                return true;
            default:
                return false;
        }
    }

    /** OUTSEL VV (0/2/4/6) from packed mixer byte; bits 1–2 only (bit 0 may carry ELDT when shared). */
    inline int outselRawFromPackedByte8101 (uint8 packed, bool eldtSharesByte) noexcept
    {
        juce::ignoreUnused (eldtSharesByte);
        return (int) (packed & 0x06);
    }

    inline int outselWireValueMasked (int raw) noexcept
    {
        if (raw < 0)
            return -1;

        return (int) ((uint8) raw & 0x06);
    }

    /** Send 3 only valid in EFMODE Parallel (0x02). */
    inline int eflnUiForEffectMode (int maskedRaw, int efmodeRaw) noexcept
    {
        if (maskedRaw < 0)
            return -1;

        int ui = eflnWireValueMasked (maskedRaw);

        if (efmodeRaw >= 0 && efmodeRaw != 2)
            ui &= ~0x04;

        return ui;
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

    /** Audit-only: log first mixer anchor context (does not affect parsing). */
    inline void auditLm8101MixerAnchor (const uint8* frame, int frameSize) noexcept
    {
        const int anchor = findFirstMixerAnchor (frame, frameSize);

        if (anchor < 0)
        {
            juce::Logger::writeToLog ("[8101 anchor audit] no 7F 01 7F anchor (frameSize="
                                      + juce::String (frameSize) + ")");
            return;
        }

        juce::String hexDump;

        for (int i = 0; i < 20 && anchor + i < frameSize; ++i)
        {
            if (i > 0)
                hexDump << ' ';

            hexDump << juce::String::toHexString ((int) frame[anchor + i]).toUpperCase()
                                         .paddedLeft ('0', 2);
        }

        const bool insidePackedNameOrData = anchor < 64;

        int secondaryAfter110 = -1;
        const int searchStart = juce::jmax (111, 90);
        const int searchEnd = juce::jmin (frameSize - 7, 125);

        for (int a = searchStart; a <= searchEnd; ++a)
        {
            if (frame[a] == 0x7f && frame[a + 1] == 0x01 && frame[a + 2] == 0x7f)
            {
                secondaryAfter110 = a;
                break;
            }
        }

        const juce::String secondaryStr = secondaryAfter110 >= 0
                                              ? juce::String (secondaryAfter110)
                                              : juce::String ("none");

        juce::Logger::writeToLog ("[8101 anchor audit] anchorPos=" + juce::String (anchor)
                                  + " insidePackedNameOrData(<64)="
                                  + juce::String ((int) insidePackedNameOrData)
                                  + " hex20=" + hexDump
                                  + " secondaryAnchorAfter110=" + secondaryStr);
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
        int lmEnllRaw[4] { -1, -1, -1, -1 };
        int lmEnlhRaw[4] { -1, -1, -1, -1 };
        int lmEvllRaw[4] { -1, -1, -1, -1 };
        int lmEvlhRaw[4] { -1, -1, -1, -1 };
        int lmEfln1ElRaw = -1;
        int lmEfsdlvRaw = -1;
        int lmEfsdvlRaw = -1;
        int lmEfsdsclRaw = -1;
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
        int sptpntRaw = -1;
        int efmodeRaw = -1;
        int efsdlvE1Raw = -1;
        int efsdlvE2Raw = -1;
        int eflnE1Raw = -1;
        int eflnE2Raw = -1;
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

    inline bool appendWrappedSysexBody (juce::MemoryBlock& out,
                                        const uint8* body, int bodyLen) noexcept
    {
        if (body == nullptr || bodyLen <= 0 || body[0] != 0x43)
            return false;

        out.setSize ((size_t) bodyLen + 2, true);
        auto* w = static_cast<uint8*> (out.getData());
        w[0] = 0xf0;
        std::memcpy (w + 1, body, (size_t) bodyLen);
        w[bodyLen + 1] = 0xf7;
        return true;
    }

    /** LiveRead capture: prefer full MIDI bytes (F0…F7); getSysExData() is body-only (starts 0x43). */
    inline LmBlockView findLmBlockInSysexMessages (const juce::Array<juce::MidiMessage>& messages,
                                                   const char* tag6) noexcept
    {
        juce::MemoryBlock wrapped;

        for (const auto& m : messages)
        {
            if (! m.isSysEx())
                continue;

            const uint8* raw = m.getRawData();
            const int rawN = m.getRawDataSize();

            if (raw != nullptr && rawN > 0)
            {
                if (raw[0] == 0xf0)
                {
                    const auto block = findLmBlock (raw, (size_t) rawN, tag6);

                    if (block.data != nullptr)
                        return block;
                }
                else if (raw[0] == 0x43 && appendWrappedSysexBody (wrapped, raw, rawN))
                {
                    const auto* w = static_cast<const uint8*> (wrapped.getData());
                    const auto block = findLmBlock (w, wrapped.getSize(), tag6);

                    if (block.data != nullptr)
                        return block;
                }
            }

            const auto* d = m.getSysExData();
            const int n = m.getSysExDataSize();

            if (d == nullptr || n <= 0)
                continue;

            if (d[0] == 0xf0)
            {
                const auto block = findLmBlock (d, (size_t) n, tag6);

                if (block.data != nullptr)
                    return block;

                continue;
            }

            if (d[0] == 0x43 && appendWrappedSysexBody (wrapped, d, n))
            {
                const auto* w = static_cast<const uint8*> (wrapped.getData());
                const auto block = findLmBlock (w, wrapped.getSize(), tag6);

                if (block.data != nullptr)
                    return block;
            }
        }

        return {};
    }

    /** Prefer the largest full 0040VC frame (SYM7 RX ~115 B); ignore short echoes / partial tails. */
    inline LmBlockView findBestLm0040VcBlockInSysexMessages (
        const juce::Array<juce::MidiMessage>& messages) noexcept
    {
        LmBlockView best;
        juce::MemoryBlock wrapped;

        for (const auto& m : messages)
        {
            if (! m.isSysEx())
                continue;

            const uint8* raw = m.getRawData();
            const int rawN = m.getRawDataSize();

            auto consider = [&best] (const uint8* data, size_t size) noexcept
            {
                if (data == nullptr || size < (size_t) kMin0040VcFrameSize)
                    return;

                const auto block = findLmBlock (data, size, "0040VC");

                if (block.data != nullptr && block.size > best.size)
                    best = block;
            };

            if (raw != nullptr && rawN > 0)
            {
                if (raw[0] == 0xf0)
                    consider (raw, (size_t) rawN);
                else if (raw[0] == 0x43 && appendWrappedSysexBody (wrapped, raw, rawN))
                    consider (static_cast<const uint8*> (wrapped.getData()), wrapped.getSize());
            }

            const uint8* d = m.getSysExData();
            const int n = m.getSysExDataSize();

            if (d == nullptr || n <= 0)
                continue;

            if (d[0] == 0xf0)
                consider (d, (size_t) n);
            else if (d[0] == 0x43 && appendWrappedSysexBody (wrapped, d, n))
                consider (static_cast<const uint8*> (wrapped.getData()), wrapped.getSize());
        }

        return best;
    }

    /** Concatenate LiveRead SysEx bytes (same layout as saveLiveReadCaptureToSyx .syx). */
    inline juce::MemoryBlock concatLiveReadSysexRaw (
        const juce::Array<juce::MidiMessage>& messages) noexcept
    {
        juce::MemoryBlock out;
        juce::MemoryBlock wrapped;

        for (const auto& m : messages)
        {
            if (! m.isSysEx())
                continue;

            const uint8* raw = m.getRawData();
            const int rawN = m.getRawDataSize();

            if (raw != nullptr && rawN > 0)
            {
                if (raw[0] == 0xf0)
                {
                    out.append (raw, (size_t) rawN);
                    continue;
                }

                if (raw[0] == 0x43 && appendWrappedSysexBody (wrapped, raw, rawN))
                {
                    out.append (wrapped.getData(), wrapped.getSize());
                    continue;
                }

                out.append (raw, (size_t) rawN);
                continue;
            }

            const uint8* d = m.getSysExData();
            const int n = m.getSysExDataSize();

            if (d != nullptr && n > 0 && appendWrappedSysexBody (wrapped, d, n))
                out.append (wrapped.getData(), wrapped.getSize());
        }

        return out;
    }

    inline bool frameContainsLmTag (const uint8* frame, int frameSize, const char* tag6) noexcept
    {
        return findLmBlock (frame, (size_t) frameSize, tag6).data != nullptr;
    }

    /** N-th SysEx frame containing LM tag (0-based), e.g. mm index in AUTOSYNC-0040VC-INT. */
    inline bool findNthLmTagFrameInFileData (const uint8* fileData, size_t fileSize,
                                             const char* tag6, int frameIndex,
                                             int& outOffset, int& outLength) noexcept
    {
        outOffset = -1;
        outLength = 0;

        if (fileData == nullptr || fileSize < 16 || frameIndex < 0 || tag6 == nullptr)
            return false;

        int count = 0;

        for (size_t i = 0; i < fileSize;)
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

            if (frameContainsLmTag (fileData + i, frameLen, tag6))
            {
                if (count == frameIndex)
                {
                    outOffset = (int) i;
                    outLength = frameLen;
                    return true;
                }

                ++count;
            }

            i = j + 1;
        }

        return false;
    }

    inline bool findNth0040VcFrameInFileData (const uint8* fileData, size_t fileSize,
                                              int frameIndex,
                                              int& outOffset, int& outLength) noexcept
    {
        return findNthLmTagFrameInFileData (fileData, fileSize, "0040VC", frameIndex,
                                            outOffset, outLength);
    }

    /** Read SYM7 slot address (byte28, mm) from an LM frame body. */
    inline bool readLmSym7SlotAddressFromFrame (const uint8* frame, int frameSize, const char* tag6,
                                                uint8& byte28Out, uint8& mmOut) noexcept
    {
        byte28Out = 0;
        mmOut = 0;

        if (frame == nullptr || tag6 == nullptr || frameSize < 26)
            return false;

        for (int i = 0; i <= frameSize - 26; ++i)
        {
            if (frame[i] != 'L' || frame[i + 1] != 'M')
                continue;

            if (! tagsEqual6 (frame + i + kLmTagOffsetFromLm, tag6))
                continue;

            const int addr = i + 24;

            if (addr + 1 >= frameSize)
                return false;

            byte28Out = frame[addr];
            mmOut = frame[addr + 1];
            return true;
        }

        return false;
    }

    /** Companion AUTOSYNC-0040VC-INT: match by slot mm (not nth frame — sync may contain duplicates). */
    inline bool find0040VcFrameBySlotMmInFileData (const uint8* fileData, size_t fileSize,
                                                   uint8 byte28, uint8 mm,
                                                   int& outOffset, int& outLength) noexcept
    {
        outOffset = -1;
        outLength = 0;

        if (fileData == nullptr || fileSize < 16)
            return false;

        int bestScore = -1;

        for (size_t i = 0; i < fileSize;)
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
            uint8 frameB28 = 0;
            uint8 frameMm = 0;

            if (readLmSym7SlotAddressFromFrame (fileData + i, frameLen, "0040VC",
                                                frameB28, frameMm)
                && frameB28 == byte28
                && frameMm == mm
                && frameLen >= kMin0040VcFrameSize)
            {
                int score = 1000;

                if (frameLen <= 130)
                    score += 500;

                score -= frameLen;

                if (score > bestScore)
                {
                    bestScore = score;
                    outOffset = (int) i;
                    outLength = frameLen;
                }
            }

            i = j + 1;
        }

        return bestScore >= 0;
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

    inline int lm8101NameOffsetForTag (const char* tag6) noexcept
    {
        if (tag6 != nullptr && std::memcmp (tag6, "8101MU", 6) == 0)
            return kLm8101MuNameOffset;

        return kLm8101VcNameOffset;
    }

    inline void copyLm8101BlockName (const uint8* frame,
                                     int frameSize,
                                     const char* tag6,
                                     char* dest,
                                     int destSize) noexcept
    {
        if (dest == nullptr || destSize <= 0)
            return;

        std::memset (dest, 0, (size_t) destSize);

        const int nameOff = lm8101NameOffsetForTag (tag6);

        if (frame == nullptr || frameSize < nameOff + kLm8101VcNameLength)
            return;

        int n = 0;

        for (int i = 0; i < kLm8101VcNameLength && n < destSize - 1; ++i)
        {
            const uint8 c = frame[nameOff + i];
            dest[n++] = (char) ((c >= 0x20 && c < 0x7f) ? c : ' ');
        }

        while (n > 0 && dest[n - 1] == ' ')
            --n;

        dest[n] = '\0';
    }

    inline void copyVoiceName (const uint8* frame, int frameSize, char* dest, int destSize) noexcept
    {
        copyLm8101BlockName (frame, frameSize, "8101VC", dest, destSize);
    }

    /** Name + ELMODE for library index listing — avoids full parse and Debug audit spam. */
    inline bool readLm8101VcIndexFields (const uint8* frame,
                                         int frameSize,
                                         char* nameOut,
                                         int nameOutSize,
                                         int& elmodeRawOut) noexcept
    {
        elmodeRawOut = -1;

        if (nameOut == nullptr || nameOutSize <= 0)
            return false;

        copyVoiceName (frame, frameSize, nameOut, nameOutSize);

        if (frame != nullptr && frameSize > kLm8101VcElmodeOffset)
        {
            const uint8 em = frame[kLm8101VcElmodeOffset];

            if (em <= 10)
                elmodeRawOut = (int) em;
        }

        return nameOut[0] != '\0';
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

        if (outselOff > 0 && outselOff + 5 < frameSize)
        {
            out.lmElnsRaw[0] = (int) frame[outselOff + 1];
            out.lmEnllRaw[0] = (int) frame[outselOff + 2];
            out.lmEnlhRaw[0] = (int) frame[outselOff + 3];
            out.lmEvllRaw[0] = (int) frame[outselOff + 4];
            out.lmEvlhRaw[0] = (int) frame[outselOff + 5];

            if (eldtE1UsesOutselStrip (out.elmodeRaw))
                out.lmEldtRaw[0] = (int) frame[outselOff];
        }

        if (out.elvlE1Offset >= 0 && out.wolOffset >= 0)
        {
            out.lmEfln1ElRaw = efln1ElRawFromMixerStrip (frame, frameSize,
                                                          out.elvlE1Offset, out.elvlE1Raw);

            const int efsdlvOff = out.elvlE1Offset + kLm8101VcEfsdlvDeltaFromElvl;

            if (efsendBulk8101TrustedForElmodeRaw (out.elmodeRaw)
                && efsdlvOff >= 0 && efsdlvOff < frameSize)
                out.lmEfsdlvRaw = (int) frame[efsdlvOff];

            /* EFSDVSNS / EFSDSCL: not located in 8101VC bulk (live param9 only). */
        }

        const int maxElnsAnchors = maxElnsAnchorSlotsFromElmodeRaw (out.elmodeRaw);

        if (maxElnsAnchors > 0)
        {
            int anchor = findFirstMixerAnchor (frame, frameSize);

            for (int anchorSlot = 0; anchorSlot < maxElnsAnchors && anchor >= 0; ++anchorSlot)
            {
                if (anchor + 11 < frameSize)
                {
                    const int elIdx = 1 + anchorSlot;

                    if (elIdx < 4)
                    {
                        out.lmOutselRaw[elIdx] = (int) (frame[anchor + 4] & 0x06);
                        out.lmElnsRaw[elIdx] = (int) frame[anchor + 7];
                        out.lmEnllRaw[elIdx] = (int) frame[anchor + 8];
                        out.lmEnlhRaw[elIdx] = (int) frame[anchor + 9];
                        out.lmEvllRaw[elIdx] = (int) frame[anchor + 10];
                        out.lmEvlhRaw[elIdx] = (int) frame[anchor + 11];
                    }

                    if (anchorSlot == 0)
                    {
                        if (out.elvlRaw[1] < 0)
                        {
                            out.elvlRaw[1] = (int) frame[anchor + 5];
                            out.elvlOffset[1] = anchor + 5;
                        }

                        if (eldtE2UsesFirstAnchor (out.elmodeRaw))
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
        readByteIfInRange (frame, frameSize, kLm0040EfsdlvE1Offset, out.efsdlvE1Raw);
        readByteIfInRange (frame, frameSize, kLm0040EfsdlvE2Offset, out.efsdlvE2Raw);
        readByteIfInRange (frame, frameSize, kLm0040SptpntOffset, out.sptpntRaw);
        readByteIfInRange (frame, frameSize, kLm0040EfmodeOffset, out.efmodeRaw);

        int eflnWire = -1;
        readByteIfInRange (frame, frameSize, kLm0040EflnE1Offset, eflnWire);

        if (eflnWire >= 0)
            out.eflnE1Raw = eflnWireValueMasked (eflnWire);

        eflnWire = -1;
        readByteIfInRange (frame, frameSize, kLm0040EflnE2Offset, eflnWire);

        if (eflnWire >= 0)
            out.eflnE2Raw = eflnWireValueMasked (eflnWire);

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
        {
            const auto block = findBestLm0040VcBlockInSysexMessages (messages);

            if (block.data != nullptr && parseLm0040VcMinimal (block.data, block.size, out))
                return true;
        }

        {
            const auto block = findLmBlockInSysexMessages (messages, "0040VC");

            if (block.data != nullptr && parseLm0040VcMinimal (block.data, block.size, out))
                return true;
        }

        juce::MemoryBlock wrapped;

        for (const auto& m : messages)
        {
            if (! m.isSysEx())
                continue;

            const uint8* raw = m.getRawData();
            const int rawN = m.getRawDataSize();

            if (raw != nullptr && rawN > 0)
            {
                if (raw[0] == 0xf0)
                {
                    const auto block = findLmBlock (raw, (size_t) rawN, "0040VC");

                    if (block.data != nullptr && parseLm0040VcMinimal (block.data, block.size, out))
                        return true;
                }
                else if (raw[0] == 0x43 && appendWrappedSysexBody (wrapped, raw, rawN))
                {
                    const auto* w = static_cast<const uint8*> (wrapped.getData());
                    const auto block = findLmBlock (w, wrapped.getSize(), "0040VC");

                    if (block.data != nullptr && parseLm0040VcMinimal (block.data, block.size, out))
                        return true;
                }

                continue;
            }

            const uint8* d = m.getSysExData();
            const int n = m.getSysExDataSize();

            if (d == nullptr || n <= 0)
                continue;

            if (d[0] == 0xf0)
            {
                const auto block = findLmBlock (d, (size_t) n, "0040VC");

                if (block.data != nullptr && parseLm0040VcMinimal (block.data, block.size, out))
                    return true;

                continue;
            }

            if (d[0] != 0x43)
                continue;

            if (! appendWrappedSysexBody (wrapped, d, n))
                continue;

            const auto* w = static_cast<const uint8*> (wrapped.getData());
            const auto block = findLmBlock (w, wrapped.getSize(), "0040VC");

            if (block.data != nullptr && parseLm0040VcMinimal (block.data, block.size, out))
                return true;
        }

        const juce::MemoryBlock concat = concatLiveReadSysexRaw (messages);

        if (concat.getSize() > 0)
        {
            const auto block = findLmBlock (concat.getData(), concat.getSize(), "0040VC");

            if (block.data != nullptr && parseLm0040VcMinimal (block.data, block.size, out))
                return true;

            const auto b8101 = findLmBlock (concat.getData(), concat.getSize(), "8101VC");

            if (b8101.data != nullptr)
            {
                const auto* bytes = static_cast<const uint8*> (concat.getData());
                const int afterEnd = (int) ((b8101.data - bytes) + b8101.size);
                int pairedOff = -1;
                int pairedLen = 0;

                if (findPaired0040FrameAfter (bytes, concat.getSize(), afterEnd, pairedOff, pairedLen)
                    && pairedOff >= 0 && pairedLen > 0
                    && parseLm0040VcMinimal (bytes + (size_t) pairedOff, pairedLen, out))
                    return true;
            }
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

    inline bool parseLm8101VcMinimal (const uint8* frame,
                                      int frameSize,
                                      Lm8101VcMinimal& out,
                                      Lm8101ParseMode mode = Lm8101ParseMode::ingest) noexcept
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
                out.lmOutselE1Raw = outselRawFromPackedByte8101 (packed,
                                                                  eldtE1UsesOutselStrip (out.elmodeRaw));
            }
        }
        else if (frameSize > kLm8101VcOutselE1OffsetTypical)
        {
            const uint8 packed = frame[kLm8101VcOutselE1OffsetTypical];
            out.lmOutselE1Offset = kLm8101VcOutselE1OffsetTypical;
            out.lmOutselE1Raw = outselRawFromPackedByte8101 (packed,
                                                             eldtE1UsesOutselStrip (out.elmodeRaw));
        }

        parseMixerTail8101 (frame, frameSize, out);

#if JUCE_DEBUG
        if (mode == Lm8101ParseMode::auditDebug)
        {
            for (int e = 0; e < 4; ++e)
            {
                const int bulk = e == 0 ? out.lmOutselE1Raw : out.lmOutselRaw[e];
                juce::Logger::writeToLog ("[OUTSEL audit] stage=8101-parse"
                                          + juce::String (" idx=") + juce::String (e)
                                          + " liveRaw=- bulk8101Raw="
                                          + (bulk >= 0
                                                 ? juce::String (bulk) + "(0x"
                                                   + juce::String::toHexString (bulk & 0x7f)
                                                        .toUpperCase().paddedLeft ('0', 2) + ")"
                                                 : juce::String ("-"))
                                          + " resolved=- ui=- baseline=-");
            }

            for (int e = 0; e < 4; ++e)
            {
                juce::Logger::writeToLog ("[ELDT audit] stage=parse8101 idx=" + juce::String (e)
                                          + " parsedRaw="
                                          + (out.lmEldtRaw[e] != -1
                                                 ? juce::String (out.lmEldtRaw[e])
                                                 : juce::String ("-")));
            }

            const struct { const char* tag; int raw; } efsendFields[] = {
                { "EFLN1EL",  out.lmEfln1ElRaw },
                { "EFSDLV",   out.lmEfsdlvRaw },
                { "EFSDVSNS", out.lmEfsdvlRaw },
                { "EFSDSCL",  out.lmEfsdsclRaw },
            };

            for (const auto& f : efsendFields)
            {
                juce::Logger::writeToLog ("[EFSEND audit] stage=parse8101 field=" + juce::String (f.tag)
                                          + " idx=0 parsedRaw="
                                          + (f.raw >= 0 ? juce::String (f.raw) : juce::String ("-")));
            }
        }
#endif

        return true;
    }

    inline bool parseLm8101VcMinimal (const void* data,
                                      size_t dataSize,
                                      Lm8101VcMinimal& out,
                                      Lm8101ParseMode mode = Lm8101ParseMode::ingest) noexcept
    {
        const auto block = findLmBlock (data, dataSize, "8101VC");
        return block.data != nullptr && parseLm8101VcMinimal (block.data, block.size, out, mode);
    }

    inline bool parseLm8101VcMinimalFromSysexMessages (const juce::Array<juce::MidiMessage>& messages,
                                                       Lm8101VcMinimal& out,
                                                       Lm8101ParseMode mode = Lm8101ParseMode::ingest) noexcept
    {
        {
            const auto block = findLmBlockInSysexMessages (messages, "8101VC");

            if (block.data != nullptr && parseLm8101VcMinimal (block.data, block.size, out, mode))
            {
                auditLm8101MixerAnchor (block.data, block.size);
                return true;
            }
        }

        juce::MemoryBlock wrapped;

        for (const auto& m : messages)
        {
            if (! m.isSysEx())
                continue;

            const uint8* raw = m.getRawData();
            const int rawN = m.getRawDataSize();

            if (raw != nullptr && rawN > 0)
            {
                if (raw[0] == 0xf0)
                {
                    const auto block = findLmBlock (raw, (size_t) rawN, "8101VC");

                    if (block.data != nullptr && parseLm8101VcMinimal (block.data, block.size, out, mode))
                    {
                        auditLm8101MixerAnchor (block.data, block.size);
                        return true;
                    }
                }
                else if (raw[0] == 0x43 && appendWrappedSysexBody (wrapped, raw, rawN))
                {
                    const auto* w = static_cast<const uint8*> (wrapped.getData());
                    const auto block = findLmBlock (w, wrapped.getSize(), "8101VC");

                    if (block.data != nullptr && parseLm8101VcMinimal (block.data, block.size, out, mode))
                    {
                        auditLm8101MixerAnchor (block.data, block.size);
                        return true;
                    }
                }

                continue;
            }

            const uint8* d = m.getSysExData();
            const int n = m.getSysExDataSize();

            if (d == nullptr || n <= 0)
                continue;

            if (d[0] == 0xf0)
            {
                const auto block = findLmBlock (d, (size_t) n, "8101VC");

                if (block.data != nullptr && parseLm8101VcMinimal (block.data, block.size, out, mode))
                {
                    auditLm8101MixerAnchor (block.data, block.size);
                    return true;
                }

                continue;
            }

            if (d[0] != 0x43)
                continue;

            if (! appendWrappedSysexBody (wrapped, d, n))
                continue;

            const auto* w = static_cast<const uint8*> (wrapped.getData());
            const auto block = findLmBlock (w, wrapped.getSize(), "8101VC");

            if (block.data != nullptr && parseLm8101VcMinimal (block.data, block.size, out, mode))
            {
                auditLm8101MixerAnchor (block.data, block.size);
                return true;
            }
        }

        const juce::MemoryBlock concat = concatLiveReadSysexRaw (messages);

        if (concat.getSize() > 0)
        {
            const auto block = findLmBlock (concat.getData(), concat.getSize(), "8101VC");

            if (block.data != nullptr && parseLm8101VcMinimal (block.data, block.size, out, mode))
            {
                auditLm8101MixerAnchor (block.data, block.size);
                return true;
            }
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
            if (p.lmEnllRaw[e] >= 0)
                s << " ENLL_E" << (e + 1) << "=" << p.lmEnllRaw[e];
            else
                s << " ENLL_E" << (e + 1) << "=--";

            if (p.lmEnlhRaw[e] >= 0)
                s << " ENLH_E" << (e + 1) << "=" << p.lmEnlhRaw[e];
            else
                s << " ENLH_E" << (e + 1) << "=--";

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
            if (p.lmEnllRaw[e] >= 0)
                s << " ENLL_E" << (e + 1) << "=" << p.lmEnllRaw[e];

            if (p.lmEnlhRaw[e] >= 0)
                s << " ENLH_E" << (e + 1) << "=" << p.lmEnlhRaw[e];

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

        if (p.efmodeRaw >= 0)
            s << " EFMODE=" << p.efmodeRaw;

        if (p.atpbrRaw >= 0)
            s << " ATPBR=" << p.atpbrRaw;

        if (p.efsdlvE1Raw >= 0)
            s << " EFSDLV_E1=" << p.efsdlvE1Raw;

        if (p.efsdlvE2Raw >= 0)
            s << " EFSDLV_E2=" << p.efsdlvE2Raw;

        if (p.eflnE1Raw >= 0)
            s << " EFLN_E1=" << p.eflnE1Raw;

        if (p.eflnE2Raw >= 0)
            s << " EFLN_E2=" << p.eflnE2Raw;

        return s;
    }

#if JUCE_DEBUG

    /** Debug-only: build LiveRead-style SysEx messages from a single F0…F7 frame (not for production). */
    inline juce::Array<juce::MidiMessage> debugBuildLiveReadMessagesFromFrame (
        const uint8* frame, int frameSize, bool useBodyOnlySysexData) noexcept
    {
        juce::Array<juce::MidiMessage> msgs;

        if (frame == nullptr || frameSize < 2 || frame[0] != 0xf0 || frame[frameSize - 1] != 0xf7)
            return msgs;

        const int bodyLen = frameSize - 2;
        const uint8* body = frame + 1;

        if (bodyLen <= 0)
            return msgs;

        if (useBodyOnlySysexData)
        {
            if (body[0] == 0x43)
                msgs.add (juce::MidiMessage::createSysExMessage (body, bodyLen));
        }
        else
        {
            msgs.add (juce::MidiMessage::createSysExMessage (body, bodyLen));
        }

        return msgs;
    }

    inline juce::String debugSelfTestFormatIntField (int v) noexcept
    {
        if (v < 0)
            return "--/--";

        return juce::String (v) + "/0x"
               + juce::String::toHexString (v).toUpperCase().paddedLeft ('0', v > 0xff ? 4 : 2);
    }

    inline void debugSelfTestLogIntMismatch (const char* tag, const char* fieldName,
                                             int fileVal, int liveVal, bool& ok) noexcept
    {
        if (fileVal == liveVal)
            return;

        ok = false;
        juce::Logger::writeToLog (juce::String ("[") + tag + " self-test] field=" + fieldName
                                  + " file=" + debugSelfTestFormatIntField (fileVal)
                                  + " live=" + debugSelfTestFormatIntField (liveVal));
    }

    inline bool debugSelfTestCompareLm8101VcMinimal (const Lm8101VcMinimal& fileParsed,
                                                     const Lm8101VcMinimal& liveParsed) noexcept
    {
        bool ok = true;

        if (fileParsed.found8101 != liveParsed.found8101)
        {
            ok = false;
            juce::Logger::writeToLog ("[LM8101 self-test] field=found8101 file="
                                      + juce::String ((int) fileParsed.found8101)
                                      + " live=" + juce::String ((int) liveParsed.found8101));
        }

        if (std::strcmp (fileParsed.name, liveParsed.name) != 0)
        {
            ok = false;
            juce::Logger::writeToLog ("[LM8101 self-test] field=name file=\""
                                      + juce::String (fileParsed.name).trimEnd()
                                      + "\" live=\"" + juce::String (liveParsed.name).trimEnd() + "\"");
        }

        debugSelfTestLogIntMismatch ("LM8101", "ELMODE", fileParsed.elmodeRaw, liveParsed.elmodeRaw, ok);
        debugSelfTestLogIntMismatch ("LM8101", "WOL", fileParsed.wolRaw, liveParsed.wolRaw, ok);
        debugSelfTestLogIntMismatch ("LM8101", "ELVL_E1", fileParsed.elvlE1Raw, liveParsed.elvlE1Raw, ok);
        debugSelfTestLogIntMismatch ("LM8101", "OUTSEL_E1", fileParsed.lmOutselE1Raw, liveParsed.lmOutselE1Raw, ok);
        debugSelfTestLogIntMismatch ("LM8101", "EFLN1EL", fileParsed.lmEfln1ElRaw, liveParsed.lmEfln1ElRaw, ok);
        debugSelfTestLogIntMismatch ("LM8101", "EFSDLV", fileParsed.lmEfsdlvRaw, liveParsed.lmEfsdlvRaw, ok);
        debugSelfTestLogIntMismatch ("LM8101", "EFSDVL", fileParsed.lmEfsdvlRaw, liveParsed.lmEfsdvlRaw, ok);
        debugSelfTestLogIntMismatch ("LM8101", "EFSDSCL", fileParsed.lmEfsdsclRaw, liveParsed.lmEfsdsclRaw, ok);
        debugSelfTestLogIntMismatch ("LM8101", "elmodeOffset", fileParsed.elmodeOffset, liveParsed.elmodeOffset, ok);
        debugSelfTestLogIntMismatch ("LM8101", "wolOffset", fileParsed.wolOffset, liveParsed.wolOffset, ok);
        debugSelfTestLogIntMismatch ("LM8101", "elvlE1Offset", fileParsed.elvlE1Offset, liveParsed.elvlE1Offset, ok);
        debugSelfTestLogIntMismatch ("LM8101", "outselE1Offset", fileParsed.lmOutselE1Offset, liveParsed.lmOutselE1Offset, ok);

        static const char* const kElvlFields[] = { "ELVL_E1", "ELVL_E2", "ELVL_E3", "ELVL_E4" };
        static const char* const kOutselFields[] = { "OUTSEL_E1", "OUTSEL_E2", "OUTSEL_E3", "OUTSEL_E4" };
        static const char* const kEldtFields[] = { "ELDT_E1", "ELDT_E2", "ELDT_E3", "ELDT_E4" };
        static const char* const kElnsFields[] = { "ELNS_E1", "ELNS_E2", "ELNS_E3", "ELNS_E4" };
        static const char* const kEnllFields[] = { "ENLL_E1", "ENLL_E2", "ENLL_E3", "ENLL_E4" };
        static const char* const kEnlhFields[] = { "ENLH_E1", "ENLH_E2", "ENLH_E3", "ENLH_E4" };
        static const char* const kEvllFields[] = { "EVLL_E1", "EVLL_E2", "EVLL_E3", "EVLL_E4" };
        static const char* const kEvlhFields[] = { "EVLH_E1", "EVLH_E2", "EVLH_E3", "EVLH_E4" };
        static const char* const kElvlOffFields[] = { "elvlOffset_E1", "elvlOffset_E2", "elvlOffset_E3", "elvlOffset_E4" };

        for (int e = 0; e < 4; ++e)
        {
            debugSelfTestLogIntMismatch ("LM8101", kElvlFields[e], fileParsed.elvlRaw[e], liveParsed.elvlRaw[e], ok);
            debugSelfTestLogIntMismatch ("LM8101", kOutselFields[e], fileParsed.lmOutselRaw[e], liveParsed.lmOutselRaw[e], ok);
            debugSelfTestLogIntMismatch ("LM8101", kEldtFields[e], fileParsed.lmEldtRaw[e], liveParsed.lmEldtRaw[e], ok);
            debugSelfTestLogIntMismatch ("LM8101", kElnsFields[e], fileParsed.lmElnsRaw[e], liveParsed.lmElnsRaw[e], ok);
            debugSelfTestLogIntMismatch ("LM8101", kEnllFields[e], fileParsed.lmEnllRaw[e], liveParsed.lmEnllRaw[e], ok);
            debugSelfTestLogIntMismatch ("LM8101", kEnlhFields[e], fileParsed.lmEnlhRaw[e], liveParsed.lmEnlhRaw[e], ok);
            debugSelfTestLogIntMismatch ("LM8101", kEvllFields[e], fileParsed.lmEvllRaw[e], liveParsed.lmEvllRaw[e], ok);
            debugSelfTestLogIntMismatch ("LM8101", kEvlhFields[e], fileParsed.lmEvlhRaw[e], liveParsed.lmEvlhRaw[e], ok);
            debugSelfTestLogIntMismatch ("LM8101", kElvlOffFields[e], fileParsed.elvlOffset[e], liveParsed.elvlOffset[e], ok);
        }

        return ok;
    }

    inline bool debugSelfTestCompareLm0040VcMinimal (const Lm0040VcMinimal& fileParsed,
                                                     const Lm0040VcMinimal& liveParsed) noexcept
    {
        bool ok = true;

        if (fileParsed.found0040 != liveParsed.found0040)
        {
            ok = false;
            juce::Logger::writeToLog ("[LM0040 self-test] field=found0040 file="
                                      + juce::String ((int) fileParsed.found0040)
                                      + " live=" + juce::String ((int) liveParsed.found0040));
        }

        debugSelfTestLogIntMismatch ("LM0040", "WPBR", fileParsed.wpbrRaw, liveParsed.wpbrRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "ATPBR", fileParsed.atpbrRaw, liveParsed.atpbrRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "PMRNG", fileParsed.pmrngRaw, liveParsed.pmrngRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "PMASN", fileParsed.pmasnRaw, liveParsed.pmasnRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "AMASN", fileParsed.amasnRaw, liveParsed.amasnRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "AMRNG", fileParsed.amrngRaw, liveParsed.amrngRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "FMASN", fileParsed.fmasnRaw, liveParsed.fmasnRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "FMRNG", fileParsed.fmrngRaw, liveParsed.fmrngRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "PNLASN", fileParsed.pnlasnRaw, liveParsed.pnlasnRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "PNLRNG", fileParsed.pnlrngRaw, liveParsed.pnlrngRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "COASN", fileParsed.coasnRaw, liveParsed.coasnRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "CORNG", fileParsed.corngRaw, liveParsed.corngRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "PNBASN", fileParsed.pnbasnRaw, liveParsed.pnbasnRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "PNBRNG", fileParsed.pnbrngRaw, liveParsed.pnbrngRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "EGBASN", fileParsed.egbasnRaw, liveParsed.egbasnRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "EGBRNG", fileParsed.egbrngRaw, liveParsed.egbrngRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "WLASN", fileParsed.wlasnRaw, liveParsed.wlasnRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "WLLML", fileParsed.wllmlRaw, liveParsed.wllmlRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "MCTUN", fileParsed.mctunRaw, liveParsed.mctunRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "RNDP", fileParsed.rndpRaw, liveParsed.rndpRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "EFSDLV_E1", fileParsed.efsdlvE1Raw, liveParsed.efsdlvE1Raw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "EFSDLV_E2", fileParsed.efsdlvE2Raw, liveParsed.efsdlvE2Raw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "SPTPNT", fileParsed.sptpntRaw, liveParsed.sptpntRaw, ok);
        debugSelfTestLogIntMismatch ("LM0040", "EFMODE", fileParsed.efmodeRaw, liveParsed.efmodeRaw, ok);

        return ok;
    }

    /** Debug-only: compare direct frame parse vs LiveRead glue for LM 8101VC. Not for production. */
    inline bool debugSelfTestLm8101VcParsePaths (const uint8* frame, int frameSize,
                                                 bool useBodyOnlySysexData = false) noexcept
    {
        Lm8101VcMinimal parsedFile;
        Lm8101VcMinimal parsedLive;

        if (! parseLm8101VcMinimal (frame, frameSize, parsedFile, Lm8101ParseMode::auditDebug))
        {
            juce::Logger::writeToLog ("[LM8101 self-test] direct parse failed (frameSize="
                                      + juce::String (frameSize) + ")");
            return false;
        }

        const auto msgs = debugBuildLiveReadMessagesFromFrame (frame, frameSize, useBodyOnlySysexData);

        if (msgs.isEmpty())
        {
            juce::Logger::writeToLog ("[LM8101 self-test] failed to build LiveRead messages (bodyOnly="
                                      + juce::String ((int) useBodyOnlySysexData) + ")");
            return false;
        }

        if (! parseLm8101VcMinimalFromSysexMessages (msgs, parsedLive))
        {
            juce::Logger::writeToLog ("[LM8101 self-test] LiveRead glue parse failed (bodyOnly="
                                      + juce::String ((int) useBodyOnlySysexData) + ")");
            return false;
        }

        return debugSelfTestCompareLm8101VcMinimal (parsedFile, parsedLive);
    }

    /** Debug-only: compare direct frame parse vs LiveRead glue for LM 0040VC. Not for production. */
    inline bool debugSelfTestLm0040VcParsePaths (const uint8* frame, int frameSize,
                                                 bool useBodyOnlySysexData = false) noexcept
    {
        Lm0040VcMinimal parsedFile;
        Lm0040VcMinimal parsedLive;

        if (! parseLm0040VcMinimal (frame, frameSize, parsedFile))
        {
            juce::Logger::writeToLog ("[LM0040 self-test] direct parse failed (frameSize="
                                      + juce::String (frameSize) + ")");
            return false;
        }

        const auto msgs = debugBuildLiveReadMessagesFromFrame (frame, frameSize, useBodyOnlySysexData);

        if (msgs.isEmpty())
        {
            juce::Logger::writeToLog ("[LM0040 self-test] failed to build LiveRead messages (bodyOnly="
                                      + juce::String ((int) useBodyOnlySysexData) + ")");
            return false;
        }

        if (! parseLm0040VcMinimalFromSysexMessages (msgs, parsedLive))
        {
            juce::Logger::writeToLog ("[LM0040 self-test] LiveRead glue parse failed (bodyOnly="
                                      + juce::String ((int) useBodyOnlySysexData) + ")");
            return false;
        }

        return debugSelfTestCompareLm0040VcMinimal (parsedFile, parsedLive);
    }

#endif
}

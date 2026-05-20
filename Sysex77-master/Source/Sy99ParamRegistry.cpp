/*
  ==============================================================================

    Sy99ParamRegistry.cpp

  ==============================================================================
*/

#include "Sy99ParamRegistry.h"

#include <cstring>

namespace Sy99ParamRegistry
{
    namespace
    {
        int readSlot (const int* slot) noexcept
        {
            return slot != nullptr ? *slot : -1;
        }

        void writeSlot (int* slot, int value) noexcept
        {
            if (slot != nullptr)
                *slot = value;
        }

        ConfidenceFlags cf (bool live, bool b8101, bool b0040, bool cand0040, bool packed) noexcept
        {
            ConfidenceFlags f;
            f.confirmedLive = live;
            f.confirmedBulk8101 = b8101;
            f.confirmedBulk0040 = b0040;
            f.candidateBulk0040 = cand0040;
            f.packedConflict = packed;
            return f;
        }

        const Meta kRegistry[] =
        {
            { Id::ELMODE,   "ELMODE",   { 0x02, 0x00, 0x00, 0x00, false }, 0,   10, cf (true, true,  false, false, false) },
            { Id::WOL,      "WOL",      { 0x02, 0x00, 0x00, 0x3F, false }, 0,  127, cf (true, true,  false, false, false) },
            { Id::WPBR,     "WPBR",     { 0x02, 0x00, 0x00, 0x28, false }, 0,   12, cf (true, false, true,  false, false) },
            { Id::ATPBR,    "ATPBR",    { 0x02, 0x00, 0x00, 0x29, false }, 0,  127, cf (true, false, false, true,  false) },
            { Id::PMASN,    "PMASN",    { 0x02, 0x00, 0x00, 0x2A, false }, 0,  121, cf (true, false, false, true,  false) },
            { Id::PMRNG,    "PMRNG",    { 0x02, 0x00, 0x00, 0x2B, false }, 0,  127, cf (true, false, false, true,  false) },
            { Id::AMASN,    "AMASN",    { 0x02, 0x00, 0x00, 0x2C, false }, 0,  121, cf (true, false, false, true,  true ) },
            { Id::AMRNG,    "AMRNG",    { 0x02, 0x00, 0x00, 0x2D, false }, 0,  127, cf (true, false, false, true,  true ) },
            { Id::FMASN,    "FMASN",    { 0x02, 0x00, 0x00, 0x2E, false }, 0,  121, cf (true, false, false, true,  false) },
            { Id::FMRNG,    "FMRNG",    { 0x02, 0x00, 0x00, 0x2F, false }, 0,  127, cf (true, false, false, true,  true ) },
            { Id::PNLASN,   "PNLASN",   { 0x02, 0x00, 0x00, 0x30, false }, 0,  121, cf (true, false, false, true,  false) },
            { Id::PNLRNG,   "PNLRNG",   { 0x02, 0x00, 0x00, 0x31, false }, 0,  127, cf (true, false, false, true,  true ) },
            { Id::COASN,    "COASN",    { 0x02, 0x00, 0x00, 0x32, false }, 0,  121, cf (true, false, false, true,  false) },
            { Id::CORNG,    "CORNG",    { 0x02, 0x00, 0x00, 0x33, false }, 0,  127, cf (true, false, false, true,  true ) },
            { Id::PNBASN,   "PNBASN",   { 0x02, 0x00, 0x00, 0x34, false }, 0,  121, cf (true, false, false, true,  false) },
            { Id::PNBRNG,   "PNBRNG",   { 0x02, 0x00, 0x00, 0x35, false }, 0,  127, cf (true, false, false, true,  true ) },
            { Id::EGBASN,   "EGBASN",   { 0x02, 0x00, 0x00, 0x36, false }, 0,  121, cf (true, false, false, true,  false) },
            { Id::EGBRNG,   "EGBRNG",   { 0x02, 0x00, 0x00, 0x37, false }, 0,  127, cf (true, false, false, true,  true ) },
            { Id::WLASN,    "WLASN",    { 0x02, 0x00, 0x00, 0x38, false }, 0,  121, cf (true, false, false, true,  false) },
            { Id::WLLML,    "WLLML",    { 0x02, 0x00, 0x00, 0x39, false }, 0,  127, cf (true, false, true,  false, false) },
            { Id::MCTUN,    "MCTUN",    { 0x02, 0x00, 0x00, 0x3A, false }, 0,   65, cf (true, false, false, true,  false) },
            { Id::RNDP,     "RNDP",     { 0x02, 0x00, 0x00, 0x3B, false }, 0,    7, cf (true, false, false, true,  true ) },
            { Id::AFTMD,    "AFTMD",    { 0x02, 0x00, 0x00, 0x42, false }, 0,    4, cf (true, false, false, true,  false) },
            { Id::SPTPNT,   "SPTPNT",   { 0x02, 0x00, 0x00, 0x43, false }, 0,  127, cf (true, false, true,  false, false) },
            { Id::ELVL,     "ELVL",     { 0x03, 0x00, 0x00, 0x00, true  }, 0,  127, cf (true, true,  false, false, false) },
            { Id::ELDT,     "ELDT",     { 0x03, 0x00, 0x00, 0x01, true  }, 0,  127, cf (true, true,  false, false, false) },
            { Id::ELNS,     "ELNS",     { 0x03, 0x00, 0x00, 0x02, true  }, 0,  127, cf (true, true,  false, false, false) },
            { Id::ENLL,     "ENLL",     { 0x03, 0x00, 0x00, 0x03, true  }, 0,  127, cf (true, false, false, false, false) },
            { Id::ENLH,     "ENLH",     { 0x03, 0x00, 0x00, 0x04, true  }, 0,  127, cf (true, false, false, false, false) },
            { Id::EVLL,     "EVLL",     { 0x03, 0x00, 0x00, 0x05, true  }, 0,  127, cf (true, true,  false, false, false) },
            { Id::EVLH,     "EVLH",     { 0x03, 0x00, 0x00, 0x06, true  }, 0,  127, cf (true, true,  false, false, false) },
            { Id::OUTSEL,   "OUTSEL",   { 0x03, 0x00, 0x00, 0x08, true  }, 0,  127, cf (true, true,  false, false, false) },
            { Id::EFLN1EL,  "EFLN1EL",  { 0x03, 0x00, 0x00, 0x09, true  }, 0,  127, cf (false, true, false, false, false) },
            { Id::EFSDLV,   "EFSDLV",   { 0x03, 0x00, 0x00, 0x0A, true  }, 0,  127, cf (false, false, false, false, false) },
            { Id::EFSDVSNS, "EFSDVSNS", { 0x03, 0x00, 0x00, 0x0B, true  }, 0,  127, cf (false, false, false, false, false) },
        };

        int bulk8101ForResolve (const Meta& m, int raw) noexcept
        {
            if (! m.confidence.confirmedBulk8101)
                return -1;

            return raw;
        }

        int bulk0040ForResolve (const Meta& m, int raw) noexcept
        {
            if (! m.confidence.confirmedBulk0040)
                return -1;

            if (m.confidence.candidateBulk0040 || m.confidence.packedConflict)
                return -1;

            return raw;
        }
    }

    const Id kCommonDirectSliderIds[] =
    {
        Id::WPBR, Id::PMRNG, Id::AMRNG, Id::FMRNG, Id::PNLRNG, Id::CORNG,
        Id::PNBRNG, Id::EGBRNG, Id::WLLML, Id::MCTUN, Id::RNDP, Id::SPTPNT
    };

    const Id kCommonDirectComboIds[] =
    {
        Id::PMASN, Id::AMASN, Id::FMASN, Id::PNLASN, Id::COASN,
        Id::PNBASN, Id::EGBASN, Id::WLASN
    };

    const Meta* metaFor (Id id) noexcept
    {
        for (const auto& m : kRegistry)
            if (m.id == id)
                return &m;

        return nullptr;
    }

    const Meta* findByLiveAddress (uint8 b3, uint8 b4, uint8 b5, uint8 b6,
                                   int& outElementIndex) noexcept
    {
        outElementIndex = 0;

        for (const auto& m : kRegistry)
        {
            if (m.addr.b3 != b3 || m.addr.b5 != b5 || m.addr.b6 != b6)
                continue;

            if (m.addr.perElement)
            {
                const int el = LiveSynthState::elementIndexFromOffset (b4);

                if (el < 0 || b4 != (uint8) (el * 0x20))
                    continue;

                outElementIndex = el;
                return &m;
            }

            if (m.addr.b4 == b4)
                return &m;
        }

        return nullptr;
    }

    RawRefs refsFor (LiveSynthState& s, Id id, int elementIndex) noexcept
    {
        RawRefs r;

        switch (id)
        {
            case Id::ELMODE:    r.live = &s.elmodeRaw;         r.bulk8101 = &s.lmElmodeRaw;     break;
            case Id::WOL:       r.live = &s.commonVolumeRaw;   r.bulk8101 = &s.lmWolRaw;        break;
            case Id::WPBR:      r.live = &s.liveWpbrRaw;       r.bulk0040 = &s.lm0040WpbrRaw;   break;
            case Id::ATPBR:     r.live = &s.liveAtpbrRaw;      r.bulk0040 = &s.lm0040AtpbrRaw;  break;
            case Id::PMASN:     r.live = &s.livePmasnRaw;      r.bulk0040 = &s.lm0040PmasnRaw;  break;
            case Id::PMRNG:     r.live = &s.livePmrngRaw;      r.bulk0040 = &s.lm0040PmrngRaw;  break;
            case Id::AMASN:     r.live = &s.liveAmasnRaw;      r.bulk0040 = &s.lm0040AmasnRaw;  break;
            case Id::AMRNG:     r.live = &s.liveAmrngRaw;      r.bulk0040 = &s.lm0040AmrngRaw;  break;
            case Id::FMASN:     r.live = &s.liveFmasnRaw;      r.bulk0040 = &s.lm0040FmasnRaw;  break;
            case Id::FMRNG:     r.live = &s.liveFmrngRaw;      r.bulk0040 = &s.lm0040FmrngRaw;  break;
            case Id::PNLASN:    r.live = &s.livePnlasnRaw;     r.bulk0040 = &s.lm0040PnlasnRaw; break;
            case Id::PNLRNG:    r.live = &s.livePnlrngRaw;     r.bulk0040 = &s.lm0040PnlrngRaw; break;
            case Id::COASN:     r.live = &s.liveCoasnRaw;      r.bulk0040 = &s.lm0040CoasnRaw;  break;
            case Id::CORNG:     r.live = &s.liveCorngRaw;      r.bulk0040 = &s.lm0040CorngRaw;  break;
            case Id::PNBASN:    r.live = &s.livePnbasnRaw;     r.bulk0040 = &s.lm0040PnbasnRaw; break;
            case Id::PNBRNG:    r.live = &s.livePnbrngRaw;     r.bulk0040 = &s.lm0040PnbrngRaw; break;
            case Id::EGBASN:    r.live = &s.liveEgbasnRaw;     r.bulk0040 = &s.lm0040EgbasnRaw; break;
            case Id::EGBRNG:    r.live = &s.liveEgbrngRaw;     r.bulk0040 = &s.lm0040EgbrngRaw; break;
            case Id::WLASN:     r.live = &s.liveWlasnRaw;      r.bulk0040 = &s.lm0040WlasnRaw;  break;
            case Id::WLLML:     r.live = &s.liveWllmlRaw;      r.bulk0040 = &s.lm0040WllmlRaw;  break;
            case Id::MCTUN:     r.live = &s.liveMctunRaw;      r.bulk0040 = &s.lm0040MctunRaw;  break;
            case Id::RNDP:      r.live = &s.liveRndpRaw;       r.bulk0040 = &s.lm0040RndpRaw;   break;
            case Id::AFTMD:     r.live = &s.liveAftmdRaw;      r.bulk0040 = &s.lm0040AftmdRaw;  break;
            case Id::SPTPNT:    r.live = &s.liveSptpntRaw;     r.bulk0040 = &s.lm0040SptpntRaw; break;

            case Id::ELVL:
                if (elementIndex >= 0 && elementIndex < 4)
                {
                    r.live = &s.elementLevelRaw[(size_t) elementIndex];
                    r.bulk8101 = elementIndex == 0 ? &s.lmElvlE1Raw : &s.lmElvlRaw[elementIndex];
                }
                break;

            case Id::ELDT:
                if (elementIndex >= 0 && elementIndex < 4)
                {
                    r.live = &s.elementEldtRaw[(size_t) elementIndex];
                    r.bulk8101 = &s.lmEldtRaw[(size_t) elementIndex];
                }
                break;

            case Id::ELNS:
                if (elementIndex >= 0 && elementIndex < 4)
                {
                    r.live = &s.elementElnsRaw[(size_t) elementIndex];
                    r.bulk8101 = &s.lmElnsRaw[(size_t) elementIndex];
                }
                break;

            case Id::ENLL:
                if (elementIndex >= 0 && elementIndex < 4)
                    r.live = &s.elementEnllRaw[(size_t) elementIndex];
                break;

            case Id::ENLH:
                if (elementIndex >= 0 && elementIndex < 4)
                    r.live = &s.elementEnlhRaw[(size_t) elementIndex];
                break;

            case Id::EVLL:
                if (elementIndex >= 0 && elementIndex < 4)
                {
                    r.live = &s.elementEvllRaw[(size_t) elementIndex];
                    r.bulk8101 = &s.lmEvllRaw[(size_t) elementIndex];
                }
                break;

            case Id::EVLH:
                if (elementIndex >= 0 && elementIndex < 4)
                {
                    r.live = &s.elementEvlhRaw[(size_t) elementIndex];
                    r.bulk8101 = &s.lmEvlhRaw[(size_t) elementIndex];
                }
                break;

            case Id::OUTSEL:
                if (elementIndex >= 0 && elementIndex < 4)
                {
                    r.live = &s.elementOutselRaw[(size_t) elementIndex];
                    r.bulk8101 = elementIndex == 0 ? &s.lmOutselE1Raw : &s.lmOutselRaw[elementIndex];
                }
                break;

            case Id::EFLN1EL:
                if (elementIndex == 0)
                {
                    r.live = &s.elementEfsendselRaw[0];
                    r.bulk8101 = &s.lmEfln1ElRaw;
                }
                break;

            case Id::EFSDLV:
                if (elementIndex == 0)
                {
                    r.live = &s.elementEfsendlvlRaw[0];
                    r.bulk8101 = &s.lmEfsdlvRaw;
                }
                break;

            case Id::EFSDVSNS:
                if (elementIndex == 0)
                {
                    r.live = &s.elementEfsdvsnsRaw[0];
                    r.bulk8101 = &s.lmEfsdvlRaw;
                }
                break;

            default:
                break;
        }

        return r;
    }

    RawRefs refsFor (const LiveSynthState& s, Id id, int elementIndex) noexcept
    {
        return refsFor (const_cast<LiveSynthState&> (s), id, elementIndex);
    }

    int resolveParam (const LiveSynthState& s, Id id, int elementIndex,
                      LiveSynthState::ParamSource& src) noexcept
    {
        const Meta* m = metaFor (id);

        if (m == nullptr)
        {
            src = LiveSynthState::ParamSource::None;
            return -1;
        }

        const RawRefs refs = refsFor (s, id, elementIndex);

        return LiveSynthState::resolveInt (readSlot (refs.live),
                                           bulk8101ForResolve (*m, readSlot (refs.bulk8101)),
                                           bulk0040ForResolve (*m, readSlot (refs.bulk0040)),
                                           src);
    }

    bool ingestLiveParameterFrame (LiveSynthState& s,
                                   uint8 b3, uint8 b4, uint8 b5, uint8 b6, uint8 b8) noexcept
    {
        int el = 0;
        const Meta* m = findByLiveAddress (b3, b4, b5, b6, el);

        if (m == nullptr)
            return false;

        writeSlot (refsFor (s, m->id, el).live, (int) b8);
        return true;
    }

    void applyBulk8101FromParsed (LiveSynthState& s,
                                  const YamahaLmVoiceDump::Lm8101VcMinimal& p) noexcept
    {
        s.hasParsedBulk8101 = p.found8101;

        writeSlot (refsFor (s, Id::ELMODE, 0).bulk8101, p.elmodeRaw);
        writeSlot (refsFor (s, Id::WOL, 0).bulk8101, p.wolRaw);
        writeSlot (refsFor (s, Id::ELVL, 0).bulk8101, p.elvlE1Raw);
        writeSlot (refsFor (s, Id::OUTSEL, 0).bulk8101, p.lmOutselE1Raw);

        if (p.lmOutselE1Raw >= 0)
            s.elementOutselRaw[0] = -1;

        if (p.lmElnsRaw[0] >= -64)
            s.elementElnsRaw[0] = -1;

        writeSlot (refsFor (s, Id::ELNS, 0).bulk8101, p.lmElnsRaw[0]);

        if (p.lmEvllRaw[0] >= 0)
            s.elementEvllRaw[0] = -1;

        if (p.lmEvlhRaw[0] >= 0)
            s.elementEvlhRaw[0] = -1;

        writeSlot (refsFor (s, Id::EVLL, 0).bulk8101, p.lmEvllRaw[0]);
        writeSlot (refsFor (s, Id::EVLH, 0).bulk8101, p.lmEvlhRaw[0]);

        for (int e = 1; e < 4; ++e)
        {
            writeSlot (refsFor (s, Id::ELVL, e).bulk8101, p.elvlRaw[e]);

            if (p.lmOutselRaw[e] >= 0)
                s.elementOutselRaw[e] = -1;

            writeSlot (refsFor (s, Id::OUTSEL, e).bulk8101, p.lmOutselRaw[e]);
            writeSlot (refsFor (s, Id::ELDT, e).bulk8101, p.lmEldtRaw[e]);

            if (p.lmElnsRaw[e] >= -64)
                s.elementElnsRaw[e] = -1;

            writeSlot (refsFor (s, Id::ELNS, e).bulk8101, p.lmElnsRaw[e]);

            if (p.lmEvllRaw[e] >= 0)
                s.elementEvllRaw[e] = -1;

            if (p.lmEvlhRaw[e] >= 0)
                s.elementEvlhRaw[e] = -1;

            writeSlot (refsFor (s, Id::EVLL, e).bulk8101, p.lmEvllRaw[e]);
            writeSlot (refsFor (s, Id::EVLH, e).bulk8101, p.lmEvlhRaw[e]);
        }

        writeSlot (refsFor (s, Id::EFLN1EL, 0).bulk8101, p.lmEfln1ElRaw);
        writeSlot (refsFor (s, Id::EFSDLV, 0).bulk8101, p.lmEfsdlvRaw);
        writeSlot (refsFor (s, Id::EFSDVSNS, 0).bulk8101, p.lmEfsdvlRaw);

        std::memcpy (s.lmVoiceName, p.name, sizeof (s.lmVoiceName));
    }

    void applyBulk0040FromParsed (LiveSynthState& s,
                                  const YamahaLmVoiceDump::Lm0040VcMinimal& p) noexcept
    {
        s.hasParsedBulk0040 = p.found0040;

        writeSlot (refsFor (s, Id::WPBR, 0).bulk0040, p.wpbrRaw);
        writeSlot (refsFor (s, Id::ATPBR, 0).bulk0040, p.atpbrRaw);
        writeSlot (refsFor (s, Id::PMASN, 0).bulk0040, p.pmasnRaw);
        writeSlot (refsFor (s, Id::PMRNG, 0).bulk0040, p.pmrngRaw);
        writeSlot (refsFor (s, Id::AMASN, 0).bulk0040, p.amasnRaw);
        writeSlot (refsFor (s, Id::AMRNG, 0).bulk0040, p.amrngRaw);
        writeSlot (refsFor (s, Id::FMASN, 0).bulk0040, p.fmasnRaw);
        writeSlot (refsFor (s, Id::FMRNG, 0).bulk0040, p.fmrngRaw);
        writeSlot (refsFor (s, Id::PNLASN, 0).bulk0040, p.pnlasnRaw);
        writeSlot (refsFor (s, Id::PNLRNG, 0).bulk0040, p.pnlrngRaw);
        writeSlot (refsFor (s, Id::COASN, 0).bulk0040, p.coasnRaw);
        writeSlot (refsFor (s, Id::CORNG, 0).bulk0040, p.corngRaw);
        writeSlot (refsFor (s, Id::PNBASN, 0).bulk0040, p.pnbasnRaw);
        writeSlot (refsFor (s, Id::PNBRNG, 0).bulk0040, p.pnbrngRaw);
        writeSlot (refsFor (s, Id::EGBASN, 0).bulk0040, p.egbasnRaw);
        writeSlot (refsFor (s, Id::EGBRNG, 0).bulk0040, p.egbrngRaw);
        writeSlot (refsFor (s, Id::WLASN, 0).bulk0040, p.wlasnRaw);
        writeSlot (refsFor (s, Id::WLLML, 0).bulk0040, p.wllmlRaw);
        writeSlot (refsFor (s, Id::MCTUN, 0).bulk0040, p.mctunRaw);
        writeSlot (refsFor (s, Id::RNDP, 0).bulk0040, p.rndpRaw);
        writeSlot (refsFor (s, Id::AFTMD, 0).bulk0040, p.aftmdRaw);
        writeSlot (refsFor (s, Id::SPTPNT, 0).bulk0040, p.sptpntRaw);
    }

    void mirrorBaselineValue (LiveSynthState& s, Id id, int elementIndex, int value) noexcept
    {
        const Meta* m = metaFor (id);

        if (m == nullptr)
            return;

        const RawRefs refs = refsFor (s, id, elementIndex);

        if (refs.live != nullptr)
            writeSlot (refs.live, value);

        if (m->confidence.confirmedBulk8101 && refs.bulk8101 != nullptr)
            writeSlot (refs.bulk8101, value);

        if (m->confidence.confirmedBulk0040 && refs.bulk0040 != nullptr)
            writeSlot (refs.bulk0040, value);
    }

} // namespace Sy99ParamRegistry

void LiveSynthState::applyLm8101Minimal (const YamahaLmVoiceDump::Lm8101VcMinimal& parsed) noexcept
{
    Sy99ParamRegistry::applyBulk8101FromParsed (*this, parsed);
}

void LiveSynthState::applyLm0040Minimal (const YamahaLmVoiceDump::Lm0040VcMinimal& parsed) noexcept
{
    Sy99ParamRegistry::applyBulk0040FromParsed (*this, parsed);
}

void LiveSynthState::ingestParameterFrame (uint8 b3, uint8 b4, uint8 b5, uint8 b6,
                                           uint8 /*b7*/, uint8 b8) noexcept
{
    ++parameterFrameCount;

    if (b3 < 16)
        ++groupFrameCount[b3];

    if (Sy99ParamRegistry::ingestLiveParameterFrame (*this, b3, b4, b5, b6, b8))
        return;

    // VNAM — special case: 10 character slots, not registry scalars.
    if (b3 == 0x02 && b4 == 0x00 && b5 == 0x00 && b6 >= 0x01 && b6 <= 0x0a)
    {
        const int idx = (int) b6 - 0x01;
        voiceName[idx] = (char) (b8 & 0x7f);
        voiceNameCharCount = juce::jmax (voiceNameCharCount, idx + 1);
    }
}

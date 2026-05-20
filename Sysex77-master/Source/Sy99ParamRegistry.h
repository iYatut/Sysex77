/*
  ==============================================================================

    Sy99ParamRegistry.h
    Central parameter registry for SY99 Common / Mixer sync.
    Metadata source: _agent_context audit (lm_8101_offsets.md, 09_confirmed_addresses.md).

    refsFor() points only at existing LiveSynthState canonical raw fields.
    Implementation lives in Sy99ParamRegistry.cpp (no include-at-end coupling).

  ==============================================================================
*/

#pragma once

#include "LiveSynthState.h"

#include <cstdint>

namespace YamahaLmVoiceDump
{
    struct Lm8101VcMinimal;
    struct Lm0040VcMinimal;
}

namespace Sy99ParamRegistry
{
    enum class Id : uint16_t
    {
        ELMODE,
        WOL,
        WPBR,
        ATPBR,
        PMASN,
        PMRNG,
        AMASN,
        AMRNG,
        FMASN,
        FMRNG,
        PNLASN,
        PNLRNG,
        COASN,
        CORNG,
        PNBASN,
        PNBRNG,
        EGBASN,
        EGBRNG,
        WLASN,
        WLLML,
        MCTUN,
        RNDP,
        AFTMD,
        SPTPNT,
        ELVL,
        ELDT,
        ELNS,
        ENLL,
        ENLH,
        EVLL,
        EVLH,
        OUTSEL,
        EFLN1EL,
        EFSDLV,
        EFSDVSNS,
        Count
    };

    struct ConfidenceFlags
    {
        bool confirmedLive       = false;
        bool confirmedBulk8101   = false;
        bool confirmedBulk0040   = false;
        bool candidateBulk0040   = false;
        bool packedConflict      = false;
    };

    struct Address
    {
        uint8 b3 = 0, b4 = 0, b5 = 0, b6 = 0;
        bool perElement = false;
    };

    struct Meta
    {
        Id              id;
        const char*     tag;
        Address         addr;
        int             rawMin;
        int             rawMax;
        ConfidenceFlags confidence;
    };

    struct RawRefs
    {
        int* live     = nullptr;
        int* bulk8101 = nullptr;
        int* bulk0040 = nullptr;
    };

    const Meta* metaFor (Id id) noexcept;
    const Meta* findByLiveAddress (uint8 b3, uint8 b4, uint8 b5, uint8 b6,
                                   int& outElementIndex) noexcept;

    RawRefs refsFor (LiveSynthState& s, Id id, int elementIndex) noexcept;
    RawRefs refsFor (const LiveSynthState& s, Id id, int elementIndex) noexcept;

    /** live > confirmed bulk8101 > confirmed bulk0040 (never candidate / packedConflict 0040). */
    int resolveParam (const LiveSynthState& s, Id id, int elementIndex,
                      LiveSynthState::ParamSource& src) noexcept;

    bool ingestLiveParameterFrame (LiveSynthState& s,
                                   uint8 b3, uint8 b4, uint8 b5, uint8 b6, uint8 b8) noexcept;

    void applyBulk8101FromParsed (LiveSynthState& s,
                                  const YamahaLmVoiceDump::Lm8101VcMinimal& parsed) noexcept;

    void applyBulk0040FromParsed (LiveSynthState& s,
                                  const YamahaLmVoiceDump::Lm0040VcMinimal& parsed) noexcept;

    /** Commit path: write live/canonical field; mirror bulk0040 only when confirmedBulk0040. */
    void mirrorBaselineValue (LiveSynthState& s, Id id, int elementIndex, int value) noexcept;

    constexpr int kCommonDirectSliderIdCount = 12;
    constexpr int kCommonDirectComboIdCount  = 8;

    extern const Id kCommonDirectSliderIds[kCommonDirectSliderIdCount];
    extern const Id kCommonDirectComboIds[kCommonDirectComboIdCount];

} // namespace Sy99ParamRegistry

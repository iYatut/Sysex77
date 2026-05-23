/*
  ==============================================================================

    Sy99LibraryBulkRead.h
    Generic bulk read from parsed LM frames using sy99_param_bindings.json bulkRead.

  ==============================================================================
*/

#pragma once

#include "Sy99MasterCatalog.h"
#include "Sy99ParamRegistry.h"
#include "YamahaLmVoiceDump.h"

namespace Sy99LibraryBulkRead
{
    inline bool bindingAllowsParsed8101 (const Sy99MasterCatalog::Binding& binding) noexcept
    {
        if (binding.bindStatus.equalsIgnoreCase ("manual_only") || binding.bindStatus.isEmpty())
            return false;

        if (! binding.bindStatus.startsWithIgnoreCase ("confirmed")
            && ! binding.bindStatus.startsWithIgnoreCase ("candidate"))
            return false;

        if (binding.bulkRead.frame.equalsIgnoreCase ("8101"))
            return true;

        return binding.bulkSource.equalsIgnoreCase ("8101");
    }

    inline bool bindingAllowsParsed0040 (const Sy99MasterCatalog::Binding& binding) noexcept
    {
        if (binding.bindStatus.equalsIgnoreCase ("manual_only") || binding.bindStatus.isEmpty())
            return false;

        if (! binding.bindStatus.startsWithIgnoreCase ("confirmed")
            && ! binding.bindStatus.startsWithIgnoreCase ("candidate"))
            return false;

        if (binding.bulkRead.frame.equalsIgnoreCase ("0040"))
            return true;

        return binding.bulkSource.equalsIgnoreCase ("0040");
    }

    inline int read8101Field (const juce::String& field,
                              int elementIndex,
                              const YamahaLmVoiceDump::Lm8101VcMinimal& parsed) noexcept
    {
        if (field == "elmodeRaw")
            return parsed.elmodeRaw;

        if (field == "wolRaw")
            return parsed.wolRaw;

        if (elementIndex < 0 || elementIndex >= 4)
            return -1;

        if (field == "elvlRaw")
            return parsed.elvlRaw[elementIndex];

        if (field == "lmEldtRaw")
            return parsed.lmEldtRaw[elementIndex];

        if (field == "lmElnsRaw")
            return parsed.lmElnsRaw[elementIndex];

        if (field == "lmEnllRaw")
            return parsed.lmEnllRaw[elementIndex];

        if (field == "lmEnlhRaw")
            return parsed.lmEnlhRaw[elementIndex];

        if (field == "lmEvllRaw")
            return parsed.lmEvllRaw[elementIndex];

        if (field == "lmEvlhRaw")
            return parsed.lmEvlhRaw[elementIndex];

        if (field == "lmOutselRaw")
            return parsed.lmOutselRaw[elementIndex];

        if (field == "lmEfln1ElRaw")
        {
            if (elementIndex == 0)
                return parsed.lmEfln1ElRaw;

            if (elementIndex == 1
                && YamahaLmVoiceDump::maxElnsAnchorSlotsFromElmodeRaw (parsed.elmodeRaw) >= 1)
                return parsed.lmEfln1ElRaw;

            return -1;
        }

        if (field == "lmEfsdlvRaw")
        {
            if (elementIndex == 0)
                return parsed.lmEfsdlvRaw;

            if (elementIndex == 1
                && YamahaLmVoiceDump::maxElnsAnchorSlotsFromElmodeRaw (parsed.elmodeRaw) >= 1)
                return parsed.lmEfsdlvRaw;

            return -1;
        }

        return -1;
    }

    inline int read0040Field (const juce::String& field,
                              const YamahaLmVoiceDump::Lm0040VcMinimal& parsed) noexcept
    {
        if (field == "wpbrRaw")   return parsed.wpbrRaw;
        if (field == "atpbrRaw")  return parsed.atpbrRaw;
        if (field == "pmasnRaw")  return parsed.pmasnRaw;
        if (field == "pmrngRaw")  return parsed.pmrngRaw;
        if (field == "amasnRaw")  return parsed.amasnRaw;
        if (field == "amrngRaw")  return parsed.amrngRaw;
        if (field == "fmasnRaw")  return parsed.fmasnRaw;
        if (field == "fmrngRaw")  return parsed.fmrngRaw;
        if (field == "pnlasnRaw") return parsed.pnlasnRaw;
        if (field == "pnlrngRaw") return parsed.pnlrngRaw;
        if (field == "coasnRaw")  return parsed.coasnRaw;
        if (field == "corngRaw")  return parsed.corngRaw;
        if (field == "pnbasnRaw") return parsed.pnbasnRaw;
        if (field == "pnbrngRaw") return parsed.pnbrngRaw;
        if (field == "egbasnRaw") return parsed.egbasnRaw;
        if (field == "egbrngRaw") return parsed.egbrngRaw;
        if (field == "wlasnRaw")  return parsed.wlasnRaw;
        if (field == "wllmlRaw")  return parsed.wllmlRaw;
        if (field == "mctunRaw")  return parsed.mctunRaw;
        if (field == "rndpRaw")   return parsed.rndpRaw;
        if (field == "aftmdRaw")  return parsed.aftmdRaw;
        if (field == "sptpntRaw") return parsed.sptpntRaw;
        if (field == "efmodeRaw") return parsed.efmodeRaw;

        return -1;
    }

    inline int readParsed8101FromBinding (const Sy99MasterCatalog::Binding& binding,
                                          int elementIndex,
                                          const YamahaLmVoiceDump::Lm8101VcMinimal* parsed8101) noexcept
    {
        if (parsed8101 == nullptr || ! bindingAllowsParsed8101 (binding))
            return -1;

        if (binding.bulkRead.maxElementIndex >= 0 && elementIndex > binding.bulkRead.maxElementIndex)
            return -1;

        const juce::String field = binding.bulkRead.field;

        if (field.isEmpty())
            return -1;

        const int el = binding.bulkRead.perElement ? elementIndex : 0;
        return read8101Field (field, el, *parsed8101);
    }

    inline int readParsed0040FromBinding (const Sy99MasterCatalog::Binding& binding,
                                          const YamahaLmVoiceDump::Lm0040VcMinimal* parsed0040) noexcept
    {
        if (parsed0040 == nullptr || ! parsed0040->found0040 || ! bindingAllowsParsed0040 (binding))
            return -1;

        const juce::String field = binding.bulkRead.field;

        if (field.isEmpty())
            return -1;

        return read0040Field (field, *parsed0040);
    }

    inline int readParsed8101ByRegistryId (Sy99ParamRegistry::Id id,
                                           int elementIndex,
                                           const YamahaLmVoiceDump::Lm8101VcMinimal* parsed8101) noexcept
    {
        if (parsed8101 == nullptr || elementIndex < 0 || elementIndex >= 4)
            return -1;

        switch (id)
        {
            case Sy99ParamRegistry::Id::ELMODE: return parsed8101->elmodeRaw;
            case Sy99ParamRegistry::Id::WOL:    return parsed8101->wolRaw;
            case Sy99ParamRegistry::Id::ELVL:   return parsed8101->elvlRaw[elementIndex];
            case Sy99ParamRegistry::Id::ELDT:   return parsed8101->lmEldtRaw[elementIndex];
            case Sy99ParamRegistry::Id::ELNS:   return parsed8101->lmElnsRaw[elementIndex];
            case Sy99ParamRegistry::Id::ENLL:   return parsed8101->lmEnllRaw[elementIndex];
            case Sy99ParamRegistry::Id::ENLH:   return parsed8101->lmEnlhRaw[elementIndex];
            case Sy99ParamRegistry::Id::EVLL:   return parsed8101->lmEvllRaw[elementIndex];
            case Sy99ParamRegistry::Id::EVLH:   return parsed8101->lmEvlhRaw[elementIndex];
            case Sy99ParamRegistry::Id::OUTSEL:
                if (elementIndex == 0)
                {
                    int outselE1 = parsed8101->lmOutselE1Raw;

                    if (YamahaLmVoiceDump::eldtE1UsesOutselStrip (parsed8101->elmodeRaw)
                        && parsed8101->lmOutselRaw[1] >= 0
                        && outselE1 != parsed8101->lmOutselRaw[1])
                        outselE1 = parsed8101->lmOutselRaw[1];

                    return outselE1;
                }

                return parsed8101->lmOutselRaw[elementIndex];
            case Sy99ParamRegistry::Id::EFLN1EL:
                if (elementIndex == 0
                    || (elementIndex == 1
                        && YamahaLmVoiceDump::maxElnsAnchorSlotsFromElmodeRaw (parsed8101->elmodeRaw) >= 1))
                    return parsed8101->lmEfln1ElRaw;

                return -1;
            case Sy99ParamRegistry::Id::EFSDLV:
                if (elementIndex == 0
                    || (elementIndex == 1
                        && YamahaLmVoiceDump::maxElnsAnchorSlotsFromElmodeRaw (parsed8101->elmodeRaw) >= 1))
                    return parsed8101->lmEfsdlvRaw;

                return -1;
            default: break;
        }

        return -1;
    }

    inline int readParsed0040ByRegistryId (Sy99ParamRegistry::Id id,
                                           const YamahaLmVoiceDump::Lm0040VcMinimal* parsed0040) noexcept
    {
        if (parsed0040 == nullptr || ! parsed0040->found0040)
            return -1;

        switch (id)
        {
            case Sy99ParamRegistry::Id::WPBR:   return parsed0040->wpbrRaw;
            case Sy99ParamRegistry::Id::ATPBR:  return parsed0040->atpbrRaw;
            case Sy99ParamRegistry::Id::PMASN:  return parsed0040->pmasnRaw;
            case Sy99ParamRegistry::Id::PMRNG:  return parsed0040->pmrngRaw;
            case Sy99ParamRegistry::Id::AMASN:  return parsed0040->amasnRaw;
            case Sy99ParamRegistry::Id::AMRNG:  return parsed0040->amrngRaw;
            case Sy99ParamRegistry::Id::FMASN:  return parsed0040->fmasnRaw;
            case Sy99ParamRegistry::Id::FMRNG:  return parsed0040->fmrngRaw;
            case Sy99ParamRegistry::Id::PNLASN: return parsed0040->pnlasnRaw;
            case Sy99ParamRegistry::Id::PNLRNG: return parsed0040->pnlrngRaw;
            case Sy99ParamRegistry::Id::COASN:  return parsed0040->coasnRaw;
            case Sy99ParamRegistry::Id::CORNG:  return parsed0040->corngRaw;
            case Sy99ParamRegistry::Id::PNBASN: return parsed0040->pnbasnRaw;
            case Sy99ParamRegistry::Id::PNBRNG: return parsed0040->pnbrngRaw;
            case Sy99ParamRegistry::Id::EGBASN: return parsed0040->egbasnRaw;
            case Sy99ParamRegistry::Id::EGBRNG: return parsed0040->egbrngRaw;
            case Sy99ParamRegistry::Id::WLASN:  return parsed0040->wlasnRaw;
            case Sy99ParamRegistry::Id::WLLML:  return parsed0040->wllmlRaw;
            case Sy99ParamRegistry::Id::MCTUN:  return parsed0040->mctunRaw;
            case Sy99ParamRegistry::Id::RNDP:   return parsed0040->rndpRaw;
            case Sy99ParamRegistry::Id::AFTMD:  return parsed0040->aftmdRaw;
            case Sy99ParamRegistry::Id::SPTPNT: return parsed0040->sptpntRaw;
            case Sy99ParamRegistry::Id::EFMODE: return parsed0040->efmodeRaw;
            default: break;
        }

        return -1;
    }
} // namespace Sy99LibraryBulkRead

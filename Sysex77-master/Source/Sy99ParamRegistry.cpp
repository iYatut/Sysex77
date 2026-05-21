/*
  ==============================================================================

    Sy99ParamRegistry.cpp

  ==============================================================================
*/

#include "Sy99ParamRegistry.h"

#include <cstring>
#include <utility>

namespace Sy99ParamRegistry
{
    namespace
    {
        int identityDecode (int raw) noexcept { return raw; }
        int identityEncode (int ui) noexcept { return ui; }

        int decodeAtpbr (int raw) noexcept
        {
            return YamahaLmVoiceDump::uiFromVoiceCommonAtpbrSysex (raw);
        }

        int encodeAtpbr (int ui) noexcept
        {
            return YamahaLmVoiceDump::sysexFromVoiceCommonAtpbrUi (ui);
        }

        int decodeAftmd (int raw) noexcept
        {
            return YamahaLmVoiceDump::uiFromVoiceCommonAftmdSysex (raw);
        }

        int decodeEldt (int raw) noexcept
        {
            return YamahaLmVoiceDump::uiFromElementDetuneSysex (raw);
        }

        int encodeEldt (int ui) noexcept
        {
            return YamahaLmVoiceDump::sysexFromElementDetuneUi (ui);
        }

        int decodeElns (int raw) noexcept
        {
            return YamahaLmVoiceDump::uiFromElementNoteShiftSysex (raw);
        }

        int encodeElns (int ui) noexcept
        {
            return YamahaLmVoiceDump::sysexFromElementNoteShiftUi (ui);
        }

        int decodeEfsdvsns (int raw) noexcept
        {
            return YamahaLmVoiceDump::uiFromMixerEffectSendSigned7Sysex (raw);
        }

        int encodeEfsdvsns (int ui) noexcept
        {
            return YamahaLmVoiceDump::sysexFromMixerEffectSendSigned7Ui (ui);
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

        const Meta kMetaTable[] =
        {
            { Id::ELMODE,   "ELMODE",   false, "Element Mode",              "Voice Common",
              { 0x02, 0x00, 0x00, 0x00, false }, 0,   10, identityDecode, identityEncode,
              { "1 AFM MONO", "2 AFM MONO", "4 AFM MONO", "1 AFM POLY", "2 AFM POLY",
                "1 AWM POLY", "2 AWM POLY", "4 AWM POLY", "1 AFM & 1 AWM POLY", "2 AFM & 2 AWM POLY",
                "DRUM SET", nullptr, nullptr, nullptr, nullptr, nullptr },
              cf (true, true,  false, false, false) },
            { Id::WOL,      "WOL",      false, "Master Volume",             "Voice Common",
              { 0x02, 0x00, 0x00, 0x3F, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::WPBR,     "WPBR",     false, "Wheel Pitch Bend Range",    "Voice Common",
              { 0x02, 0x00, 0x00, 0x28, false }, 0,   12, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::ATPBR,    "ATPBR",    false, "AfterTouch Pitch Bend Range", "Voice Common",
              { 0x02, 0x00, 0x00, 0x29, false }, 0,  127, decodeAtpbr, encodeAtpbr, {},
              cf (true, false, false, true,  false) },
            { Id::PMASN,    "PMASN",    false, "Pitch Mod Assign",          "Voice Common",
              { 0x02, 0x00, 0x00, 0x2A, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, false, true,  false) },
            { Id::PMRNG,    "PMRNG",    false, "Pitch Mod Range",           "Voice Common",
              { 0x02, 0x00, 0x00, 0x2B, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, false, true,  false) },
            { Id::AMASN,    "AMASN",    false, "Amplitude Mod Assign",      "Voice Common",
              { 0x02, 0x00, 0x00, 0x2C, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, false, true,  true ) },
            { Id::AMRNG,    "AMRNG",    false, "Amplitude Mod Range",       "Voice Common",
              { 0x02, 0x00, 0x00, 0x2D, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, false, true,  true ) },
            { Id::FMASN,    "FMASN",    false, "Filter Mod Assign",         "Voice Common",
              { 0x02, 0x00, 0x00, 0x2E, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, false, true,  false) },
            { Id::FMRNG,    "FMRNG",    false, "Filter Mod Range",          "Voice Common",
              { 0x02, 0x00, 0x00, 0x2F, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, false, true,  true ) },
            { Id::PNLASN,   "PNLASN",   false, "Pan Level Assign",          "Voice Common",
              { 0x02, 0x00, 0x00, 0x30, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, false, true,  false) },
            { Id::PNLRNG,   "PNLRNG",   false, "Pan Level Range",           "Voice Common",
              { 0x02, 0x00, 0x00, 0x31, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, false, true,  true ) },
            { Id::COASN,    "COASN",    false, "Filter Cutoff Assign",      "Voice Common",
              { 0x02, 0x00, 0x00, 0x32, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, false, true,  false) },
            { Id::CORNG,    "CORNG",    false, "Filter Cutoff Range",       "Voice Common",
              { 0x02, 0x00, 0x00, 0x33, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, false, true,  true ) },
            { Id::PNBASN,   "PNBASN",   false, "Pan Bias Assign",           "Voice Common",
              { 0x02, 0x00, 0x00, 0x34, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, false, true,  false) },
            { Id::PNBRNG,   "PNBRNG",   false, "Pan Bias Range",            "Voice Common",
              { 0x02, 0x00, 0x00, 0x35, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, false, true,  true ) },
            { Id::EGBASN,   "EGBASN",   false, "EG Bias Assign",            "Voice Common",
              { 0x02, 0x00, 0x00, 0x36, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, false, true,  false) },
            { Id::EGBRNG,   "EGBRNG",   false, "EG Bias Range",             "Voice Common",
              { 0x02, 0x00, 0x00, 0x37, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, false, true,  true ) },
            { Id::WLASN,    "WLASN",    false, "Volume Mod Assign",         "Voice Common",
              { 0x02, 0x00, 0x00, 0x38, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, false, true,  false) },
            { Id::WLLML,    "WLLML",    false, "Volume Limit Low",          "Voice Common",
              { 0x02, 0x00, 0x00, 0x39, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::MCTUN,    "MCTUN",    false, "Micro Tuning",              "Voice Common",
              { 0x02, 0x00, 0x00, 0x3A, false }, 0,   65, identityDecode, identityEncode, {},
              cf (true, false, false, true,  false) },
            { Id::RNDP,     "RNDP",     false, "Random Pitch",              "Voice Common",
              { 0x02, 0x00, 0x00, 0x3B, false }, 0,    7, identityDecode, identityEncode, {},
              cf (true, false, false, true,  true ) },
            { Id::AFTMD,    "AFTMD",    false, "AfterTouch Mode",           "Voice Common",
              { 0x02, 0x00, 0x00, 0x42, false }, 0,    4, decodeAftmd, identityEncode,
              { "all", "top", "btm", "hi", "low", nullptr, nullptr, nullptr, nullptr,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
              cf (true, false, false, true,  false) },
            { Id::SPTPNT,   "SPTPNT",   false, "Split Point",               "Voice Common",
              { 0x02, 0x00, 0x00, 0x43, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::ELVL,     "ELVL",     true,  "Element Level",             "Mixer",
              { 0x03, 0x00, 0x00, 0x00, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::ELDT,     "ELDT",     true,  "Element Detune",            "Mixer",
              { 0x03, 0x00, 0x00, 0x01, true  }, 0,  127, decodeEldt, encodeEldt, {},
              cf (true, true,  false, false, false) },
            { Id::ELNS,     "ELNS",     true,  "Note Shift",                "Mixer",
              { 0x03, 0x00, 0x00, 0x02, true  }, 0,  127, decodeElns, encodeElns, {},
              cf (true, true,  false, false, false) },
            { Id::ENLL,     "ENLL",     true,  "Note Limit Low",            "Mixer",
              { 0x03, 0x00, 0x00, 0x03, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, false, false, false) },
            { Id::ENLH,     "ENLH",     true,  "Note Limit High",           "Mixer",
              { 0x03, 0x00, 0x00, 0x04, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, false, false, false) },
            { Id::EVLL,     "EVLL",     true,  "Velocity Limit Low",        "Mixer",
              { 0x03, 0x00, 0x00, 0x05, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::EVLH,     "EVLH",     true,  "Velocity Limit High",       "Mixer",
              { 0x03, 0x00, 0x00, 0x06, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::OUTSEL,   "OUTSEL",   true,  "Output Select",             "Mixer",
              { 0x03, 0x00, 0x00, 0x08, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::EFLN1EL,  "EFLN1EL",  true,  "Effect Send Lines",         "Mixer",
              { 0x03, 0x00, 0x00, 0x09, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (false, true, false, false, false) },
            { Id::EFSDLV,   "EFSDLV",   true,  "Effect Send Level",         "Mixer",
              { 0x03, 0x00, 0x00, 0x0A, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (false, false, false, false, false) },
            { Id::EFSDVSNS, "EFSDVSNS", true,  "Effect Send Vel Sense",     "Mixer",
              { 0x03, 0x00, 0x00, 0x0B, true  }, 0,  127, decodeEfsdvsns, encodeEfsdvsns, {},
              cf (false, false, false, false, false) },
        };

        static_assert (sizeof (kMetaTable) / sizeof (kMetaTable[0]) == (size_t) Id::Count,
                       "kMetaTable must have one entry per Id");

        int readSlot (const int* slot) noexcept
        {
            return slot != nullptr ? *slot : -1;
        }

        void writeSlot (int* slot, int value) noexcept
        {
            if (slot != nullptr)
                *slot = value;
        }

        int bulk8101ForResolve (const Meta& m, int raw) noexcept
        {
            if (! m.confidence.confirmedBulk8101)
                return -1;

            return raw;
        }

        Id idFromTag (const juce::String& idStr) noexcept
        {
            for (size_t i = 0; i < (size_t) Id::Count; ++i)
                if (idStr.equalsIgnoreCase (kMetaTable[i].tag))
                    return kMetaTable[i].id;

            return Id::Count;
        }

        int bulk0040ForResolve (const Meta& m, int raw) noexcept
        {
            if (! m.confidence.confirmedBulk0040)
                return -1;

            if (m.confidence.candidateBulk0040 || m.confidence.packedConflict)
                return -1;

            return raw;
        }

        int (*codecStringToFn (const juce::String& name, bool decode,
                               int (*fallback) (int)) noexcept) (int);

        struct MetaRecord
        {
            juce::String tag;
            juce::String uiLabel;
            juce::String group;
            juce::StringArray enumNames;
            Meta meta {};

            void syncPointers() noexcept
            {
                meta.tag = tag.isNotEmpty() ? tag.toRawUTF8() : nullptr;
                meta.uiLabel = uiLabel.isNotEmpty() ? uiLabel.toRawUTF8() : nullptr;
                meta.group = group.isNotEmpty() ? group.toRawUTF8() : nullptr;

                for (int i = 0; i < 16; ++i)
                    meta.enumNames[i] = i < enumNames.size() ? enumNames[i].toRawUTF8() : nullptr;
            }

            void assignFromBuiltin (const Meta& src) noexcept
            {
                meta = src;
                tag = src.tag != nullptr ? juce::String (src.tag) : juce::String();
                uiLabel = src.uiLabel != nullptr ? juce::String (src.uiLabel) : juce::String();
                group = src.group != nullptr ? juce::String (src.group) : juce::String();
                enumNames.clearQuick();

                for (int i = 0; i < 16; ++i)
                    if (src.enumNames[i] != nullptr)
                        enumNames.add (src.enumNames[i]);

                syncPointers();
            }

            void applyJsonObject (const juce::var& objVar, const Meta& defaults) noexcept
            {
                assignFromBuiltin (defaults);

                auto* obj = objVar.getDynamicObject();

                if (obj == nullptr)
                    return;

                if (obj->hasProperty ("tag"))
                    tag = obj->getProperty ("tag").toString();

                if (obj->hasProperty ("uiLabel"))
                    uiLabel = obj->getProperty ("uiLabel").toString();

                if (obj->hasProperty ("group"))
                    group = obj->getProperty ("group").toString();

                if (obj->hasProperty ("perElement"))
                    meta.perElement = (bool) obj->getProperty ("perElement");

                if (obj->hasProperty ("rawMin"))
                    meta.rawMin = (int) obj->getProperty ("rawMin");

                if (obj->hasProperty ("rawMax"))
                    meta.rawMax = (int) obj->getProperty ("rawMax");

                if (obj->hasProperty ("decode"))
                    meta.decodeRawToUi = codecStringToFn (obj->getProperty ("decode").toString(), true,
                                                          meta.decodeRawToUi);

                if (obj->hasProperty ("encode"))
                    meta.encodeUiToRaw = codecStringToFn (obj->getProperty ("encode").toString(), false,
                                                          meta.encodeUiToRaw);

                if (obj->hasProperty ("enumNames"))
                {
                    enumNames.clearQuick();
                    const juce::var namesVar = obj->getProperty ("enumNames");

                    if (namesVar.isArray())
                        for (const auto& name : *namesVar.getArray())
                            enumNames.add (name.toString());
                }

                if (obj->hasProperty ("addr"))
                {
                    if (auto* addr = obj->getProperty ("addr").getDynamicObject())
                    {
                        if (addr->hasProperty ("b3"))
                            meta.addr.b3 = (uint8) (int) addr->getProperty ("b3");

                        if (addr->hasProperty ("b4"))
                            meta.addr.b4 = (uint8) (int) addr->getProperty ("b4");

                        if (addr->hasProperty ("b5"))
                            meta.addr.b5 = (uint8) (int) addr->getProperty ("b5");

                        if (addr->hasProperty ("b6"))
                            meta.addr.b6 = (uint8) (int) addr->getProperty ("b6");

                        if (addr->hasProperty ("perElement"))
                            meta.addr.perElement = (bool) addr->getProperty ("perElement");
                    }
                }

                if (obj->hasProperty ("confidence"))
                {
                    if (auto* conf = obj->getProperty ("confidence").getDynamicObject())
                    {
                        if (conf->hasProperty ("confirmedLive"))
                            meta.confidence.confirmedLive = (bool) conf->getProperty ("confirmedLive");

                        if (conf->hasProperty ("confirmedBulk8101"))
                            meta.confidence.confirmedBulk8101 = (bool) conf->getProperty ("confirmedBulk8101");

                        if (conf->hasProperty ("confirmedBulk0040"))
                            meta.confidence.confirmedBulk0040 = (bool) conf->getProperty ("confirmedBulk0040");

                        if (conf->hasProperty ("candidateBulk0040"))
                            meta.confidence.candidateBulk0040 = (bool) conf->getProperty ("candidateBulk0040");

                        if (conf->hasProperty ("packedConflict"))
                            meta.confidence.packedConflict = (bool) conf->getProperty ("packedConflict");
                    }
                }

                meta.addr.perElement = meta.perElement;
                syncPointers();
            }

            void applyPartialJsonPatch (const juce::var& objVar) noexcept
            {
                auto* obj = objVar.getDynamicObject();

                if (obj == nullptr)
                    return;

                if (obj->hasProperty ("uiLabel"))
                    uiLabel = obj->getProperty ("uiLabel").toString();

                if (obj->hasProperty ("group"))
                    group = obj->getProperty ("group").toString();

                if (obj->hasProperty ("perElement"))
                {
                    meta.perElement = (bool) obj->getProperty ("perElement");
                    meta.addr.perElement = meta.perElement;
                }

                if (obj->hasProperty ("rawMin"))
                    meta.rawMin = (int) obj->getProperty ("rawMin");

                if (obj->hasProperty ("rawMax"))
                    meta.rawMax = (int) obj->getProperty ("rawMax");

                if (obj->hasProperty ("decode"))
                    meta.decodeRawToUi = codecStringToFn (obj->getProperty ("decode").toString(), true,
                                                          meta.decodeRawToUi);

                if (obj->hasProperty ("encode"))
                    meta.encodeUiToRaw = codecStringToFn (obj->getProperty ("encode").toString(), false,
                                                          meta.encodeUiToRaw);

                if (obj->hasProperty ("enumNames"))
                {
                    enumNames.clearQuick();
                    const juce::var namesVar = obj->getProperty ("enumNames");

                    if (namesVar.isArray())
                        for (const auto& name : *namesVar.getArray())
                            enumNames.add (name.toString());
                }

                syncPointers();
            }
        };

        std::vector<MetaRecord> gActiveMetaRecords;
        std::vector<MetaRecord> gLastLoadedMetaRecords;
        bool gMetaRegistryInitialized = false;
        juce::CriticalSection gMetaRegistryLock;

        int (*codecStringToFn (const juce::String& name, bool decode,
                               int (*fallback) (int)) noexcept) (int)
        {
            if (name == "identity")
                return decode ? identityDecode : identityEncode;

            if (decode)
            {
                if (name == "uiFromVoiceCommonAtpbrSysex")   return decodeAtpbr;
                if (name == "uiFromVoiceCommonAftmdSysex")   return decodeAftmd;
                if (name == "uiFromElementDetuneSysex")      return decodeEldt;
                if (name == "uiFromElementNoteShiftSysex")   return decodeElns;
                if (name == "uiFromMixerEffectSendSigned7Sysex") return decodeEfsdvsns;
            }
            else
            {
                if (name == "sysexFromVoiceCommonAtpbrUi")   return encodeAtpbr;
                if (name == "sysexFromElementDetuneUi")      return encodeEldt;
                if (name == "sysexFromElementNoteShiftUi")   return encodeElns;
                if (name == "sysexFromMixerEffectSendSigned7Ui") return encodeEfsdvsns;
            }

            return fallback;
        }

        void ensureActiveMetaRecordsInitialized() noexcept
        {
            if (gMetaRegistryInitialized)
                return;

            gActiveMetaRecords.clear();
            gActiveMetaRecords.resize ((size_t) Id::Count);

            for (size_t i = 0; i < (size_t) Id::Count; ++i)
                gActiveMetaRecords[i].assignFromBuiltin (kMetaTable[i]);

            gMetaRegistryInitialized = true;
        }

        const Meta* activeMetaForIndex (size_t idx) noexcept
        {
            ensureActiveMetaRecordsInitialized();

            if (idx >= gActiveMetaRecords.size())
                return nullptr;

            return &gActiveMetaRecords[idx].meta;
        }

        void applyOverridesFromJsonVar (const juce::var& parsed) noexcept
        {
            if (! parsed.isArray())
                return;

            for (const auto& item : *parsed.getArray())
            {
                if (! item.isObject())
                    continue;

                const juce::String idStr = item.getProperty ("id", {}).toString();
                const Id id = idFromTag (idStr);

                if (id >= Id::Count)
                {
                    juce::Logger::writeToLog ("[MetaRegistry] skip unknown id: " + idStr);
                    continue;
                }

                gActiveMetaRecords[(size_t) id].applyJsonObject (item, kMetaTable[(size_t) id]);
            }
        }

        std::vector<MetaRecord> parseMetaRecordsFromJsonVar (const juce::var& parsed)
        {
            std::vector<MetaRecord> out;

            if (! parsed.isArray())
                return out;

            for (const auto& item : *parsed.getArray())
            {
                if (! item.isObject())
                    continue;

                const juce::String idStr = item.getProperty ("id", {}).toString();
                const Id id = idFromTag (idStr);

                if (id >= Id::Count)
                    continue;

                MetaRecord rec;
                rec.applyJsonObject (item, kMetaTable[(size_t) id]);
                out.push_back (std::move (rec));
            }

            return out;
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
        const juce::ScopedLock lock (gMetaRegistryLock);
        const auto idx = static_cast<uint16_t> (id);

        if (idx >= static_cast<uint16_t> (Id::Count))
            return nullptr;

        const Meta* m = activeMetaForIndex ((size_t) idx);

        if (m == nullptr || m->id != id)
            return nullptr;

        return m;
    }

    const Meta* findByLiveAddress (uint8 b3, uint8 b4, uint8 b5, uint8 b6,
                                   int& outElementIndex) noexcept
    {
        const juce::ScopedLock lock (gMetaRegistryLock);
        outElementIndex = 0;
        ensureActiveMetaRecordsInitialized();

        for (const auto& rec : gActiveMetaRecords)
        {
            const Meta& m = rec.meta;

            if (m.addr.b3 != b3 || m.addr.b5 != b5 || m.addr.b6 != b6)
                continue;

            if (m.perElement)
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

    bool buildLiveParameterSysexFrame (Id id, int elementIndex, int rawValue,
                                       uint8 outFrame[9], uint8 sysexDeviceByte) noexcept
    {
        const Meta* m = metaFor (id);

        if (m == nullptr || outFrame == nullptr)
            return false;

        const int rawClamped = juce::jlimit (m->rawMin, m->rawMax, rawValue);

        outFrame[0] = 0x43;
        outFrame[1] = sysexDeviceByte;
        outFrame[2] = 0x34;
        outFrame[3] = m->addr.b3;
        outFrame[4] = m->perElement ? (uint8) (elementIndex * 0x20) : m->addr.b4;
        outFrame[5] = m->addr.b5;
        outFrame[6] = m->addr.b6;
        outFrame[7] = 0x00;
        outFrame[8] = (uint8) rawClamped;
        return true;
    }

    Id idFromString (const juce::String& idStr)
    {
        return idFromTag (idStr);
    }

    void initializeMetaRegistryAtStartup()
    {
        ensureActiveMetaRecordsInitialized();

        const juce::File overrideFile = paramsMetaJsonFile();

        if (! overrideFile.existsAsFile())
            return;

        const juce::var parsed = juce::JSON::parse (overrideFile.loadFileAsString());

        if (parsed.isVoid())
        {
            juce::Logger::writeToLog ("[MetaRegistry] failed to parse " + overrideFile.getFullPathName());
            return;
        }

        applyOverridesFromJsonVar (parsed);
        juce::Logger::writeToLog ("[MetaRegistry] applied overrides from "
                                  + overrideFile.getFullPathName());
    }

    juce::File paramsMetaJsonFile()
    {
        return appDirPath.getChildFile ("params_meta.json");
    }

    std::vector<Meta> loadMetaFromJsonFile (const juce::File& f)
    {
        gLastLoadedMetaRecords.clear();

        if (! f.existsAsFile())
            return {};

        const juce::var parsed = juce::JSON::parse (f.loadFileAsString());

        if (parsed.isVoid())
            return {};

        gLastLoadedMetaRecords = parseMetaRecordsFromJsonVar (parsed);

        std::vector<Meta> out;
        out.reserve (gLastLoadedMetaRecords.size());

        for (const auto& rec : gLastLoadedMetaRecords)
            out.push_back (rec.meta);

        return out;
    }

    juce::String metaIdToString (Id id)
    {
        if (const Meta* m = metaFor (id))
            return m->tag != nullptr ? juce::String (m->tag) : juce::String();

        return {};
    }

    juce::String codecFnToString (int (*fn) (int))
    {
        if (fn == nullptr)
            return "null";

        if (fn == identityDecode || fn == identityEncode)
            return "identity";

        if (fn == decodeAtpbr)
            return "uiFromVoiceCommonAtpbrSysex";

        if (fn == encodeAtpbr)
            return "sysexFromVoiceCommonAtpbrUi";

        if (fn == decodeAftmd)
            return "uiFromVoiceCommonAftmdSysex";

        if (fn == decodeEldt)
            return "uiFromElementDetuneSysex";

        if (fn == encodeEldt)
            return "sysexFromElementDetuneUi";

        if (fn == decodeElns)
            return "uiFromElementNoteShiftSysex";

        if (fn == encodeElns)
            return "sysexFromElementNoteShiftUi";

        if (fn == decodeEfsdvsns)
            return "uiFromMixerEffectSendSigned7Sysex";

        if (fn == encodeEfsdvsns)
            return "sysexFromMixerEffectSendSigned7Ui";

        return "unknown";
    }

    juce::var metaToVar (const Meta& m)
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("id", metaIdToString (m.id));
        root->setProperty ("tag", m.tag != nullptr ? juce::String (m.tag) : juce::String());
        root->setProperty ("uiLabel", m.uiLabel != nullptr ? juce::String (m.uiLabel) : juce::String());
        root->setProperty ("group", m.group != nullptr ? juce::String (m.group) : juce::String());
        root->setProperty ("perElement", m.perElement);
        root->setProperty ("rawMin", m.rawMin);
        root->setProperty ("rawMax", m.rawMax);
        root->setProperty ("decode", codecFnToString (m.decodeRawToUi));
        root->setProperty ("encode", codecFnToString (m.encodeUiToRaw));

        juce::Array<juce::var> enumNames;

        for (int i = 0; i < 16; ++i)
            if (m.enumNames[i] != nullptr)
                enumNames.add (juce::String (m.enumNames[i]));

        root->setProperty ("enumNames", enumNames);

        auto* addr = new juce::DynamicObject();
        addr->setProperty ("b3", (int) m.addr.b3);
        addr->setProperty ("b4", (int) m.addr.b4);
        addr->setProperty ("b5", (int) m.addr.b5);
        addr->setProperty ("b6", (int) m.addr.b6);
        addr->setProperty ("perElement", m.addr.perElement);
        root->setProperty ("addr", addr);

        auto* confidence = new juce::DynamicObject();
        confidence->setProperty ("confirmedLive", m.confidence.confirmedLive);
        confidence->setProperty ("confirmedBulk8101", m.confidence.confirmedBulk8101);
        confidence->setProperty ("confirmedBulk0040", m.confidence.confirmedBulk0040);
        confidence->setProperty ("candidateBulk0040", m.confidence.candidateBulk0040);
        confidence->setProperty ("packedConflict", m.confidence.packedConflict);
        root->setProperty ("confidence", confidence);

        return root;
    }

    void saveMetaToJsonFile (const juce::File& f, const std::vector<Meta>& metas)
    {
        juce::Array<juce::var> rows;

        for (const auto& m : metas)
            rows.add (metaToVar (m));

        const juce::File parent = f.getParentDirectory();

        if (parent != juce::File() && ! parent.exists())
            parent.createDirectory();

        f.replaceWithText (juce::JSON::toString (rows, true));
    }

    std::vector<Meta> defaultMetaTable()
    {
        ensureActiveMetaRecordsInitialized();

        std::vector<Meta> out;
        out.reserve (gActiveMetaRecords.size());

        for (const auto& rec : gActiveMetaRecords)
            out.push_back (rec.meta);

        return out;
    }

    bool applyMetaPatch (Id id, const juce::var& patchObject)
    {
        const juce::ScopedLock lock (gMetaRegistryLock);
        ensureActiveMetaRecordsInitialized();

        if (id >= Id::Count || ! patchObject.isObject())
            return false;

        gActiveMetaRecords[(size_t) id].applyPartialJsonPatch (patchObject);
        return true;
    }

    void persistActiveMetaRegistry()
    {
        std::vector<Meta> snapshot;

        {
            const juce::ScopedLock lock (gMetaRegistryLock);
            ensureActiveMetaRecordsInitialized();
            snapshot.reserve (gActiveMetaRecords.size());

            for (const auto& rec : gActiveMetaRecords)
                snapshot.push_back (rec.meta);
        }

        saveMetaToJsonFile (paramsMetaJsonFile(), snapshot);
    }

    juce::var allParamsToJsonVar()
    {
        juce::Array<juce::var> rows;
        const juce::ScopedLock lock (gMetaRegistryLock);
        ensureActiveMetaRecordsInitialized();

        for (const auto& rec : gActiveMetaRecords)
            rows.add (metaToVar (rec.meta));

        return rows;
    }

    juce::var currentLiveDumpToJsonVar()
    {
        const LiveSynthState& state = getLiveSynthState();
        juce::Array<juce::var> rows;

        for (size_t i = 0; i < (size_t) Id::Count; ++i)
        {
            const Id id = static_cast<Id> (i);
            const Meta* meta = metaFor (id);

            if (meta == nullptr)
                continue;

            const int elementCount = meta->perElement ? 4 : 1;

            for (int elementIndex = 0; elementIndex < elementCount; ++elementIndex)
            {
                LiveSynthState::ParamSource src = LiveSynthState::ParamSource::None;
                const int raw = resolveParam (state, id, elementIndex, src);

                auto* row = new juce::DynamicObject();
                row->setProperty ("id", metaIdToString (id));
                row->setProperty ("elementIndex", elementIndex);

                if (raw >= 0)
                {
                    row->setProperty ("raw", raw);

                    if (meta->decodeRawToUi != nullptr)
                        row->setProperty ("ui", meta->decodeRawToUi (raw));
                    else
                        row->setProperty ("ui", raw);
                }
                else
                {
                    row->setProperty ("raw", juce::var());
                    row->setProperty ("ui", juce::var());
                }

                switch (src)
                {
                    case LiveSynthState::ParamSource::Live:      row->setProperty ("source", "Live"); break;
                    case LiveSynthState::ParamSource::Bulk8101:  row->setProperty ("source", "Bulk8101"); break;
                    case LiveSynthState::ParamSource::Bulk0040:  row->setProperty ("source", "Bulk0040"); break;
                    default:                                     row->setProperty ("source", "None"); break;
                }

                rows.add (row);
            }
        }

        return rows;
    }

    juce::String presentationKindToString (PresentationKind kind) noexcept
    {
        switch (kind)
        {
            case PresentationKind::namedEnum: return "namedEnum";
            case PresentationKind::decoded:   return "decoded";
            default:                          return "numeric";
        }
    }

    juce::String formatLiveSysexFrameHex (const uint8 frame[9]) noexcept
    {
        if (frame == nullptr)
            return {};

        juce::String out = "F0";

        for (int i = 0; i < 9; ++i)
            out << " " << juce::String::toHexString (frame[i]).paddedLeft ('0', 2).toUpperCase();

        out << " F7";
        return out;
    }

    int decodeRawToUiValue (const Meta& meta, int raw) noexcept
    {
        if (meta.decodeRawToUi != nullptr)
            return meta.decodeRawToUi (raw);

        return raw;
    }

    namespace
    {
        bool idInList (Id id, const Id* list, int count) noexcept
        {
            for (int i = 0; i < count; ++i)
                if (list[i] == id)
                    return true;

            return false;
        }

        const char* aftmdProgramLabel (int raw) noexcept
        {
            static const char* kLabels[] = { "all", "top", "btm", "hi", "low" };

            if (raw < 0 || raw > 4)
                return nullptr;

            return kLabels[raw];
        }
    }

    ProgramUiLabel programUiLabelForRaw (Id id, int raw) noexcept
    {
        ProgramUiLabel result;

        if (id == Id::AFTMD)
        {
            if (const char* label = aftmdProgramLabel (raw))
            {
                result.label = label;
                result.kind = PresentationKind::namedEnum;
                return result;
            }
        }

        if (idInList (id, kCommonDirectComboIds, kCommonDirectComboIdCount)
            || idInList (id, kCommonDirectSliderIds, kCommonDirectSliderIdCount))
        {
            result.label = juce::String (raw);
            result.kind = PresentationKind::numeric;
            return result;
        }

        if (id == Id::ATPBR)
        {
            result.label = juce::String (decodeAtpbr (raw));
            result.kind = PresentationKind::decoded;
            return result;
        }

        const Meta* meta = metaFor (id);

        if (meta != nullptr)
        {
            if (raw >= 0 && raw < 16 && meta->enumNames[raw] != nullptr)
            {
                result.label = meta->enumNames[raw];
                result.kind = PresentationKind::namedEnum;
                return result;
            }

            if (meta->decodeRawToUi != nullptr && meta->decodeRawToUi != identityDecode)
            {
                result.label = juce::String (meta->decodeRawToUi (raw));
                result.kind = PresentationKind::decoded;
                return result;
            }
        }

        result.label = juce::String (raw);
        result.kind = PresentationKind::numeric;
        return result;
    }

    juce::String registryEnumLabelForRaw (const Meta& meta, int raw) noexcept
    {
        if (raw >= 0 && raw < 16 && meta.enumNames[raw] != nullptr)
            return meta.enumNames[raw];

        return {};
    }

    juce::var paramValueMapToJsonVar (Id id, int elementIndex, uint8 sysexDeviceByte) noexcept
    {
        const Meta* meta = metaFor (id);

        if (meta == nullptr)
            return {};

        elementIndex = meta->perElement ? juce::jlimit (0, 3, elementIndex) : 0;

        auto* root = new juce::DynamicObject();
        root->setProperty ("paramId", metaIdToString (id));
        root->setProperty ("elementIndex", elementIndex);
        root->setProperty ("rawMin", meta->rawMin);
        root->setProperty ("rawMax", meta->rawMax);

        juce::Array<juce::var> rows;
        int index = 0;

        for (int raw = meta->rawMin; raw <= meta->rawMax; ++raw)
        {
            uint8 frame[9] = {};

            if (! buildLiveParameterSysexFrame (id, elementIndex, raw, frame, sysexDeviceByte))
                continue;

            const int ui = decodeRawToUiValue (*meta, raw);
            const ProgramUiLabel program = programUiLabelForRaw (id, raw);
            const juce::String registryLabel = registryEnumLabelForRaw (*meta, raw);
            const bool labelMismatch = registryLabel.isNotEmpty()
                                       && registryLabel != program.label;

            auto* row = new juce::DynamicObject();
            row->setProperty ("index", index++);
            row->setProperty ("raw", raw);
            row->setProperty ("ui", ui);
            row->setProperty ("sysexHex", formatLiveSysexFrameHex (frame));
            row->setProperty ("presentationKind", presentationKindToString (program.kind));
            row->setProperty ("programLabel", program.label);
            row->setProperty ("registryLabel", registryLabel.isNotEmpty() ? registryLabel : juce::var());
            row->setProperty ("labelMismatch", labelMismatch);
            rows.add (row);
        }

        root->setProperty ("rows", rows);
        return root;
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

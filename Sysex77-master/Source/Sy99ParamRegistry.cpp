/*
  ==============================================================================

    Sy99ParamRegistry.cpp

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"

#include "Sy99ParamRegistry.h"

#include "Sy99HardwareMappingRuntime.h"

#if ! defined (JUCE_DEBUG) && defined (_DEBUG)
 #define JUCE_DEBUG 1
#endif

#include <cstring>
#include <map>
#include <set>
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
              cf (true, false, true,  false, false) },
            { Id::PMASN,    "PMASN",    false, "Pitch Mod Assign",          "Voice Common",
              { 0x02, 0x00, 0x00, 0x2A, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::PMRNG,    "PMRNG",    false, "Pitch Mod Range",           "Voice Common",
              { 0x02, 0x00, 0x00, 0x2B, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::AMASN,    "AMASN",    false, "Amplitude Mod Assign",      "Voice Common",
              { 0x02, 0x00, 0x00, 0x2C, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::AMRNG,    "AMRNG",    false, "Amplitude Mod Range",       "Voice Common",
              { 0x02, 0x00, 0x00, 0x2D, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::FMASN,    "FMASN",    false, "Filter Mod Assign",         "Voice Common",
              { 0x02, 0x00, 0x00, 0x2E, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::FMRNG,    "FMRNG",    false, "Filter Mod Range",          "Voice Common",
              { 0x02, 0x00, 0x00, 0x2F, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::PNLASN,   "PNLASN",   false, "Pan Level Assign",          "Voice Common",
              { 0x02, 0x00, 0x00, 0x30, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::PNLRNG,   "PNLRNG",   false, "Pan Level Range",           "Voice Common",
              { 0x02, 0x00, 0x00, 0x31, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::COASN,    "COASN",    false, "Filter Cutoff Assign",      "Voice Common",
              { 0x02, 0x00, 0x00, 0x32, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::CORNG,    "CORNG",    false, "Filter Cutoff Range",       "Voice Common",
              { 0x02, 0x00, 0x00, 0x33, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::PNBASN,   "PNBASN",   false, "Pan Bias Assign",           "Voice Common",
              { 0x02, 0x00, 0x00, 0x34, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::PNBRNG,   "PNBRNG",   false, "Pan Bias Range",            "Voice Common",
              { 0x02, 0x00, 0x00, 0x35, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::EGBASN,   "EGBASN",   false, "EG Bias Assign",            "Voice Common",
              { 0x02, 0x00, 0x00, 0x36, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::EGBRNG,   "EGBRNG",   false, "EG Bias Range",             "Voice Common",
              { 0x02, 0x00, 0x00, 0x37, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::WLASN,    "WLASN",    false, "Volume Mod Assign",         "Voice Common",
              { 0x02, 0x00, 0x00, 0x38, false }, 0,  121, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::WLLML,    "WLLML",    false, "Volume Limit Low",          "Voice Common",
              { 0x02, 0x00, 0x00, 0x39, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
            { Id::MCTUN,    "MCTUN",    false, "Micro Tuning",              "Voice Common",
              { 0x02, 0x00, 0x00, 0x3A, false }, 0,  127, identityDecode, identityEncode, {},
              cf (true, false, true,  false, false) },
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
            /* ELVL: raw/ui 0..127, identity mapping (no sysex remapping). */
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
              { 0x03, 0x00, 0x00, 0x05, true  }, 1,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::EVLH,     "EVLH",     true,  "Velocity Limit High",       "Mixer",
              { 0x03, 0x00, 0x00, 0x06, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::OUTSEL,   "OUTSEL",   true,  "Output Select",             "Mixer",
              { 0x03, 0x00, 0x00, 0x08, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::EFLN1EL,  "EFLN1EL",  true,  "Effect Send Lines",         "Mixer",
              { 0x03, 0x00, 0x00, 0x09, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::EFSDLV,   "EFSDLV",   true,  "Effect Send Level",         "Mixer",
              { 0x03, 0x00, 0x00, 0x0A, true  }, 0,  127, identityDecode, identityEncode, {},
              cf (true, true,  false, false, false) },
            { Id::EFSDVSNS, "EFSDVSNS", true,  "Effect Send Vel Sense",     "Mixer",
              { 0x03, 0x00, 0x00, 0x0B, true  }, 0,  127, decodeEfsdvsns, encodeEfsdvsns, {},
              cf (true, true,  false, false, false) },
            { Id::EFSDSCL,  "EFSDSCL",  true,  "Effect Send Scaling",       "Mixer",
              { 0x03, 0x00, 0x00, 0x0C, true  }, 0,  127, decodeEfsdvsns, encodeEfsdvsns, {},
              cf (true, true,  false, false, false) },
            { Id::EFMODE,   "EFMODE",   false, "Effect Mode",               "Effect",
              { 0x08, 0x00, 0x00, 0x20, false }, 0,    2, identityDecode, identityEncode,
              { "Off", "Serial", "Parallel", nullptr, nullptr, nullptr, nullptr, nullptr,
                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
              cf (true, false, true,  false, false) },
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

#if JUCE_DEBUG
        juce::String debugFormatWriteAuditInt (int raw) noexcept
        {
            return juce::String (raw);
        }

        juce::String debugFormatWriteAuditChar (char c) noexcept
        {
            if (c == 0)
                return "-";

            if (c >= 32 && c < 127)
                return juce::String::charToString (c);

            return "0x" + juce::String::toHexString ((int) (uint8) c).toUpperCase().paddedLeft ('0', 2);
        }

        const char* efsendFieldTag (Id id) noexcept;

        void debugLogWriteAuditImpl (const char* field, int idx, const char* writer,
                                     const char* stage,
                                     const juce::String& oldStr, const juce::String& newStr) noexcept
        {
            const int64 seq = LiveSynthState::nextWriteAuditSeq();
            juce::Logger::writeToLog ("[WRITE audit] seq=" + juce::String (seq)
                                      + " field=" + juce::String (field)
                                      + " idx=" + juce::String (idx)
                                      + " writer=" + juce::String (writer)
                                      + " stage=" + juce::String (stage != nullptr ? stage : "?")
                                      + " old=" + oldStr
                                      + " new=" + newStr);
        }

        int debugReadIntSlot (const int* slot) noexcept
        {
            return slot != nullptr ? *slot : -1;
        }

        juce::String debugFormatSnapshotChar (char c) noexcept
        {
            if (c == 0)
                return "-";

            return debugFormatWriteAuditChar (c);
        }

        void debugLogOverwriteSnapshotFieldLine (const char* tag, const char* field, int idx,
                                                 int live, int bulk8101, int bulk0040,
                                                 int baseline) noexcept
        {
            juce::Logger::writeToLog ("[OVERWRITE SNAPSHOT] tag=" + juce::String (tag)
                                      + " field=" + juce::String (field)
                                      + " idx=" + juce::String (idx)
                                      + " live=" + debugFormatWriteAuditInt (live)
                                      + " bulk8101=" + debugFormatWriteAuditInt (bulk8101)
                                      + " bulk0040=" + debugFormatWriteAuditInt (bulk0040)
                                      + " baseline=" + debugFormatWriteAuditInt (baseline));
        }

        void debugLogOverwriteSnapshotCharLine (const char* tag, int idx, char live, char bulk8101,
                                                char baseline) noexcept
        {
            juce::Logger::writeToLog ("[OVERWRITE SNAPSHOT] tag=" + juce::String (tag)
                                      + " field=VNAM"
                                      + " idx=" + juce::String (idx)
                                      + " live=" + debugFormatSnapshotChar (live)
                                      + " bulk8101=" + debugFormatSnapshotChar (bulk8101)
                                      + " bulk0040=-"
                                      + " baseline=" + debugFormatSnapshotChar (baseline));
        }

        void debugLogOverwriteSnapshotAllFields (const LiveSynthState& s, const char* tag) noexcept
        {
            for (int i = 0; i < 10; ++i)
            {
                debugLogOverwriteSnapshotCharLine (tag, i, s.voiceName[i], s.lmVoiceName[i],
                                                   s.lmVoiceName[i]);
            }

            const RawRefs elvlRefs = refsFor (s, Id::ELVL, 0);
            debugLogOverwriteSnapshotFieldLine (tag, "ELVL", 0,
                                                debugReadIntSlot (elvlRefs.live),
                                                debugReadIntSlot (elvlRefs.bulk8101), -1,
                                                debugReadIntSlot (elvlRefs.bulk8101));

            const RawRefs elnsRefs = refsFor (s, Id::ELNS, 0);
            debugLogOverwriteSnapshotFieldLine (tag, "ELNS", 0,
                                                debugReadIntSlot (elnsRefs.live),
                                                debugReadIntSlot (elnsRefs.bulk8101), -1,
                                                debugReadIntSlot (elnsRefs.bulk8101));

            const RawRefs outselRefs = refsFor (s, Id::OUTSEL, 0);
            debugLogOverwriteSnapshotFieldLine (tag, "OUTSEL", 0,
                                                debugReadIntSlot (outselRefs.live),
                                                debugReadIntSlot (outselRefs.bulk8101), -1,
                                                debugReadIntSlot (outselRefs.bulk8101));

            const Id efsendIds[] = { Id::EFLN1EL, Id::EFSDLV, Id::EFSDVSNS, Id::EFSDSCL };

            for (const Id id : efsendIds)
            {
                const RawRefs refs = refsFor (s, id, 0);
                debugLogOverwriteSnapshotFieldLine (tag, efsendFieldTag (id), 0,
                                                    debugReadIntSlot (refs.live),
                                                    debugReadIntSlot (refs.bulk8101), -1,
                                                    debugReadIntSlot (refs.bulk8101));
            }
        }

        struct DebugLastWriterEntry
        {
            int lastOld = -1;
            int lastNew = -1;
            const char* writer = "-";
            const char* stage   = "-";
        };

        struct DebugGoldenBulk8101
        {
            bool active = false;
            char vnam[11] {};
            int elvlE1   = -1;
            int outselE1 = -1;
            int elnsE1   = -1;
            int efln1El  = -1;
            int efsdlv   = -1;
            int efsdvsns = -1;
            int efsdscl   = -1;
        };

        DebugLastWriterEntry gLastWriterVNAM;
        DebugLastWriterEntry gLastWriterELVL_bulk_E1;
        DebugLastWriterEntry gLastWriterELVL_live_E1;
        DebugLastWriterEntry gLastWriterOUTSEL_bulk_E1;
        DebugLastWriterEntry gLastWriterOUTSEL_live_E1;
        DebugLastWriterEntry gLastWriterELNS_bulk_E1;
        DebugLastWriterEntry gLastWriterELNS_live_E1;
        DebugLastWriterEntry gLastWriterEFLN1EL;
        DebugLastWriterEntry gLastWriterEFSDLV;
        DebugLastWriterEntry gLastWriterEFSDVSNS;
        DebugLastWriterEntry gLastWriterEFSDSCL;
        DebugGoldenBulk8101 gGoldenBulk8101;

        void debugRecordLastWriter (DebugLastWriterEntry& entry, int oldVal, int newVal,
                                    const char* writer, const char* stage) noexcept
        {
            entry.lastOld = oldVal;
            entry.lastNew = newVal;
            entry.writer = writer != nullptr ? writer : "?";
            entry.stage  = stage  != nullptr ? stage  : "?";
        }

        void debugRecordTrackedFieldWrite (const char* field, int idx, int oldVal, int newVal,
                                           const char* writer, const char* stage) noexcept
        {
            if (field == nullptr)
                return;

            if (juce::String (field) == "VNAM")
                debugRecordLastWriter (gLastWriterVNAM, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "ELVL" && idx == 0)
                debugRecordLastWriter (gLastWriterELVL_bulk_E1, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "ELVL-live" && idx == 0)
                debugRecordLastWriter (gLastWriterELVL_live_E1, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "OUTSEL" && idx == 0)
                debugRecordLastWriter (gLastWriterOUTSEL_bulk_E1, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "OUTSEL-live" && idx == 0)
                debugRecordLastWriter (gLastWriterOUTSEL_live_E1, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "ELNS" && idx == 0)
                debugRecordLastWriter (gLastWriterELNS_bulk_E1, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "ELNS-live" && idx == 0)
                debugRecordLastWriter (gLastWriterELNS_live_E1, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "EFLN1EL" && idx == 0)
                debugRecordLastWriter (gLastWriterEFLN1EL, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "EFSDLV" && idx == 0)
                debugRecordLastWriter (gLastWriterEFSDLV, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "EFSDVSNS" && idx == 0)
                debugRecordLastWriter (gLastWriterEFSDVSNS, oldVal, newVal, writer, stage);
            else if (juce::String (field) == "EFSDSCL" && idx == 0)
                debugRecordLastWriter (gLastWriterEFSDSCL, oldVal, newVal, writer, stage);
        }

        juce::String debugFormatLastWriterLine (const char* label, const DebugLastWriterEntry& e) noexcept
        {
            return juce::String (label) + " writer=" + juce::String (e.writer)
                   + " stage=" + juce::String (e.stage)
                   + " old=" + debugFormatWriteAuditInt (e.lastOld)
                   + " new=" + debugFormatWriteAuditInt (e.lastNew);
        }

        void debugWriteAuditIntSlotImpl (int* slot, const char* field, int idx, int value,
                                         const char* writer, const char* stage) noexcept
        {
            if (slot == nullptr)
                return;

            const int oldVal = *slot;

            if (oldVal != value)
            {
                debugLogWriteAuditImpl (field, idx, writer, stage,
                                        debugFormatWriteAuditInt (oldVal),
                                        debugFormatWriteAuditInt (value));
                debugRecordTrackedFieldWrite (field, idx, oldVal, value, writer, stage);
            }

            *slot = value;
        }

        void debugWriteAuditCharSlotImpl (char* slot, int idx, char value,
                                          const char* writer, const char* stage) noexcept
        {
            if (slot == nullptr)
                return;

            const char oldVal = *slot;

            if (oldVal != value)
            {
                debugLogWriteAuditImpl ("VNAM", idx, writer, stage,
                                        debugFormatWriteAuditChar (oldVal),
                                        debugFormatWriteAuditChar (value));
                debugRecordTrackedFieldWrite ("VNAM", idx, (int) (uint8) oldVal, (int) (uint8) value,
                                                writer, stage);
            }

            *slot = value;
        }

        void debugWriteAuditVnamBufferImpl (char* dest, size_t destSize, const char* src,
                                            size_t srcLen, const char* writer,
                                            const char* stage) noexcept
        {
            if (dest == nullptr || destSize == 0)
                return;

            const size_t n = juce::jmin (destSize, srcLen);

            for (size_t i = 0; i < n; ++i)
                debugWriteAuditCharSlotImpl (dest + i, (int) i, src[i], writer, stage);

            for (size_t i = n; i < destSize; ++i)
                debugWriteAuditCharSlotImpl (dest + i, (int) i, '\0', writer, stage);
        }

        void debugWriteElvlSlotImpl (int* slot, int value, const char* writer, int idx,
                                     const char* stage, const char* layer) noexcept
        {
            const char* field = (layer != nullptr && juce::String (layer) == "live")
                                  ? "ELVL-live" : "ELVL";
            debugWriteAuditIntSlotImpl (slot, field, idx, value, writer, stage);
        }

        void debugWriteElnsSlotImpl (int* slot, int value, const char* writer, int idx,
                                     const char* stage, const char* layer) noexcept
        {
            const char* field = (layer != nullptr && juce::String (layer) == "live")
                                  ? "ELNS-live" : "ELNS";
            debugWriteAuditIntSlotImpl (slot, field, idx, value, writer, stage);
        }

        juce::String debugFormatOutselAuditRaw (int raw) noexcept
        {
            if (raw < 0)
                return "-";

            return juce::String (raw) + "(0x"
                   + juce::String::toHexString (raw & 0x7f).toUpperCase().paddedLeft ('0', 2) + ")";
        }

        void debugLogOutselOverwriteImpl (const char* writer, int idx, int oldVal, int newVal,
                                          const char* stage) noexcept
        {
            juce::Logger::writeToLog ("[OUTSEL audit overwrite] writer=" + juce::String (writer)
                                      + " idx=" + juce::String (idx)
                                      + " old=" + debugFormatOutselAuditRaw (oldVal)
                                      + " new=" + debugFormatOutselAuditRaw (newVal)
                                      + " stage=" + juce::String (stage != nullptr ? stage : "?"));
            debugLogWriteAuditImpl ("OUTSEL", idx, writer, stage,
                                    debugFormatWriteAuditInt (oldVal),
                                    debugFormatWriteAuditInt (newVal));
            if (idx == 0)
                debugRecordTrackedFieldWrite ("OUTSEL", idx, oldVal, newVal, writer, stage);
        }

        void debugWriteOutselSlotImpl (int* slot, int value, const char* writer, int idx,
                                       const char* stage, const char* layer) noexcept
        {
            if (slot == nullptr)
                return;

            const int oldVal = *slot;

            if (oldVal != value)
            {
                const char* field = (layer != nullptr && juce::String (layer) == "live")
                                      ? "OUTSEL-live" : "OUTSEL";
                debugLogWriteAuditImpl (field, idx, writer, stage,
                                        debugFormatWriteAuditInt (oldVal),
                                        debugFormatWriteAuditInt (value));
                debugRecordTrackedFieldWrite (field, idx, oldVal, value, writer, stage);
                juce::Logger::writeToLog ("[OUTSEL audit overwrite] writer=" + juce::String (writer)
                                          + " idx=" + juce::String (idx)
                                          + " old=" + debugFormatOutselAuditRaw (oldVal)
                                          + " new=" + debugFormatOutselAuditRaw (value)
                                          + " stage=" + juce::String (stage != nullptr ? stage : "?"));
            }

            *slot = value;
        }

        bool isOutselLiveAddress (uint8 b3, uint8 b4, uint8 b5, uint8 b6, int& outEl) noexcept
        {
            if (b3 != 0x03 || b5 != 0x00 || b6 != 0x08)
                return false;

            outEl = LiveSynthState::elementIndexFromOffset (b4);
            return outEl >= 0 && outEl < 4;
        }

        void debugLogOutselAuditImpl (const LiveSynthState& s, const char* stage, int elementIndex,
                                      int uiRaw, int baselineRaw) noexcept
        {
            const RawRefs refs = refsFor (s, Id::OUTSEL, elementIndex);
            const int liveRaw = readSlot (refs.live);
            const int bulk8101Raw = readSlot (refs.bulk8101);

            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            int resolved = -1;

            resolved = LiveSynthState::resolveInt (liveRaw, bulk8101Raw, -1, rowSrc);

            juce::Logger::writeToLog ("[OUTSEL audit] stage=" + juce::String (stage)
                                      + " idx=" + juce::String (elementIndex)
                                      + " liveRaw=" + debugFormatOutselAuditRaw (liveRaw)
                                      + " bulk8101Raw=" + debugFormatOutselAuditRaw (bulk8101Raw)
                                      + " resolved=" + debugFormatOutselAuditRaw (resolved)
                                      + " ui=" + debugFormatOutselAuditRaw (uiRaw)
                                      + " baseline=" + debugFormatOutselAuditRaw (baselineRaw));
        }

        void debugLogOutselAuditAllImpl (const LiveSynthState& s, const char* stage,
                                         const int uiByIdx[4], const int baselineByIdx[4]) noexcept
        {
            for (int i = 0; i < 4; ++i)
            {
                const int ui = uiByIdx != nullptr ? uiByIdx[i] : -1;
                const int baseline = baselineByIdx != nullptr ? baselineByIdx[i] : -1;
                debugLogOutselAuditImpl (s, stage, i, ui, baseline);
            }
        }

        juce::String debugFormatEldtAuditRaw (int raw) noexcept
        {
            return raw != -1 ? juce::String (raw) : juce::String ("-");
        }

        void debugLogEldtOverwriteImpl (const char* writer, int idx, int oldVal, int newVal,
                                        const char* stage) noexcept
        {
            juce::Logger::writeToLog ("[ELDT audit overwrite] writer=" + juce::String (writer)
                                      + " idx=" + juce::String (idx)
                                      + " old=" + debugFormatEldtAuditRaw (oldVal)
                                      + " new=" + debugFormatEldtAuditRaw (newVal)
                                      + " stage=" + juce::String (stage != nullptr ? stage : "?"));
            debugLogWriteAuditImpl ("ELDT", idx, writer, stage,
                                    debugFormatWriteAuditInt (oldVal),
                                    debugFormatWriteAuditInt (newVal));
        }

        void debugWriteEldtSlotImpl (int* slot, int value, const char* writer, int idx,
                                     const char* stage) noexcept
        {
            if (slot == nullptr)
                return;

            const int oldVal = *slot;

            if (oldVal != value)
                debugLogEldtOverwriteImpl (writer, idx, oldVal, value, stage);

            *slot = value;
        }

        bool isEldtLiveAddress (uint8 b3, uint8 b4, uint8 b5, uint8 b6, int& outEl) noexcept
        {
            if (b3 != 0x03 || b5 != 0x00 || b6 != 0x01)
                return false;

            outEl = LiveSynthState::elementIndexFromOffset (b4);
            return outEl >= 0 && outEl < 4;
        }

        void debugLogEldtResolveImpl (int elementIndex, int liveRaw, int bulk8101Raw,
                                      int bulk0040Raw, int resolvedRaw,
                                      LiveSynthState::ParamSource src) noexcept
        {
            juce::Logger::writeToLog ("[ELDT audit] stage=resolve idx=" + juce::String (elementIndex)
                                      + " liveRaw=" + debugFormatEldtAuditRaw (liveRaw)
                                      + " bulk8101Raw=" + debugFormatEldtAuditRaw (bulk8101Raw)
                                      + " bulk0040Raw=" + debugFormatEldtAuditRaw (bulk0040Raw)
                                      + " resolvedRaw=" + debugFormatEldtAuditRaw (resolvedRaw)
                                      + " src=" + juce::String (LiveSynthState::paramSourceTag (src)));
        }

        juce::String debugFormatEfsendAuditRaw (int raw) noexcept
        {
            return raw >= 0 ? juce::String (raw) : juce::String ("-");
        }

        const char* efsendFieldTag (Id id) noexcept
        {
            switch (id)
            {
                case Id::EFLN1EL:   return "EFLN1EL";
                case Id::EFSDLV:    return "EFSDLV";
                case Id::EFSDVSNS:  return "EFSDVSNS";
                case Id::EFSDSCL:   return "EFSDSCL";
                default:            return "?";
            }
        }

        bool isEfsendAuditId (Id id) noexcept
        {
            return id == Id::EFLN1EL || id == Id::EFSDLV || id == Id::EFSDVSNS || id == Id::EFSDSCL;
        }

        void debugLogEfsendOverwriteImpl (Id id, const char* writer, int idx, int oldVal,
                                          int newVal, const char* stage) noexcept
        {
            juce::Logger::writeToLog ("[EFSEND audit overwrite] writer=" + juce::String (writer)
                                      + " field=" + juce::String (efsendFieldTag (id))
                                      + " idx=" + juce::String (idx)
                                      + " old=" + debugFormatEfsendAuditRaw (oldVal)
                                      + " new=" + debugFormatEfsendAuditRaw (newVal)
                                      + " stage=" + juce::String (stage != nullptr ? stage : "?"));
            debugLogWriteAuditImpl (efsendFieldTag (id), idx, writer, stage,
                                    debugFormatWriteAuditInt (oldVal),
                                    debugFormatWriteAuditInt (newVal));
            if (idx == 0)
                debugRecordTrackedFieldWrite (efsendFieldTag (id), idx, oldVal, newVal, writer, stage);
        }

        void debugWriteEfsendSlotImpl (int* slot, Id id, int value, const char* writer, int idx,
                                       const char* stage) noexcept
        {
            if (slot == nullptr)
                return;

            const int oldVal = *slot;

            if (oldVal != value)
                debugLogEfsendOverwriteImpl (id, writer, idx, oldVal, value, stage);

            *slot = value;
        }

        void debugLogEfsendAuditResolveImpl (Id id, int elementIndex, int liveRaw, int bulk8101Raw,
                                           int bulk0040Raw, int resolvedRaw,
                                           LiveSynthState::ParamSource src) noexcept
        {
            juce::Logger::writeToLog ("[EFSEND audit] stage=resolve field=" + juce::String (efsendFieldTag (id))
                                      + " idx=" + juce::String (elementIndex)
                                      + " liveRaw=" + debugFormatEfsendAuditRaw (liveRaw)
                                      + " bulk8101Raw=" + debugFormatEfsendAuditRaw (bulk8101Raw)
                                      + " bulk0040Raw=" + debugFormatEfsendAuditRaw (bulk0040Raw)
                                      + " resolvedRaw=" + debugFormatEfsendAuditRaw (resolvedRaw)
                                      + " src=" + juce::String (LiveSynthState::paramSourceTag (src)));
        }

        void debugLogEfsendTruthOverrideImpl (Id id, int elementIndex, int oldVal, int newVal) noexcept
        {
            juce::Logger::writeToLog ("[EFSEND audit] stage=truthOverride field=" + juce::String (efsendFieldTag (id))
                                      + " idx=" + juce::String (elementIndex)
                                      + " old=" + debugFormatEfsendAuditRaw (oldVal)
                                      + " new=" + debugFormatEfsendAuditRaw (newVal));
        }

        void debugLogEfsendApplyBulkImpl (Id id, int elementIndex, int lmRaw) noexcept
        {
            juce::Logger::writeToLog ("[EFSEND audit] stage=applyBulk field=" + juce::String (efsendFieldTag (id))
                                      + " idx=" + juce::String (elementIndex)
                                      + " lmRaw=" + debugFormatEfsendAuditRaw (lmRaw));
        }

        void debugLogTrackedOverwriteSnapshotImpl (const LiveSynthState& s, const char* tag) noexcept
        {
            debugLogOverwriteSnapshotAllFields (s, tag);

            juce::Logger::writeToLog ("[OVERWRITE LAST-WRITER] tag=" + juce::String (tag)
                                      + " " + debugFormatLastWriterLine ("VNAM", gLastWriterVNAM)
                                      + " | " + debugFormatLastWriterLine ("ELVL-bulk-E1", gLastWriterELVL_bulk_E1)
                                      + " | " + debugFormatLastWriterLine ("ELVL-live-E1", gLastWriterELVL_live_E1)
                                      + " | " + debugFormatLastWriterLine ("OUTSEL-bulk-E1", gLastWriterOUTSEL_bulk_E1)
                                      + " | " + debugFormatLastWriterLine ("OUTSEL-live-E1", gLastWriterOUTSEL_live_E1)
                                      + " | " + debugFormatLastWriterLine ("ELNS-bulk-E1", gLastWriterELNS_bulk_E1)
                                      + " | " + debugFormatLastWriterLine ("ELNS-live-E1", gLastWriterELNS_live_E1)
                                      + " | " + debugFormatLastWriterLine ("EFLN1EL", gLastWriterEFLN1EL)
                                      + " | " + debugFormatLastWriterLine ("EFSDLV", gLastWriterEFSDLV)
                                      + " | " + debugFormatLastWriterLine ("EFSDVSNS", gLastWriterEFSDVSNS)
                                      + " | " + debugFormatLastWriterLine ("EFSDSCL", gLastWriterEFSDSCL));

            if (tag != nullptr && juce::String (tag) == "StopSync")
            {
                gGoldenBulk8101.active = true;
                std::memcpy (gGoldenBulk8101.vnam, s.lmVoiceName, sizeof (gGoldenBulk8101.vnam));
                gGoldenBulk8101.elvlE1   = s.lmElvlE1Raw;
                gGoldenBulk8101.outselE1 = s.lmOutselE1Raw;
                gGoldenBulk8101.elnsE1   = s.lmElnsRaw[0];
                gGoldenBulk8101.efln1El  = s.lmEfln1ElRaw;
                gGoldenBulk8101.efsdlv   = s.lmEfsdlvRaw;
                gGoldenBulk8101.efsdvsns = s.lmEfsdvlRaw;
                gGoldenBulk8101.efsdscl   = s.lmEfsdsclRaw;
                return;
            }

            if (! gGoldenBulk8101.active)
                return;

            auto logRegress = [] (const char* field, int golden, int current,
                                  const DebugLastWriterEntry& lw) noexcept
            {
                if (golden < 0 || golden == current)
                    return;

                juce::Logger::writeToLog ("[OVERWRITE REGRESS] field=" + juce::String (field)
                                          + " golden=" + debugFormatWriteAuditInt (golden)
                                          + " now=" + debugFormatWriteAuditInt (current)
                                          + " lastWriter=" + juce::String (lw.writer)
                                          + " stage=" + juce::String (lw.stage)
                                          + " wrote old=" + debugFormatWriteAuditInt (lw.lastOld)
                                          + " new=" + debugFormatWriteAuditInt (lw.lastNew));
            };

            logRegress ("VNAM", (int) (uint8) gGoldenBulk8101.vnam[0], (int) (uint8) s.lmVoiceName[0],
                        gLastWriterVNAM);
            logRegress ("ELVL_E1", gGoldenBulk8101.elvlE1, s.lmElvlE1Raw, gLastWriterELVL_bulk_E1);
            logRegress ("OUTSEL_E1", gGoldenBulk8101.outselE1, s.lmOutselE1Raw, gLastWriterOUTSEL_bulk_E1);
            logRegress ("ELNS_E1", gGoldenBulk8101.elnsE1, s.lmElnsRaw[0], gLastWriterELNS_bulk_E1);
            logRegress ("EFLN1EL", gGoldenBulk8101.efln1El, s.lmEfln1ElRaw, gLastWriterEFLN1EL);
            logRegress ("EFSDLV", gGoldenBulk8101.efsdlv, s.lmEfsdlvRaw, gLastWriterEFSDLV);
            logRegress ("EFSDVSNS", gGoldenBulk8101.efsdvsns, s.lmEfsdvlRaw, gLastWriterEFSDVSNS);
            logRegress ("EFSDSCL", gGoldenBulk8101.efsdscl, s.lmEfsdsclRaw, gLastWriterEFSDSCL);
        }
#endif

        constexpr int kElnsUiMin = -64;
        constexpr int kElnsUiMax = 63;

        bool rawSlotPresent (int raw) noexcept
        {
            return raw >= 0;
        }

        /** LM8101 lmElnsRaw[] stores note-shift UI (-64..63), not sysex 0..127. */
        bool elnsBulkSlotPresent (int raw) noexcept
        {
            return raw >= kElnsUiMin && raw <= kElnsUiMax;
        }

        constexpr int kEldtUiMin = -7;
        constexpr int kEldtUiMax = 7;

        /** LM8101 lmEldtRaw[] stores detune UI (-7..+7) after applyBulk8101FromParsed. */
        bool eldtBulkSlotPresent (int raw) noexcept
        {
            return raw >= kEldtUiMin && raw <= kEldtUiMax;
        }

        int resolveElnsInt (int liveRaw, int bulk8101Raw, int bulk0040Raw,
                            LiveSynthState::ParamSource& src) noexcept
        {
            if (rawSlotPresent (liveRaw))
            {
                src = LiveSynthState::ParamSource::Live;
                return liveRaw;
            }

            if (elnsBulkSlotPresent (bulk8101Raw))
            {
                src = LiveSynthState::ParamSource::Bulk8101;
                return bulk8101Raw;
            }

            if (rawSlotPresent (bulk0040Raw))
            {
                src = LiveSynthState::ParamSource::Bulk0040;
                return bulk0040Raw;
            }

            src = LiveSynthState::ParamSource::None;
            return -1;
        }

        int resolveEldtInt (int liveRaw, int bulk8101Raw, int bulk0040Raw,
                            LiveSynthState::ParamSource& src) noexcept
        {
            if (rawSlotPresent (liveRaw))
            {
                src = LiveSynthState::ParamSource::Live;
                return liveRaw;
            }

            if (eldtBulkSlotPresent (bulk8101Raw))
            {
                src = LiveSynthState::ParamSource::Bulk8101;
                return bulk8101Raw;
            }

            if (rawSlotPresent (bulk0040Raw))
            {
                src = LiveSynthState::ParamSource::Bulk0040;
                return bulk0040Raw;
            }

            src = LiveSynthState::ParamSource::None;
            return -1;
        }

        bool bulkIngestIsAuthoritative (const LiveSynthState& s) noexcept
        {
            return s.hasParsedBulk8101 || s.hasParsedBulk0040;
        }

        bool paramSourceIsPresent (LiveSynthState::ParamSource src) noexcept
        {
            return src == LiveSynthState::ParamSource::Live
                || src == LiveSynthState::ParamSource::Bulk8101
                || src == LiveSynthState::ParamSource::Bulk0040;
        }

        const char* paramSourceLabel (LiveSynthState::ParamSource src) noexcept
        {
            switch (src)
            {
                case LiveSynthState::ParamSource::Live:      return "Live";
                case LiveSynthState::ParamSource::Bulk8101:  return "Bulk8101";
                case LiveSynthState::ParamSource::Bulk0040:  return "Bulk0040";
                default:                                     return "None";
            }
        }

        juce::String formatResolvedRawForLog (Id id, int raw) noexcept
        {
            if (id == Id::ELNS)
            {
                if (elnsBulkSlotPresent (raw) || rawSlotPresent (raw))
                    return juce::String (raw);

                return "-";
            }

            return raw >= 0 ? juce::String (raw) : juce::String ("-");
        }

        void logSyncAuthoritativeResolve (Id id, int elementIndex, int resolvedRaw,
                                          LiveSynthState::ParamSource src,
                                          bool metaOverrideSkipped) noexcept
        {
            juce::String idTag = metaIdToString (id);

            if (const Meta* m = metaFor (id); m != nullptr && m->tag != nullptr)
                idTag = m->tag;

            juce::Logger::writeToLog ("[Sync authoritative] id=" + idTag
                                      + " idx=" + juce::String (elementIndex)
                                      + " resolvedRaw=" + formatResolvedRawForLog (id, resolvedRaw)
                                      + " src=" + juce::String (paramSourceLabel (src))
                                      + " metaOverrideSkipped="
                                      + juce::String (metaOverrideSkipped ? 1 : 0));
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

        juce::var emptySy99VerificationObject() noexcept
        {
            return juce::var (new juce::DynamicObject());
        }

        void normalizeVerificationElementVar (juce::var& elementVar) noexcept
        {
            auto* el = elementVar.getDynamicObject();

            if (el == nullptr)
                return;

            auto dedupeIntArrayProp = [&] (const char* propName)
            {
                if (auto* rawArr = el->getProperty (propName).getArray())
                {
                    juce::SortedSet<int> seen;
                    juce::Array<juce::var> unique;

                    for (const auto& v : *rawArr)
                    {
                        const int n = (int) v;

                        if (! seen.contains (n))
                        {
                            seen.add (n);
                            unique.add (n);
                        }
                    }

                    el->setProperty (propName, unique);
                }
            };

            dedupeIntArrayProp ("verifiedRaws");
            el->removeProperty ("hiddenRaws");

            if (el->hasProperty ("canonicalRaw"))
            {
                const int canon = (int) el->getProperty ("canonicalRaw");

                if (canon < 0)
                    el->removeProperty ("canonicalRaw");
                else
                    el->setProperty ("canonicalRaw", canon);
            }
        }

        void normalizeSy99VerificationByElement (juce::var& byElement) noexcept
        {
            auto* root = byElement.getDynamicObject();

            if (root == nullptr)
                return;

            for (auto& prop : root->getProperties())
            {
                juce::var val = prop.value;
                normalizeVerificationElementVar (val);
                root->setProperty (prop.name, val);
            }
        }

        /** Полная замена состояния по каждому ключу element ("0"…"3"); массивы не сливаются. */
        void replaceSy99VerificationPatch (juce::var& target, const juce::var& patch) noexcept
        {
            if (! patch.isObject())
                return;

            auto* patchObj = patch.getDynamicObject();

            if (patchObj == nullptr)
                return;

            if (! target.isObject())
                target = emptySy99VerificationObject();

            auto* targetObj = target.getDynamicObject();

            if (targetObj == nullptr)
                return;

            for (const auto& prop : patchObj->getProperties())
            {
                juce::var val = prop.value;
                normalizeVerificationElementVar (val);
                targetObj->setProperty (prop.name, val);
            }

            normalizeSy99VerificationByElement (target);
        }

        /** enumNames[raw] в MetaRecord; JSON-массив: индекс = raw (0…127). */
        void loadEnumNamesFromJsonArray (juce::StringArray& dest, const juce::var& namesVar) noexcept
        {
            dest.clearQuick();

            if (! namesVar.isArray())
                return;

            auto* arr = namesVar.getArray();

            for (int i = 0; i < arr->size() && i <= 127; ++i)
            {
                while (dest.size() <= i)
                    dest.add (juce::String());

                dest.set (i, (*arr)[i].toString());
            }
        }

        struct MetaRecord
        {
            juce::String tag;
            juce::String uiLabel;
            juce::String group;
            juce::StringArray enumNames;
            juce::var sy99VerificationByElement;
            int sy99LcdPage = 0;
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
                sy99LcdPage = (src.id == Id::WOL) ? 202 : 0;
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
                    loadEnumNamesFromJsonArray (enumNames, obj->getProperty ("enumNames"));

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

                if (obj->hasProperty ("sy99Verification"))
                {
                    sy99VerificationByElement = obj->getProperty ("sy99Verification");
                    normalizeSy99VerificationByElement (sy99VerificationByElement);
                    collapseVerificationTruthOnly();
                }

                if (obj->hasProperty ("sy99LcdPage"))
                    sy99LcdPage = (int) obj->getProperty ("sy99LcdPage");

                meta.addr.perElement = meta.perElement;
                syncPointers();
            }

            /** canonicalRaw — fallback for resolve only when no bulk ingest / no present source. */
            void collapseVerificationTruthOnly() noexcept
            {
                if (! sy99VerificationByElement.isObject())
                    return;

                auto* root = sy99VerificationByElement.getDynamicObject();

                if (root == nullptr)
                    return;

                for (auto& prop : root->getProperties())
                {
                    juce::var elVar = prop.value;
                    auto* el = elVar.getDynamicObject();

                    if (el == nullptr)
                        continue;

                    int canon = -1;

                    if (el->hasProperty ("canonicalRaw"))
                        canon = (int) el->getProperty ("canonicalRaw");

                    if (canon >= meta.rawMin && canon <= meta.rawMax)
                    {
                        juce::String savedAt = el->getProperty ("confirmedAt").toString();

                        if (savedAt.isEmpty())
                            savedAt = juce::Time::getCurrentTime().toISO8601 (true);

                        el->setProperty ("canonicalRaw", canon);
                        el->setProperty ("confirmed", true);
                        el->setProperty ("confirmedAt", savedAt);
                        el->removeProperty ("verifiedRaws");
                    }
                    else
                    {
                        if (auto* rawArr = el->getProperty ("verifiedRaws").getArray())
                        {
                            const int span = meta.rawMax - meta.rawMin + 1;

                            if (span > 0 && rawArr->size() >= (size_t) span)
                            {
                                el->removeProperty ("verifiedRaws");
                                el->setProperty ("confirmed", false);
                                el->removeProperty ("confirmedAt");
                            }
                        }
                    }

                    root->setProperty (prop.name, elVar);
                }
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
                    loadEnumNamesFromJsonArray (enumNames, obj->getProperty ("enumNames"));

                if (obj->hasProperty ("sy99Verification"))
                {
                    const juce::var patch = obj->getProperty ("sy99Verification");

                    if (! meta.perElement)
                    {
                        if (auto* patchObj = patch.getDynamicObject())
                        {
                            if (patchObj->getProperties().size() == 0)
                                sy99VerificationByElement = juce::var();
                            else
                            {
                                sy99VerificationByElement = patch;
                                normalizeSy99VerificationByElement (sy99VerificationByElement);
                            }
                        }
                        else
                        {
                            sy99VerificationByElement = juce::var();
                        }
                    }
                    else
                    {
                        replaceSy99VerificationPatch (sy99VerificationByElement, patch);
                    }

                    collapseVerificationTruthOnly();
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

        MetaRecord* activeMetaRecordFor (Id id) noexcept
        {
            ensureActiveMetaRecordsInitialized();
            const auto idx = static_cast<size_t> (id);

            if (id >= Id::Count || idx >= gActiveMetaRecords.size())
                return nullptr;

            return &gActiveMetaRecords[idx];
        }

        /** ELVL: raw/ui 0..127, identity codec — не перезаписывается params_meta.json. */
        void enforceElvlMetaGuard() noexcept
        {
            if (MetaRecord* elvl = activeMetaRecordFor (Id::ELVL); elvl != nullptr)
            {
                elvl->meta.rawMin = 0;
                elvl->meta.rawMax = 127;
                elvl->meta.decodeRawToUi = identityDecode;
                elvl->meta.encodeUiToRaw = identityEncode;
            }
        }

        juce::String enumLabelAtRaw (const MetaRecord& rec, int raw) noexcept
        {
            if (raw >= 0 && raw < rec.enumNames.size() && rec.enumNames[raw].isNotEmpty())
                return rec.enumNames[raw];

            if (raw >= 0 && raw < 16 && rec.meta.enumNames[raw] != nullptr)
                return rec.meta.enumNames[raw];

            return {};
        }

        juce::var enumNamesToJsonVarFromRecord (const MetaRecord& rec) noexcept
        {
            juce::Array<juce::var> arr;
            const int last = juce::jmin (127, juce::jmax (rec.meta.rawMax, rec.enumNames.size() - 1));

            for (int raw = 0; raw <= last; ++raw)
            {
                while (arr.size() < (size_t) raw)
                    arr.add (juce::String());

                const juce::String name = raw < rec.enumNames.size() ? rec.enumNames[raw] : juce::String();
                arr.add (name);
            }

            return arr;
        }

        int truthRawForElement (const MetaRecord& rec, int elementIndex) noexcept
        {
            if (! rec.sy99VerificationByElement.isObject())
                return -1;

            auto* root = rec.sy99VerificationByElement.getDynamicObject();

            if (root == nullptr)
                return -1;

            auto* el = root->getProperty (juce::String (elementIndex)).getDynamicObject();

            if (el == nullptr || ! el->hasProperty ("canonicalRaw"))
                return -1;

            const int canon = (int) el->getProperty ("canonicalRaw");

            if (canon < rec.meta.rawMin || canon > rec.meta.rawMax)
                return -1;

            if (el->getProperty ("confirmedAt").toString().isEmpty())
                return -1;

            return canon;
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

            juce::StringArray seenIds;

            for (const auto& item : *parsed.getArray())
            {
                if (! item.isObject())
                    continue;

                const juce::String idStr = item.getProperty ("id", {}).toString();

                if (idStr.isEmpty())
                    continue;

                if (seenIds.contains (idStr))
                {
                    juce::Logger::writeToLog ("[MetaRegistry] skip duplicate id in params_meta.json: "
                                              + idStr);
                    continue;
                }

                seenIds.add (idStr);

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

        juce::String catalogEntryKey (const juce::String& id, int elementIndex) noexcept
        {
            if (elementIndex < 0)
                return id;

            return id + ":" + juce::String (elementIndex);
        }

        juce::String formatCatalogElementForLog (int elementIndex) noexcept
        {
            return elementIndex < 0 ? juce::String ("-") : juce::String (elementIndex);
        }

        juce::String sysexTemplateFromMetaAddr (const Meta& m, int elementIndex) noexcept
        {
            const uint8 b4 = m.perElement ? (uint8) (elementIndex * 0x20) : m.addr.b4;

            return juce::String ("F0 43 10 34 ")
                   + juce::String::toHexString ((int) m.addr.b3).toUpperCase().paddedLeft ('0', 2)
                   + " "
                   + juce::String::toHexString ((int) b4).toUpperCase().paddedLeft ('0', 2)
                   + " "
                   + juce::String::toHexString ((int) m.addr.b5).toUpperCase().paddedLeft ('0', 2)
                   + " "
                   + juce::String::toHexString ((int) m.addr.b6).toUpperCase().paddedLeft ('0', 2)
                   + " 00 xx F7";
        }

        juce::String expectedCatalogGroupId (const Meta& m) noexcept
        {
            if (m.perElement || m.confidence.confirmedBulk8101)
                return "8101VC";

            return "0040VC";
        }

        bool metaHasEnumNamesInRange (const Meta& m) noexcept
        {
            const int lo = juce::jmax (0, m.rawMin);
            const int hi = juce::jmin (127, m.rawMax);

            for (int raw = lo; raw <= hi && raw < 16; ++raw)
            {
                if (m.enumNames[raw] != nullptr && m.enumNames[raw][0] != '\0')
                    return true;
            }

            return false;
        }

        juce::String apiCodecStringToCatalogKind (const juce::String& apiName) noexcept
        {
            if (apiName.isEmpty() || apiName == "identity")
                return "identity";

            if (apiName == "signed" || apiName == "enum")
                return apiName;

            if (apiName.containsIgnoreCase ("signed")
                || apiName.containsIgnoreCase ("Detune")
                || apiName.containsIgnoreCase ("NoteShift")
                || apiName.containsIgnoreCase ("MixerEffectSend")
                || apiName.containsIgnoreCase ("Atpbr")
                || apiName.containsIgnoreCase ("Aftmd"))
                return "signed";

            return "identity";
        }

        juce::String metaCatalogCodecKind (const Meta& m, bool decode) noexcept
        {
            if (metaHasEnumNamesInRange (m))
                return "enum";

            int (*const fn) (int) = decode ? m.decodeRawToUi : m.encodeUiToRaw;

            if (fn == identityDecode || fn == identityEncode)
                return "identity";

            if (fn == decodeAtpbr || fn == encodeAtpbr
                || fn == decodeAftmd
                || fn == decodeEldt || fn == encodeEldt
                || fn == decodeElns || fn == encodeElns
                || fn == decodeEfsdvsns || fn == encodeEfsdvsns)
                return "signed";

            return "identity";
        }

        int metaCatalogUiBound (const Meta& m, bool isMin) noexcept
        {
            const int raw = isMin ? m.rawMin : m.rawMax;

            if (metaCatalogCodecKind (m, true) != "signed")
                return raw;

            if (m.decodeRawToUi == decodeAtpbr)
                return isMin ? -12 : 12;

            if (m.decodeRawToUi == decodeEldt)
                return isMin ? -7 : 7;

            if (m.decodeRawToUi == decodeElns)
                return isMin ? kElnsUiMin : kElnsUiMax;

            if (m.decodeRawToUi == decodeEfsdvsns)
                return isMin ? -7 : 7;

            return raw;
        }

        ParamCatalogEntry metaToCatalogEntry (const Meta& m, int elementIndex) noexcept
        {
            ParamCatalogEntry entry;
            entry.id = m.tag != nullptr ? juce::String (m.tag) : juce::String();
            entry.groupId = expectedCatalogGroupId (m);
            entry.elementIndex = elementIndex;
            entry.rawMin = m.rawMin;
            entry.rawMax = m.rawMax;
            entry.uiMin = metaCatalogUiBound (m, true);
            entry.uiMax = metaCatalogUiBound (m, false);
            entry.decode = metaCatalogCodecKind (m, true);
            entry.encode = metaCatalogCodecKind (m, false);
            entry.sysexTemplate = sysexTemplateFromMetaAddr (m, elementIndex < 0 ? 0 : elementIndex);
            return entry;
        }

        bool parseParamCatalogEntryFromVar (const juce::var& rowVar, ParamCatalogEntry& out) noexcept
        {
            auto* obj = rowVar.getDynamicObject();

            if (obj == nullptr)
                return false;

            const juce::String id = obj->getProperty ("id").toString();

            if (id.isEmpty())
                return false;

            juce::String groupId = obj->getProperty ("groupId").toString();

            if (groupId.isEmpty())
                groupId = obj->getProperty ("group").toString();

            if (groupId.isEmpty())
                return false;

            if (! obj->hasProperty ("rawMin") || ! obj->hasProperty ("rawMax"))
                return false;

            out.id = id;
            out.groupId = groupId;
            out.elementIndex = obj->hasProperty ("elementIndex") ? (int) obj->getProperty ("elementIndex") : -1;
            out.rawMin = (int) obj->getProperty ("rawMin");
            out.rawMax = (int) obj->getProperty ("rawMax");
            out.uiMin = obj->hasProperty ("uiMin") ? (int) obj->getProperty ("uiMin") : out.rawMin;
            out.uiMax = obj->hasProperty ("uiMax") ? (int) obj->getProperty ("uiMax") : out.rawMax;

            if (obj->hasProperty ("enumMap") && obj->getProperty ("enumMap").isObject())
            {
                out.decode = "enum";
                out.encode = "enum";
            }
            else if (obj->hasProperty ("enumNames") && obj->getProperty ("enumNames").isArray())
            {
                bool hasEnum = false;

                if (auto* names = obj->getProperty ("enumNames").getArray())
                {
                    for (const auto& name : *names)
                    {
                        if (name.toString().trim().isNotEmpty())
                        {
                            hasEnum = true;
                            break;
                        }
                    }
                }

                if (hasEnum)
                {
                    out.decode = "enum";
                    out.encode = "enum";
                }
                else
                {
                    const juce::String decodeApi = obj->getProperty ("decode").toString();
                    const juce::String encodeApi = obj->getProperty ("encode").toString();
                    out.decode = apiCodecStringToCatalogKind (decodeApi);
                    out.encode = apiCodecStringToCatalogKind (encodeApi.isEmpty() ? decodeApi : encodeApi);
                }
            }
            else
            {
                const juce::String decodeApi = obj->getProperty ("decode").toString();
                const juce::String encodeApi = obj->getProperty ("encode").toString();
                out.decode = apiCodecStringToCatalogKind (decodeApi.isEmpty() ? "identity" : decodeApi);
                out.encode = apiCodecStringToCatalogKind (encodeApi.isEmpty() ? decodeApi : encodeApi);

                if (out.encode.isEmpty())
                    out.encode = out.decode;
            }

            if (obj->hasProperty ("sysexTemplate"))
            {
                out.sysexTemplate = obj->getProperty ("sysexTemplate").toString().trim();
            }
            else if (obj->hasProperty ("addr"))
            {
                if (auto* addr = obj->getProperty ("addr").getDynamicObject())
                {
                    Meta addrOnly {};
                    addrOnly.addr.b3 = (uint8) (int) addr->getProperty ("b3");
                    addrOnly.addr.b4 = (uint8) (int) addr->getProperty ("b4");
                    addrOnly.addr.b5 = (uint8) (int) addr->getProperty ("b5");
                    addrOnly.addr.b6 = (uint8) (int) addr->getProperty ("b6");
                    const bool perElement = obj->hasProperty ("perElement")
                                              ? (bool) obj->getProperty ("perElement")
                                              : (bool) addr->getProperty ("perElement");
                    addrOnly.perElement = perElement;
                    const int el = out.elementIndex >= 0 ? out.elementIndex : 0;
                    out.sysexTemplate = sysexTemplateFromMetaAddr (addrOnly, el);
                }
            }

            return true;
        }

        std::vector<ParamCatalogEntry> parseParamCatalogFromJsonVar (const juce::var& parsed)
        {
            std::vector<ParamCatalogEntry> out;

            if (! parsed.isArray())
                return out;

            for (const auto& row : *parsed.getArray())
            {
                ParamCatalogEntry entry;

                if (parseParamCatalogEntryFromVar (row, entry))
                    out.push_back (std::move (entry));
            }

            return out;
        }

        const ParamCatalogEntry* findCatalogEntryForMeta (
            const std::map<juce::String, ParamCatalogEntry>& byKey,
            const juce::String& id,
            int elementIndex) noexcept
        {
            if (elementIndex >= 0)
            {
                const auto itExact = byKey.find (catalogEntryKey (id, elementIndex));

                if (itExact != byKey.end())
                    return &itExact->second;
            }

            const auto itCommon = byKey.find (catalogEntryKey (id, -1));

            if (itCommon != byKey.end())
                return &itCommon->second;

            return nullptr;
        }

        void logCatalogFieldMismatch (const juce::String& id,
                                      int elementIndex,
                                      const juce::String& field,
                                      const juce::String& metaVal,
                                      const juce::String& jsonVal,
                                      int& mismatchCount) noexcept
        {
            ++mismatchCount;
            juce::Logger::writeToLog ("[ParamCatalog self-test] id=" + id
                                      + " el=" + formatCatalogElementForLog (elementIndex)
                                      + " " + field + "(meta)=" + metaVal
                                      + " " + field + "(json)=" + jsonVal);
        }

        void logCatalogFieldMismatch (const juce::String& id,
                                      int elementIndex,
                                      const juce::String& field,
                                      int metaVal,
                                      int jsonVal,
                                      int& mismatchCount) noexcept
        {
            logCatalogFieldMismatch (id, elementIndex, field,
                                     juce::String (metaVal), juce::String (jsonVal),
                                     mismatchCount);
        }

        void compareCatalogEntryToMeta (const ParamCatalogEntry& metaSide,
                                        const ParamCatalogEntry& jsonSide,
                                        int elementIndex,
                                        int& mismatchCount) noexcept
        {
            if (jsonSide.id.compareIgnoreCase (metaSide.id) != 0)
            {
                logCatalogFieldMismatch (metaSide.id, elementIndex, "id", metaSide.id, jsonSide.id,
                                         mismatchCount);
            }

            if (jsonSide.rawMin != metaSide.rawMin)
                logCatalogFieldMismatch (metaSide.id, elementIndex, "rawMin", metaSide.rawMin,
                                         jsonSide.rawMin, mismatchCount);

            if (jsonSide.rawMax != metaSide.rawMax)
                logCatalogFieldMismatch (metaSide.id, elementIndex, "rawMax", metaSide.rawMax,
                                         jsonSide.rawMax, mismatchCount);

            if (jsonSide.decode.compareIgnoreCase (metaSide.decode) != 0)
                logCatalogFieldMismatch (metaSide.id, elementIndex, "decode", metaSide.decode,
                                         jsonSide.decode, mismatchCount);

            if (jsonSide.encode.compareIgnoreCase (metaSide.encode) != 0)
                logCatalogFieldMismatch (metaSide.id, elementIndex, "encode", metaSide.encode,
                                         jsonSide.encode, mismatchCount);

            if (jsonSide.groupId.compareIgnoreCase (metaSide.groupId) != 0)
                logCatalogFieldMismatch (metaSide.id, elementIndex, "groupId", metaSide.groupId,
                                         jsonSide.groupId, mismatchCount);
        }

#if JUCE_DEBUG
        juce::File resolveCanonicalParamCatalogFile() noexcept
        {
            juce::File dir = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                                 .getParentDirectory();

            for (int depth = 0; depth < 8; ++depth)
            {
                const juce::File candidate = dir.getChildFile ("ui")
                                                  .getChildFile ("fixtures")
                                                  .getChildFile ("paramCatalog.json");

                if (candidate.existsAsFile())
                    return candidate;

                const juce::File parent = dir.getParentDirectory();

                if (parent == dir)
                    break;

                dir = parent;
            }

            return juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                       .getParentDirectory()
                       .getChildFile ("ui")
                       .getChildFile ("fixtures")
                       .getChildFile ("paramCatalog.json");
        }

        void debugInstallCanonicalParamCatalogIfPresentImpl() noexcept
        {
            const juce::File canonical = resolveCanonicalParamCatalogFile();
            const juce::File dest = paramsMetaJsonFile();

            if (! canonical.existsAsFile())
            {
                juce::Logger::writeToLog ("[ParamCatalog install] skip: missing canonical catalog at "
                                          + canonical.getFullPathName());
                return;
            }

            const juce::File destDir = dest.getParentDirectory();

            if (! destDir.exists())
                destDir.createDirectory();

            if (! canonical.copyFileTo (dest))
            {
                juce::Logger::writeToLog ("[ParamCatalog install] failed to copy to "
                                          + dest.getFullPathName());
                return;
            }

            juce::Logger::writeToLog ("[ParamCatalog install] installed canonical catalog from "
                                      + canonical.getFullPathName());
        }

        void debugSelfTestParamCatalogConsistencyImpl() noexcept
        {
            const juce::File catalogFile = paramsMetaJsonFile();

            if (! catalogFile.existsAsFile())
            {
                juce::Logger::writeToLog ("[ParamCatalog self-test] skip: missing "
                                          + catalogFile.getFullPathName());
                return;
            }

            const juce::var parsed = juce::JSON::parse (catalogFile.loadFileAsString());

            if (parsed.isVoid() || ! parsed.isArray())
            {
                juce::Logger::writeToLog ("[ParamCatalog self-test] failed to parse "
                                          + catalogFile.getFullPathName());
                return;
            }

            const std::vector<ParamCatalogEntry> jsonEntries = parseParamCatalogFromJsonVar (parsed);
            std::map<juce::String, ParamCatalogEntry> jsonByKey;

            for (const auto& entry : jsonEntries)
            {
                const juce::String key = catalogEntryKey (entry.id, entry.elementIndex);
                jsonByKey[key] = entry;
            }

            int mismatchCount = 0;
            int checkedCount = 0;
            bool loggedOkExample = false;
            std::set<juce::String> matchedJsonKeys;

            for (size_t i = 0; i < (size_t) Id::Count; ++i)
            {
                const Meta& m = kMetaTable[i];
                const juce::String id = m.tag != nullptr ? juce::String (m.tag) : juce::String();

                if (m.perElement)
                {
                    for (int el = 0; el < 4; ++el)
                    {
                        const ParamCatalogEntry metaEntry = metaToCatalogEntry (m, -1);
                        const ParamCatalogEntry* jsonEntry = findCatalogEntryForMeta (jsonByKey, id, el);

                        ++checkedCount;

                        if (jsonEntry == nullptr)
                        {
                            ++mismatchCount;
                            juce::Logger::writeToLog ("[ParamCatalog self-test] id=" + id
                                                      + " el=" + juce::String (el)
                                                      + " missing in params_meta.json");
                            continue;
                        }

                        matchedJsonKeys.insert (catalogEntryKey (jsonEntry->id, jsonEntry->elementIndex));

                        const int before = mismatchCount;
                        compareCatalogEntryToMeta (metaEntry, *jsonEntry, el, mismatchCount);

                        if (! loggedOkExample && mismatchCount == before)
                        {
                            loggedOkExample = true;
                            juce::Logger::writeToLog ("[ParamCatalog self-test] id=" + id
                                                      + " el=" + juce::String (el)
                                                      + " rawMin(meta)=" + juce::String (metaEntry.rawMin)
                                                      + " rawMin(json)=" + juce::String (jsonEntry->rawMin)
                                                      + " OK");
                        }
                    }
                }
                else
                {
                    const ParamCatalogEntry metaEntry = metaToCatalogEntry (m, -1);
                    const ParamCatalogEntry* jsonEntry = findCatalogEntryForMeta (jsonByKey, id, -1);

                    ++checkedCount;

                    if (jsonEntry == nullptr)
                    {
                        ++mismatchCount;
                        juce::Logger::writeToLog ("[ParamCatalog self-test] id=" + id
                                                  + " el=- missing in params_meta.json");
                        continue;
                    }

                    matchedJsonKeys.insert (catalogEntryKey (jsonEntry->id, jsonEntry->elementIndex));

                    const int before = mismatchCount;
                    compareCatalogEntryToMeta (metaEntry, *jsonEntry, -1, mismatchCount);

                    if (! loggedOkExample && mismatchCount == before)
                    {
                        loggedOkExample = true;
                        juce::Logger::writeToLog ("[ParamCatalog self-test] id=" + id
                                                  + " el=- groupId(meta)=" + metaEntry.groupId
                                                  + " groupId(json)=" + jsonEntry->groupId
                                                  + " OK");
                    }
                }
            }

            for (const auto& pair : jsonByKey)
            {
                if (matchedJsonKeys.find (pair.first) != matchedJsonKeys.end())
                    continue;

                ++mismatchCount;
                juce::Logger::writeToLog ("[ParamCatalog self-test] extra json entry key="
                                          + pair.first + " id=" + pair.second.id
                                          + " el="
                                          + formatCatalogElementForLog (pair.second.elementIndex));
            }

            juce::Logger::writeToLog ("[ParamCatalog self-test] checked=" + juce::String (checkedCount)
                                      + " mismatches=" + juce::String (mismatchCount));
        }
#endif
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

            /* ELNS: live=elementElnsRaw (sysex 0..127); bulk8101=lmElnsRaw (UI -64..63 after applyBulk). */
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

            case Id::EFSDSCL:
                if (elementIndex == 0)
                {
                    r.live = &s.elementEfsdsclRaw[0];
                    r.bulk8101 = &s.lmEfsdsclRaw;
                }
                break;

            case Id::EFMODE:
                r.live = &s.efmodeRaw;
                r.bulk0040 = &s.lm0040EfmodeRaw;
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

        const int liveRaw = readSlot (refs.live);
        const int bulk8101Raw = readSlot (refs.bulk8101);
        const int bulk0040Raw = readSlot (refs.bulk0040);

        int resolved = -1;

        if (id == Id::ELNS)
        {
            resolved = resolveElnsInt (liveRaw,
                                       m->confidence.confirmedBulk8101 ? bulk8101Raw : -1,
                                       bulk0040ForResolve (*m, bulk0040Raw),
                                       src);
        }
        else if (id == Id::ELDT)
        {
            resolved = resolveEldtInt (liveRaw,
                                       m->confidence.confirmedBulk8101 ? bulk8101Raw : -1,
                                       bulk0040ForResolve (*m, bulk0040Raw),
                                       src);
        }
        else
        {
            resolved = LiveSynthState::resolveInt (liveRaw,
                                                   bulk8101ForResolve (*m, bulk8101Raw),
                                                   bulk0040ForResolve (*m, bulk0040Raw),
                                                   src);
        }

        {
            const juce::ScopedLock lock (gMetaRegistryLock);
            ensureActiveMetaRecordsInitialized();

            const auto idx = static_cast<size_t> (id);

            if (idx < gActiveMetaRecords.size())
            {
                const int truthRaw = truthRawForElement (gActiveMetaRecords[idx], elementIndex);

                if (truthRaw >= 0)
                {
                    const bool bulkAuthoritative = bulkIngestIsAuthoritative (s);
                    const bool presentSource = paramSourceIsPresent (src);

                    if (! (bulkAuthoritative && presentSource) && resolved < 0)
                    {
                        if (gActiveMetaRecords[idx].tag.isNotEmpty())
                        {
                            juce::Logger::writeToLog ("[MetaRegistry] last-saved truth "
                                                      + gActiveMetaRecords[idx].tag
                                                      + " el=" + juce::String (elementIndex)
                                                      + " dump=" + juce::String (resolved)
                                                      + " -> " + juce::String (truthRaw));
                        }

#if JUCE_DEBUG
                        if (id == Id::OUTSEL)
                            debugLogOutselOverwriteImpl ("resolveParam/truth", elementIndex,
                                                           resolved, truthRaw, "resolve");

                        if (isEfsendAuditId (id))
                            debugLogEfsendTruthOverrideImpl (id, elementIndex, resolved, truthRaw);
#endif

                        src = LiveSynthState::ParamSource::None;

#if JUCE_DEBUG
                        if (id == Id::ELDT)
                        {
                            debugLogEldtOverwriteImpl ("resolveParam/truth", elementIndex,
                                                       resolved, truthRaw, "resolve");
                            debugLogEldtResolveImpl (elementIndex, liveRaw, bulk8101Raw,
                                                     bulk0040Raw, truthRaw, src);
                        }

                        if (isEfsendAuditId (id))
                            debugLogEfsendAuditResolveImpl (id, elementIndex, liveRaw, bulk8101Raw,
                                                            bulk0040Raw, truthRaw, src);
#endif

                        return truthRaw;
                    }
                }
            }
        }

#if JUCE_DEBUG
        if (id == Id::OUTSEL)
        {
            juce::Logger::writeToLog ("[OUTSEL audit] stage=resolveParam"
                                      + juce::String (" idx=") + juce::String (elementIndex)
                                      + " liveRaw=" + debugFormatOutselAuditRaw (liveRaw)
                                      + " bulk8101Raw=" + debugFormatOutselAuditRaw (bulk8101Raw)
                                      + " resolved=" + debugFormatOutselAuditRaw (resolved)
                                      + " ui=- baseline=- src="
                                      + juce::String (LiveSynthState::paramSourceTag (src)));
        }

        if (id == Id::ELDT)
            debugLogEldtResolveImpl (elementIndex, liveRaw, bulk8101Raw, bulk0040Raw, resolved, src);

        if (isEfsendAuditId (id))
            debugLogEfsendAuditResolveImpl (id, elementIndex, liveRaw, bulk8101Raw, bulk0040Raw, resolved, src);
#endif

        return resolved;
    }

    void debugLogResolvedParam (const LiveSynthState& s, Id id, int elementIndex,
                                const char* label) noexcept
    {
        const RawRefs refs = refsFor (s, id, elementIndex);
        const int liveRaw = readSlot (refs.live);
        const int bulk8101Raw = readSlot (refs.bulk8101);
        const int bulk0040Raw = readSlot (refs.bulk0040);

        LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
        const int resolvedRaw = resolveParam (s, id, elementIndex, rowSrc);

        const char* srcLabel = "None";

        switch (rowSrc)
        {
            case LiveSynthState::ParamSource::Live:      srcLabel = "Live";      break;
            case LiveSynthState::ParamSource::Bulk8101:  srcLabel = "Bulk8101";  break;
            case LiveSynthState::ParamSource::Bulk0040:  srcLabel = "Bulk0040";  break;
            default:                                     break;
        }

        juce::String idTag;

        if (const Meta* m = metaFor (id); m != nullptr && m->tag != nullptr)
            idTag = m->tag;
        else
            idTag = metaIdToString (id);

        auto formatRaw = [id] (int raw) noexcept
        {
            if (id == Id::ELNS)
                return elnsBulkSlotPresent (raw) || rawSlotPresent (raw)
                           ? juce::String (raw) : juce::String ("-");

            return raw >= 0 ? juce::String (raw) : juce::String ("-");
        };

        juce::Logger::writeToLog ("[Resolve debug] label=" + juce::String (label)
                                  + " id=" + idTag
                                  + " idx=" + juce::String (elementIndex)
                                  + " liveRaw=" + formatRaw (liveRaw)
                                  + " bulk8101Raw=" + formatRaw (bulk8101Raw)
                                  + " bulk0040Raw=" + formatRaw (bulk0040Raw)
                                  + " resolvedRaw=" + formatRaw (resolvedRaw)
                                  + " src=" + srcLabel);
    }

    std::function<void()>& metaRegistryChangedCallback() noexcept
    {
        static std::function<void()> cb;
        return cb;
    }

    void fillEnumComboFromMeta (juce::ComboBox& combo, Id id,
                                juce::NotificationType notify) noexcept
    {
        MetaRecord* rec = activeMetaRecordFor (id);

        if (rec == nullptr)
            return;

        const Meta& meta = rec->meta;
        const int prevRaw = combo.getSelectedItemIndex();
        combo.clear (notify);

        for (int raw = meta.rawMin; raw <= meta.rawMax; ++raw)
        {
            juce::String label = enumLabelAtRaw (*rec, raw);

            if (label.isEmpty())
                label = juce::String (raw);

            combo.addItem (label, raw + 1);
        }

        if (prevRaw >= meta.rawMin && prevRaw <= meta.rawMax)
            combo.setSelectedId (prevRaw + 1, notify);
    }

    bool ingestLiveParameterFrame (LiveSynthState& s,
                                   uint8 b3, uint8 b4, uint8 b5, uint8 b6, uint8 b8) noexcept
    {
        int el = 0;
        const Meta* m = findByLiveAddress (b3, b4, b5, b6, el);

        if (m == nullptr)
            return false;

        const RawRefs refs = refsFor (s, m->id, el);

#if JUCE_DEBUG
        if (m->id == Id::OUTSEL)
            debugWriteOutselSlotImpl (refs.live, (int) b8, "ingestLiveParameterFrame", el, "parse", "live");
        else if (m->id == Id::ELDT)
            debugWriteEldtSlotImpl (refs.live, (int) b8, "ingestLiveParameterFrame", el, "parse");
        else if (m->id == Id::ELVL)
            debugWriteElvlSlotImpl (refs.live, (int) b8, "ingestLiveParameterFrame", el, "parse", "live");
        else if (m->id == Id::ELNS)
            debugWriteElnsSlotImpl (refs.live, (int) b8, "ingestLiveParameterFrame", el, "parse", "live");
        else if (isEfsendAuditId (m->id))
            debugWriteEfsendSlotImpl (refs.live, m->id, (int) b8, "ingestLiveParameterFrame", el, "parse");
        else
#endif
            writeSlot (refs.live, (int) b8);

#if JUCE_DEBUG
        if (m->id == Id::OUTSEL)
            debugLogOutselAuditImpl (s, "live-ingest", el, -1, -1);
        else if (m->id == Id::ELDT)
        {
            LiveSynthState::ParamSource rowSrc = LiveSynthState::ParamSource::None;
            const int resolved = resolveParam (s, Id::ELDT, el, rowSrc);
            debugLogEldtAuditUi ("live-ingest", el, (int) b8, resolved);
        }
#endif

        return true;
    }

    void applyBulk8101FromParsed (LiveSynthState& s,
                                  const YamahaLmVoiceDump::Lm8101VcMinimal& p) noexcept
    {
        s.hasParsedBulk8101 = p.found8101;

        writeSlot (refsFor (s, Id::ELMODE, 0).bulk8101, p.elmodeRaw);

        if (p.elmodeRaw >= 0)
            s.elmodeRaw = -1;

        writeSlot (refsFor (s, Id::WOL, 0).bulk8101, p.wolRaw);
#if JUCE_DEBUG
        debugWriteElvlSlotImpl (refsFor (s, Id::ELVL, 0).bulk8101, p.elvlE1Raw,
                                "applyBulk8101FromParsed", 0, "applyBulk", "bulk8101");
        debugWriteOutselSlotImpl (refsFor (s, Id::OUTSEL, 0).bulk8101, p.lmOutselE1Raw,
                                  "applyBulk8101FromParsed", 0, "applyBulk", "bulk8101");

        if (p.lmOutselE1Raw >= 0)
            debugWriteOutselSlotImpl (&s.elementOutselRaw[0], -1,
                                      "applyBulk8101FromParsed", 0, "applyBulk", "live");
#else
        writeSlot (refsFor (s, Id::ELVL, 0).bulk8101, p.elvlE1Raw);
        writeSlot (refsFor (s, Id::OUTSEL, 0).bulk8101, p.lmOutselE1Raw);

        if (p.lmOutselE1Raw >= 0)
            s.elementOutselRaw[0] = -1;
#endif

        const int elnsUi0 = p.lmElnsRaw[0] >= 0
                                ? YamahaLmVoiceDump::uiFromElementNoteShiftSysex (p.lmElnsRaw[0])
                                : -1;

#if JUCE_DEBUG
        if (elnsUi0 >= kElnsUiMin)
            debugWriteElnsSlotImpl (&s.elementElnsRaw[0], -1,
                                    "applyBulk8101FromParsed", 0, "applyBulk", "live");
#else
        if (elnsUi0 >= kElnsUiMin)
            s.elementElnsRaw[0] = -1;
#endif

#if JUCE_DEBUG
        debugWriteElnsSlotImpl (refsFor (s, Id::ELNS, 0).bulk8101, elnsUi0,
                                "applyBulk8101FromParsed", 0, "applyBulk", "bulk8101");
#else
        writeSlot (refsFor (s, Id::ELNS, 0).bulk8101, elnsUi0);
#endif

        if (p.lmEvllRaw[0] >= 0)
            s.elementEvllRaw[0] = -1;

        if (p.lmEvlhRaw[0] >= 0)
            s.elementEvlhRaw[0] = -1;

        writeSlot (refsFor (s, Id::EVLL, 0).bulk8101, p.lmEvllRaw[0]);
        writeSlot (refsFor (s, Id::EVLH, 0).bulk8101, p.lmEvlhRaw[0]);

        const int eldtUi0 = p.lmEldtRaw[0] >= 0
                                ? YamahaLmVoiceDump::uiFromElementDetuneSysex (p.lmEldtRaw[0])
                                : -1;

        if (p.lmEldtRaw[0] >= 0)
        {
#if JUCE_DEBUG
            debugWriteEldtSlotImpl (&s.elementEldtRaw[0], -1,
                                    "applyBulk8101FromParsed", 0, "applyBulk");
#else
            s.elementEldtRaw[0] = -1;
#endif
        }

#if JUCE_DEBUG
        juce::Logger::writeToLog ("[ELDT audit] stage=applyBulk idx=0 lmRaw="
                                  + debugFormatEldtAuditRaw (p.lmEldtRaw[0]));
        debugWriteEldtSlotImpl (refsFor (s, Id::ELDT, 0).bulk8101, eldtUi0,
                                "applyBulk8101FromParsed", 0, "applyBulk");
#else
        writeSlot (refsFor (s, Id::ELDT, 0).bulk8101, eldtUi0);
#endif

        for (int e = 1; e < 4; ++e)
        {
#if JUCE_DEBUG
            debugWriteElvlSlotImpl (refsFor (s, Id::ELVL, e).bulk8101, p.elvlRaw[e],
                                    "applyBulk8101FromParsed", e, "applyBulk", "bulk8101");
            debugWriteOutselSlotImpl (refsFor (s, Id::OUTSEL, e).bulk8101, p.lmOutselRaw[e],
                                      "applyBulk8101FromParsed", e, "applyBulk", "bulk8101");

            if (p.lmOutselRaw[e] >= 0)
                debugWriteOutselSlotImpl (&s.elementOutselRaw[e], -1,
                                          "applyBulk8101FromParsed", e, "applyBulk", "live");
#else
            writeSlot (refsFor (s, Id::ELVL, e).bulk8101, p.elvlRaw[e]);

            if (p.lmOutselRaw[e] >= 0)
                s.elementOutselRaw[e] = -1;

            writeSlot (refsFor (s, Id::OUTSEL, e).bulk8101, p.lmOutselRaw[e]);
#endif

            const int eldtUi = p.lmEldtRaw[e] >= 0
                                   ? YamahaLmVoiceDump::uiFromElementDetuneSysex (p.lmEldtRaw[e])
                                   : -1;

#if JUCE_DEBUG
            juce::Logger::writeToLog ("[ELDT audit] stage=applyBulk idx=" + juce::String (e)
                                      + " lmRaw=" + debugFormatEldtAuditRaw (p.lmEldtRaw[e]));
            debugWriteEldtSlotImpl (refsFor (s, Id::ELDT, e).bulk8101, eldtUi,
                                    "applyBulk8101FromParsed", e, "applyBulk");
#else
            writeSlot (refsFor (s, Id::ELDT, e).bulk8101, eldtUi);
#endif

            const int elnsUi = p.lmElnsRaw[e] >= 0
                                   ? YamahaLmVoiceDump::uiFromElementNoteShiftSysex (p.lmElnsRaw[e])
                                   : -1;

#if JUCE_DEBUG
            if (elnsUi >= kElnsUiMin)
                debugWriteElnsSlotImpl (&s.elementElnsRaw[e], -1,
                                        "applyBulk8101FromParsed", e, "applyBulk", "live");
            debugWriteElnsSlotImpl (refsFor (s, Id::ELNS, e).bulk8101, elnsUi,
                                    "applyBulk8101FromParsed", e, "applyBulk", "bulk8101");
#else
            if (elnsUi >= kElnsUiMin)
                s.elementElnsRaw[e] = -1;

            writeSlot (refsFor (s, Id::ELNS, e).bulk8101, elnsUi);
#endif

            if (p.lmEvllRaw[e] >= 0)
                s.elementEvllRaw[e] = -1;

            if (p.lmEvlhRaw[e] >= 0)
                s.elementEvlhRaw[e] = -1;

            writeSlot (refsFor (s, Id::EVLL, e).bulk8101, p.lmEvllRaw[e]);
            writeSlot (refsFor (s, Id::EVLH, e).bulk8101, p.lmEvlhRaw[e]);
        }

#if JUCE_DEBUG
        debugLogEfsendApplyBulkImpl (Id::EFLN1EL, 0, p.lmEfln1ElRaw);
        debugLogEfsendApplyBulkImpl (Id::EFSDLV, 0, p.lmEfsdlvRaw);
        debugLogEfsendApplyBulkImpl (Id::EFSDVSNS, 0, p.lmEfsdvlRaw);
        debugLogEfsendApplyBulkImpl (Id::EFSDSCL, 0, p.lmEfsdsclRaw);
        debugWriteEfsendSlotImpl (refsFor (s, Id::EFLN1EL, 0).bulk8101, Id::EFLN1EL,
                                  p.lmEfln1ElRaw, "applyBulk8101FromParsed", 0, "applyBulk");
        debugWriteEfsendSlotImpl (refsFor (s, Id::EFSDLV, 0).bulk8101, Id::EFSDLV,
                                  p.lmEfsdlvRaw, "applyBulk8101FromParsed", 0, "applyBulk");
        debugWriteEfsendSlotImpl (refsFor (s, Id::EFSDVSNS, 0).bulk8101, Id::EFSDVSNS,
                                  p.lmEfsdvlRaw, "applyBulk8101FromParsed", 0, "applyBulk");
        debugWriteEfsendSlotImpl (refsFor (s, Id::EFSDSCL, 0).bulk8101, Id::EFSDSCL,
                                  p.lmEfsdsclRaw, "applyBulk8101FromParsed", 0, "applyBulk");
#else
        writeSlot (refsFor (s, Id::EFLN1EL, 0).bulk8101, p.lmEfln1ElRaw);
        writeSlot (refsFor (s, Id::EFSDLV, 0).bulk8101, p.lmEfsdlvRaw);
        writeSlot (refsFor (s, Id::EFSDVSNS, 0).bulk8101, p.lmEfsdvlRaw);
        writeSlot (refsFor (s, Id::EFSDSCL, 0).bulk8101, p.lmEfsdsclRaw);
#endif

#if JUCE_DEBUG
        debugWriteAuditVnamBufferImpl (s.lmVoiceName, sizeof (s.lmVoiceName), p.name,
                                       sizeof (p.name), "applyBulk8101FromParsed", "applyBulk");
#else
        std::memcpy (s.lmVoiceName, p.name, sizeof (s.lmVoiceName));
#endif

#if JUCE_DEBUG
        debugLogOutselAuditAllImpl (s, "8101-apply", nullptr, nullptr);
#endif
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

        if (p.efmodeRaw >= 0)
            s.efmodeRaw = -1;

        writeSlot (refsFor (s, Id::EFMODE, 0).bulk0040, p.efmodeRaw);
    }

    void mirrorBaselineValue (LiveSynthState& s, Id id, int elementIndex, int value) noexcept
    {
        const Meta* m = metaFor (id);

        if (m == nullptr)
            return;

        const RawRefs refs = refsFor (s, id, elementIndex);

#if JUCE_DEBUG
        if (id == Id::OUTSEL)
        {
            if (refs.live != nullptr)
                debugWriteOutselSlotImpl (refs.live, value, "mirrorBaselineValue", elementIndex, "baseline", "live");

            if (m->confidence.confirmedBulk8101 && refs.bulk8101 != nullptr)
                debugWriteOutselSlotImpl (refs.bulk8101, value, "mirrorBaselineValue", elementIndex, "baseline", "bulk8101");

            if (m->confidence.confirmedBulk0040 && refs.bulk0040 != nullptr)
                debugWriteOutselSlotImpl (refs.bulk0040, value, "mirrorBaselineValue", elementIndex, "baseline", "bulk0040");

            debugLogOutselAuditImpl (s, "baseline-mirror", elementIndex, value, value);
            return;
        }

        if (id == Id::ELVL)
        {
            if (refs.live != nullptr)
                debugWriteElvlSlotImpl (refs.live, value, "mirrorBaselineValue", elementIndex, "baseline", "live");

            if (m->confidence.confirmedBulk8101 && refs.bulk8101 != nullptr)
                debugWriteElvlSlotImpl (refs.bulk8101, value, "mirrorBaselineValue", elementIndex, "baseline", "bulk8101");

            return;
        }

        if (id == Id::ELNS)
        {
            if (refs.live != nullptr)
                debugWriteElnsSlotImpl (refs.live, value, "mirrorBaselineValue", elementIndex, "baseline", "live");

            if (m->confidence.confirmedBulk8101 && refs.bulk8101 != nullptr)
                debugWriteElnsSlotImpl (refs.bulk8101, value, "mirrorBaselineValue", elementIndex, "baseline", "bulk8101");

            return;
        }

        if (id == Id::ELDT)
        {
            if (refs.live != nullptr)
                debugWriteEldtSlotImpl (refs.live, value, "mirrorBaselineValue", elementIndex, "baseline");

            if (m->confidence.confirmedBulk8101 && refs.bulk8101 != nullptr)
                debugWriteEldtSlotImpl (refs.bulk8101, value, "mirrorBaselineValue", elementIndex, "baseline");

            if (m->confidence.confirmedBulk0040 && refs.bulk0040 != nullptr)
                debugWriteEldtSlotImpl (refs.bulk0040, value, "mirrorBaselineValue", elementIndex, "baseline");

            return;
        }

        if (isEfsendAuditId (id))
        {
            if (refs.live != nullptr)
                debugWriteEfsendSlotImpl (refs.live, id, value, "mirrorBaselineValue", elementIndex, "baseline");

            if (m->confidence.confirmedBulk8101 && refs.bulk8101 != nullptr)
                debugWriteEfsendSlotImpl (refs.bulk8101, id, value, "mirrorBaselineValue", elementIndex, "baseline");

            if (m->confidence.confirmedBulk0040 && refs.bulk0040 != nullptr)
                debugWriteEfsendSlotImpl (refs.bulk0040, id, value, "mirrorBaselineValue", elementIndex, "baseline");

            return;
        }
#endif

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

#if JUCE_DEBUG
        debugInstallCanonicalParamCatalogIfPresent();
        debugSelfTestParamCatalogConsistency();
#endif

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
        enforceElvlMetaGuard();

        for (auto& rec : gActiveMetaRecords)
            rec.collapseVerificationTruthOnly();

        juce::Logger::writeToLog ("[MetaRegistry] applied overrides from "
                                  + overrideFile.getFullPathName());
    }

    juce::File paramsMetaJsonFile()
    {
        return appDirPath.getChildFile ("params_meta.json");
    }

    std::vector<ParamCatalogEntry> loadParamCatalogFromJsonFile (const juce::File& f)
    {
        if (! f.existsAsFile())
            return {};

        const juce::var parsed = juce::JSON::parse (f.loadFileAsString());

        if (parsed.isVoid())
            return {};

        return parseParamCatalogFromJsonVar (parsed);
    }

#if JUCE_DEBUG
    namespace
    {
        bool metaOverrideSkippedForSyncLog (const LiveSynthState& s, Id id, int elementIndex,
                                            LiveSynthState::ParamSource src) noexcept
        {
            if (! bulkIngestIsAuthoritative (s) || ! paramSourceIsPresent (src))
                return false;

            const juce::ScopedLock lock (gMetaRegistryLock);
            ensureActiveMetaRecordsInitialized();

            const auto idx = static_cast<size_t> (id);

            if (idx >= gActiveMetaRecords.size())
                return false;

            return truthRawForElement (gActiveMetaRecords[idx], elementIndex) >= 0;
        }

        void debugLogAllCatalogParamsForSyncImpl (const LiveSynthState& s) noexcept
        {
            if (! bulkIngestIsAuthoritative (s))
                return;

            for (size_t i = 0; i < (size_t) Id::Count; ++i)
            {
                const Id id = static_cast<Id> (i);
                const Meta* m = metaFor (id);

                if (m == nullptr)
                    continue;

                const int elementCount = m->perElement ? 4 : 1;

                for (int elementIndex = 0; elementIndex < elementCount; ++elementIndex)
                {
                    LiveSynthState::ParamSource src = LiveSynthState::ParamSource::None;
                    const int resolved = resolveParam (s, id, elementIndex, src);
                    const bool metaOverrideSkipped =
                        metaOverrideSkippedForSyncLog (s, id, elementIndex, src);

                    logSyncAuthoritativeResolve (id, elementIndex, resolved, src, metaOverrideSkipped);
                }
            }
        }
    }

    void debugInstallCanonicalParamCatalogIfPresent() noexcept
    {
        debugInstallCanonicalParamCatalogIfPresentImpl();
    }

    void debugSelfTestParamCatalogConsistency() noexcept
    {
        debugSelfTestParamCatalogConsistencyImpl();
    }

    void debugLogAllCatalogParamsForSync (const LiveSynthState& s) noexcept
    {
        debugLogAllCatalogParamsForSyncImpl (s);
    }

    void debugLogOutselAudit (const LiveSynthState& s, const char* stage, int elementIndex,
                              int uiRaw, int baselineRaw) noexcept
    {
        debugLogOutselAuditImpl (s, stage, elementIndex, uiRaw, baselineRaw);
    }

    void debugLogOutselAuditAll (const LiveSynthState& s, const char* stage,
                                 const int uiByIdx[4], const int baselineByIdx[4]) noexcept
    {
        debugLogOutselAuditAllImpl (s, stage, uiByIdx, baselineByIdx);
    }

    void debugLogOutselOverwrite (const char* writer, int idx, int oldVal, int newVal,
                                  const char* stage) noexcept
    {
        debugLogOutselOverwriteImpl (writer, idx, oldVal, newVal, stage);
    }

    void debugWriteOutselSlot (int* slot, int value, const char* writer, int idx,
                               const char* stage, const char* layer) noexcept
    {
        debugWriteOutselSlotImpl (slot, value, writer, idx, stage, layer);
    }

    void debugLogOutselLiveDecode (uint8 b3, uint8 b4, uint8 b5, uint8 b6, uint8 b8) noexcept
    {
        int el = -1;

        if (! isOutselLiveAddress (b3, b4, b5, b6, el))
            return;

        juce::Logger::writeToLog ("[OUTSEL audit] stage=live-decode"
                                  + juce::String (" idx=") + juce::String (el)
                                  + " liveRaw=- bulk8101Raw=- resolved=- ui=- baseline=-"
                                  + " rawB8=0x"
                                  + juce::String::toHexString ((int) (b8 & 0x7f)).toUpperCase()
                                      .paddedLeft ('0', 2)
                                  + " addr=03/"
                                  + juce::String::toHexString ((int) b4).toUpperCase().paddedLeft ('0', 2)
                                  + "/00/08");
    }
#endif

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

    static juce::var enumNamesToJsonVar (const Meta& m) noexcept
    {
        juce::Array<juce::var> arr;

        for (int i = 0; i < 16; ++i)
            arr.add (m.enumNames[i] != nullptr ? juce::String (m.enumNames[i]) : juce::String());

        return arr;
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

        root->setProperty ("enumNames", enumNamesToJsonVar (m));

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

    namespace
    {
        juce::var metaRecordToJsonVarImpl (const MetaRecord& rec)
        {
            juce::var result = metaToVar (rec.meta);
            auto* root = result.getDynamicObject();

            if (root == nullptr)
                return result;

            if (rec.sy99VerificationByElement.isObject())
            {
                auto* ver = rec.sy99VerificationByElement.getDynamicObject();

                if (ver != nullptr && ver->getProperties().size() > 0)
                    root->setProperty ("sy99Verification", rec.sy99VerificationByElement);
            }

            if (rec.sy99LcdPage > 0)
                root->setProperty ("sy99LcdPage", rec.sy99LcdPage);

            root->setProperty ("enumNames", enumNamesToJsonVarFromRecord (rec));

            return result;
        }
    }

    juce::var metaRecordToJsonVar (Id id)
    {
        const juce::ScopedLock lock (gMetaRegistryLock);
        ensureActiveMetaRecordsInitialized();

        const auto idx = static_cast<size_t> (id);

        if (id >= Id::Count || idx >= gActiveMetaRecords.size())
            return {};

        return metaRecordToJsonVarImpl (gActiveMetaRecords[idx]);
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

        if (id == Id::ELVL)
            enforceElvlMetaGuard();

        if (auto& cb = metaRegistryChangedCallback())
            cb();

        return true;
    }

    void persistActiveMetaRegistry()
    {
        juce::Array<juce::var> rows;

        {
            const juce::ScopedLock lock (gMetaRegistryLock);
            ensureActiveMetaRecordsInitialized();

            for (const auto& rec : gActiveMetaRecords)
                rows.add (metaRecordToJsonVarImpl (rec));
        }

        const juce::File f = paramsMetaJsonFile();
        const juce::File parent = f.getParentDirectory();

        if (parent != juce::File() && ! parent.exists())
            parent.createDirectory();

        f.replaceWithText (juce::JSON::toString (rows, true));
    }

    juce::var allParamsToJsonVar()
    {
        juce::Array<juce::var> rows;
        const juce::ScopedLock lock (gMetaRegistryLock);
        ensureActiveMetaRecordsInitialized();

        for (const auto& rec : gActiveMetaRecords)
            rows.add (metaRecordToJsonVarImpl (rec));

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

        if (MetaRecord* rec = activeMetaRecordFor (id))
        {
            const juce::String enumLabel = enumLabelAtRaw (*rec, raw);

            if (enumLabel.isNotEmpty())
            {
                result.label = enumLabel;
                result.kind = PresentationKind::namedEnum;
                return result;
            }

            const Meta& meta = rec->meta;

            if (meta.decodeRawToUi != nullptr && meta.decodeRawToUi != identityDecode)
            {
                result.label = juce::String (meta.decodeRawToUi (raw));
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

    namespace
    {
        bool valueMapUsesUiCodecRange (const Meta& meta) noexcept
        {
            return meta.encodeUiToRaw != nullptr && meta.encodeUiToRaw != identityEncode
                   && meta.decodeRawToUi != nullptr && meta.decodeRawToUi != identityDecode;
        }

        void appendValueMapRow (juce::Array<juce::var>& rows, int& index, Id id, int elementIndex,
                               const Meta& meta, int raw, int ui, uint8 sysexDeviceByte) noexcept
        {
            uint8 frame[9] = {};

            if (! buildLiveParameterSysexFrame (id, elementIndex, raw, frame, sysexDeviceByte))
                return;

            const ProgramUiLabel program = programUiLabelForRaw (id, raw);
            juce::String registryLabel;

            if (MetaRecord* rec = activeMetaRecordFor (id))
                registryLabel = enumLabelAtRaw (*rec, raw);
            else
                registryLabel = registryEnumLabelForRaw (meta, raw);
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

        if (valueMapUsesUiCodecRange (*meta))
        {
            int uiMin = 0;
            int uiMax = 0;
            bool firstUi = true;

            for (int raw = meta->rawMin; raw <= meta->rawMax; ++raw)
            {
                const int ui = decodeRawToUiValue (*meta, raw);

                if (meta->encodeUiToRaw (ui) != raw)
                    continue;

                if (firstUi)
                {
                    uiMin = uiMax = ui;
                    firstUi = false;
                }
                else
                {
                    uiMin = juce::jmin (uiMin, ui);
                    uiMax = juce::jmax (uiMax, ui);
                }

                appendValueMapRow (rows, index, id, elementIndex, *meta, raw, ui, sysexDeviceByte);
            }

            if (! firstUi)
            {
                root->setProperty ("uiMin", uiMin);
                root->setProperty ("uiMax", uiMax);
            }
        }
        else
        {
            for (int raw = meta->rawMin; raw <= meta->rawMax; ++raw)
            {
                const int ui = decodeRawToUiValue (*meta, raw);
                appendValueMapRow (rows, index, id, elementIndex, *meta, raw, ui, sysexDeviceByte);
            }
        }

        root->setProperty ("rows", rows);
        return root;
    }

    namespace
    {
        struct ParsedLogLine
        {
            int lineIndex = 0;
            int logIndex = -1;
            bool hasLogIndex = false;
            juce::String sysexHex;
            int raw = 0;
            juce::String trailingLabel;
            bool hasTrailingLabel = false;
        };

        juce::String normalizeSysexHex (const juce::String& hex) noexcept
        {
            juce::StringArray tokens;
            tokens.addTokens (hex.trim(), " \t", "");
            juce::String out;

            for (const auto& t : tokens)
            {
                if (t.isEmpty())
                    continue;

                if (out.isNotEmpty())
                    out << ' ';

                out << t.toUpperCase();
            }

            return out;
        }

        int rawFromSysexBytes (const juce::Array<uint8>& bytes) noexcept
        {
            if (bytes.isEmpty())
                return -1;

            const int f0Index = bytes[0] == 0xf0 ? 0 : -1;
            const int dataStart = f0Index >= 0 ? 1 : 0;
            const int dataEnd = (f0Index >= 0 && bytes.getLast() == 0xf7) ? bytes.size() - 1 : bytes.size();
            const int dataLen = dataEnd - dataStart;

            if (dataLen < 1)
                return -1;

            if (dataLen >= 9)
                return (int) (bytes[dataStart + 8] & 0x7f);

            return (int) (bytes[dataEnd - 1] & 0x7f);
        }

        bool isHexChar (juce::juce_wchar c) noexcept
        {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        }

        bool parseHexToken (const juce::String& token, uint8& out) noexcept
        {
            juce::String cleaned;

            for (auto c : token)
                if (isHexChar (c))
                    cleaned << c;

            if (cleaned.isEmpty())
                return false;

            out = (uint8) cleaned.getHexValue32();
            return true;
        }

        juce::var parsedLineToVar (const ParsedLogLine& line)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("lineIndex", line.lineIndex);

            if (line.hasLogIndex)
                obj->setProperty ("logIndex", line.logIndex);

            obj->setProperty ("sysexHex", line.sysexHex);
            obj->setProperty ("raw", line.raw);

            if (line.hasTrailingLabel)
                obj->setProperty ("trailingLabel", line.trailingLabel);

            return obj;
        }

        bool parseObservationLogLine (const juce::String& line, int lineIndex, ParsedLogLine& out) noexcept
        {
            const juce::String trimmed = line.trim();

            if (trimmed.isEmpty() || trimmed.startsWithChar ('#'))
                return false;

            int logIndex = -1;
            bool hasLogIndex = false;

            if (trimmed.startsWithIgnoreCase ("[#"))
            {
                const int close = trimmed.indexOfChar (']');

                if (close > 2)
                {
                    logIndex = trimmed.substring (2, close).getIntValue();
                    hasLogIndex = true;
                }
            }

            const int sysexMarker = trimmed.indexOfIgnoreCase ("SysEx");

            if (sysexMarker < 0)
                return false;

            juce::String afterMarker = trimmed.substring (sysexMarker);
            afterMarker = afterMarker.fromFirstOccurrenceOf (":", false, false).trim();

            const int f0Pos = afterMarker.indexOfIgnoreCase ("F0");
            juce::String payload = f0Pos >= 0 ? afterMarker.substring (f0Pos) : afterMarker;

            const int f7Pos = payload.indexOfIgnoreCase ("F7");
            const juce::String hexPart = f7Pos >= 0 ? payload.substring (0, f7Pos + 2) : payload;
            const juce::String tailPart = f7Pos >= 0 ? payload.substring (f7Pos + 2).trim() : juce::String();

            juce::Array<uint8> bytes;
            juce::StringArray tokens;
            tokens.addTokens (hexPart, " \t", "");

            for (const auto& token : tokens)
            {
                uint8 b = 0;

                if (parseHexToken (token, b))
                    bytes.add (b);
            }

            if (bytes.isEmpty())
                return false;

            juce::String sysexHex;

            for (int i = 0; i < bytes.size(); ++i)
            {
                if (i > 0)
                    sysexHex << ' ';

                sysexHex << juce::String::toHexString ((int) bytes[i]).toUpperCase().paddedLeft ('0', 2);
            }

            int raw = rawFromSysexBytes (bytes);

            if (raw < 0)
                return false;

            out.lineIndex = lineIndex;
            out.sysexHex = normalizeSysexHex (sysexHex);
            out.raw = raw;
            out.hasLogIndex = hasLogIndex;
            out.logIndex = logIndex;

            if (tailPart.isNotEmpty())
            {
                juce::String numbers = tailPart;
                int lastNum = -1;

                for (const auto& t : juce::StringArray::fromTokens (numbers, " \t", ""))
                {
                    if (t.containsOnly ("0123456789-"))
                        lastNum = t.getIntValue();
                }

                if (lastNum >= 0)
                {
                    out.raw = lastNum;
                    out.trailingLabel = tailPart;
                    out.hasTrailingLabel = true;
                }
            }

            return true;
        }

        juce::Array<ParsedLogLine> parseObservationLogText (const juce::String& text)
        {
            juce::Array<ParsedLogLine> parsed;
            juce::StringArray lines;
            lines.addLines (text);

            for (int i = 0; i < lines.size(); ++i)
            {
                ParsedLogLine row;

                if (parseObservationLogLine (lines[i], i, row))
                    parsed.add (row);
            }

            return parsed;
        }

        juce::var findLogDuplicatesToVar (const juce::Array<ParsedLogLine>& logLines)
        {
            juce::Array<juce::var> duplicates;
            std::map<juce::String, int> seenSysex;
            std::map<int, int> seenLogIndex;
            std::map<int, int> seenRaw;

            for (const auto& line : logLines)
            {
                auto pushDup = [&] (const char* reason, int firstLine)
                {
                    auto* obj = new juce::DynamicObject();
                    obj->setProperty ("line", parsedLineToVar (line));
                    obj->setProperty ("reason", reason);
                    obj->setProperty ("firstSeenLineIndex", firstLine);
                    duplicates.add (obj);
                };

                if (line.hasLogIndex)
                {
                    const auto it = seenLogIndex.find (line.logIndex);

                    if (it != seenLogIndex.end())
                        pushDup ("duplicateLogIndex", it->second);
                    else
                        seenLogIndex[line.logIndex] = line.lineIndex;
                }

                const auto itHex = seenSysex.find (line.sysexHex);

                if (itHex != seenSysex.end())
                    pushDup ("duplicateSysex", itHex->second);
                else
                    seenSysex[line.sysexHex] = line.lineIndex;

                const auto itRaw = seenRaw.find (line.raw);

                if (itRaw != seenRaw.end())
                    pushDup ("duplicateRaw", itRaw->second);
                else
                    seenRaw[line.raw] = line.lineIndex;
            }

            return duplicates;
        }

        void applyRowOverridesToMap (juce::var& mapVar, const juce::var& rowOverridesObject) noexcept
        {
            auto* root = mapVar.getDynamicObject();

            if (root == nullptr || ! rowOverridesObject.isObject())
                return;

            auto* overrides = rowOverridesObject.getDynamicObject();
            auto* rows = root->getProperty ("rows").getArray();

            if (rows == nullptr || overrides == nullptr)
                return;

            for (auto& rowVar : *rows)
            {
                auto* row = rowVar.getDynamicObject();

                if (row == nullptr)
                    continue;

                const juce::String rawKey = row->getProperty ("raw").toString();
                const juce::var ovVar = overrides->getProperty (rawKey);
                auto* ov = ovVar.getDynamicObject();

                if (ov == nullptr)
                    continue;

                if (ov->hasProperty ("sysexHex"))
                    row->setProperty ("sysexHex", ov->getProperty ("sysexHex"));

                if (ov->hasProperty ("programLabel"))
                    row->setProperty ("programLabel", ov->getProperty ("programLabel"));

                if (ov->hasProperty ("ui"))
                    row->setProperty ("ui", ov->getProperty ("ui"));

                if (ov->hasProperty ("presentationKind"))
                    row->setProperty ("presentationKind", ov->getProperty ("presentationKind"));
            }
        }

        juce::var compareParsedLogWithMap (const juce::Array<ParsedLogLine>& logLines,
                                           const juce::var& mapVar)
        {
            auto* mapObj = mapVar.getDynamicObject();
            auto* rows = mapObj != nullptr ? mapObj->getProperty ("rows").getArray() : nullptr;

            std::map<int, juce::var> rowByRaw;

            if (rows != nullptr)
            {
                for (const auto& rowVar : *rows)
                {
                    auto* row = rowVar.getDynamicObject();

                    if (row != nullptr)
                        rowByRaw[(int) row->getProperty ("raw")] = rowVar;
                }
            }

            auto* overlays = new juce::DynamicObject();
            juce::Array<juce::var> extras;
            std::set<int> seenRaw;

            for (const auto& logLine : logLines)
            {
                const auto it = rowByRaw.find (logLine.raw);

                if (it == rowByRaw.end())
                {
                    extras.add (parsedLineToVar (logLine));
                    continue;
                }

                auto* mapRow = it->second.getDynamicObject();
                const juce::String mapHex = normalizeSysexHex (mapRow->getProperty ("sysexHex").toString());
                juce::String status = "match";

                if (seenRaw.count (logLine.raw) > 0)
                    status = "duplicateInLog";
                else if (mapHex != logLine.sysexHex)
                    status = "sysexMismatch";
                else if (logLine.hasTrailingLabel)
                {
                    const juce::String prog = mapRow->getProperty ("programLabel").toString().trim();

                    if (logLine.trailingLabel.trim() != prog)
                        status = "labelMismatch";
                }

                seenRaw.insert (logLine.raw);

                const juce::String rawKey = juce::String (logLine.raw);
                auto* existing = overlays->getProperty (rawKey).getDynamicObject();

                if (existing != nullptr
                    && existing->getProperty ("status").toString() == "duplicateInLog")
                    continue;

                if (status == "duplicateInLog" || (existing != nullptr && status == "duplicateInLog"))
                {
                    auto* ov = new juce::DynamicObject();
                    ov->setProperty ("status", "duplicateInLog");

                    if (logLine.hasLogIndex)
                        ov->setProperty ("logIndex", logLine.logIndex);

                    ov->setProperty ("logLabel", logLine.hasTrailingLabel ? logLine.trailingLabel
                                                                         : juce::String (logLine.raw));
                    ov->setProperty ("logSysexHex", logLine.sysexHex);
                    overlays->setProperty (rawKey, ov);
                    continue;
                }

                auto* ov = new juce::DynamicObject();
                ov->setProperty ("status", status);

                if (logLine.hasLogIndex)
                    ov->setProperty ("logIndex", logLine.logIndex);

                ov->setProperty ("logLabel", logLine.hasTrailingLabel ? logLine.trailingLabel
                                                                     : juce::String (logLine.raw));
                ov->setProperty ("logSysexHex", logLine.sysexHex);
                overlays->setProperty (rawKey, ov);
            }

            auto* result = new juce::DynamicObject();
            result->setProperty ("overlaysByRaw", overlays);
            result->setProperty ("extras", extras);
            return result;
        }
    }

    juce::var compareObservationLogToJsonVar (Id id, int elementIndex, uint8 sysexDeviceByte,
                                              const juce::String& logText,
                                              const juce::var& rowOverridesObject) noexcept
    {
        juce::var mapVar = paramValueMapToJsonVar (id, elementIndex, sysexDeviceByte);

        if (mapVar.isVoid())
            return {};

        applyRowOverridesToMap (mapVar, rowOverridesObject);

        const juce::Array<ParsedLogLine> parsed = parseObservationLogText (logText);
        const juce::var comparison = compareParsedLogWithMap (parsed, mapVar);

        auto* root = new juce::DynamicObject();
        root->setProperty ("paramId", metaIdToString (id));
        root->setProperty ("elementIndex", elementIndex);
        root->setProperty ("parsedCount", parsed.size());
        root->setProperty ("duplicates", findLogDuplicatesToVar (parsed));

        if (auto* cmp = comparison.getDynamicObject())
        {
            root->setProperty ("overlaysByRaw", cmp->getProperty ("overlaysByRaw"));
            root->setProperty ("extras", cmp->getProperty ("extras"));
        }

        return root;
    }

    bool focusSy99ParameterEditor (Id id, int elementIndex, uint8 sysexDeviceByte,
                                   int rawOverride, juce::String& errorOut) noexcept
    {
        errorOut = {};

        const Meta* meta = metaFor (id);

        if (meta == nullptr)
        {
            errorOut = "parameter not found";
            return false;
        }

        elementIndex = meta->perElement ? juce::jlimit (0, 3, elementIndex) : 0;

        int raw = rawOverride;

        if (raw < 0)
        {
            LiveSynthState::ParamSource src = LiveSynthState::ParamSource::None;
            raw = resolveParam (getLiveSynthState(), id, elementIndex, src);
        }

        if (raw < 0)
        {
            errorOut = "no value: sync voice from SY99 or pass raw in request body";
            return false;
        }

        raw = juce::jlimit (meta->rawMin, meta->rawMax, raw);

        uint8 frame[9] = {};

        if (! buildLiveParameterSysexFrame (id, elementIndex, raw, frame, sysexDeviceByte))
        {
            errorOut = "failed to build SysEx frame";
            return false;
        }

        if (! Sy99HardwareMappingRuntime::sendParameterSysexFrame (frame))
        {
            errorOut = "MIDI output not available (open Sysex77 and select MIDI out)";
            return false;
        }

        return true;
    }

#if JUCE_DEBUG
    void debugLogEldtAuditUi (const char* stage, int elementIndex, int controlValue,
                              int rawUsed) noexcept
    {
        juce::Logger::writeToLog ("[ELDT audit] stage=" + juce::String (stage)
                                  + " idx=" + juce::String (elementIndex)
                                  + " controlValue=" + juce::String (controlValue)
                                  + " rawUsed=" + juce::String (rawUsed));
    }

    void debugLogEfsendAuditUi (Id id, int elementIndex, int controlValue, int rawUsed) noexcept
    {
        juce::Logger::writeToLog ("[EFSEND audit] stage=ui field=" + juce::String (efsendFieldTag (id))
                                  + " idx=" + juce::String (elementIndex)
                                  + " controlValue=" + juce::String (controlValue)
                                  + " rawUsed=" + debugFormatEfsendAuditRaw (rawUsed));
    }

    void debugLogEfsendAuditBaseline (Id id, int elementIndex, int baselineRaw) noexcept
    {
        juce::Logger::writeToLog ("[EFSEND audit] stage=baseline field=" + juce::String (efsendFieldTag (id))
                                  + " idx=" + juce::String (elementIndex)
                                  + " baselineRaw=" + debugFormatEfsendAuditRaw (baselineRaw));
    }

    namespace
    {
        juce::String debugSyncInspectFormatInt (int raw) noexcept
        {
            return raw >= 0 ? juce::String (raw) : juce::String ("-");
        }

        juce::String debugSyncInspectFormatIntArray4 (const int values[4]) noexcept
        {
            juce::String a = "[";

            for (int i = 0; i < 4; ++i)
            {
                if (i > 0)
                    a << ",";

                a << debugSyncInspectFormatInt (values[i]);
            }

            return a + "]";
        }

        int debugSyncInspectReadSlot (const int* slot) noexcept
        {
            return slot != nullptr ? *slot : -1;
        }

        void debugSyncInspectCollectRow4 (const LiveSynthState& s, Id id,
                                          int liveOut[4], int bulkOut[4],
                                          int resolvedOut[4], int baselineOut[4]) noexcept
        {
            for (int e = 0; e < 4; ++e)
            {
                const RawRefs refs = refsFor (s, id, e);
                liveOut[e] = debugSyncInspectReadSlot (refs.live);
                bulkOut[e] = debugSyncInspectReadSlot (refs.bulk8101);
                baselineOut[e] = bulkOut[e];

                LiveSynthState::ParamSource src = LiveSynthState::ParamSource::None;
                resolvedOut[e] = resolveParam (s, id, e, src);
            }
        }

        void debugSyncInspectAppendEfsendLine (const LiveSynthState& s, Id id, const char* tag,
                                               juce::String& block) noexcept
        {
            const RawRefs refs = refsFor (s, id, 0);
            const int live = debugSyncInspectReadSlot (refs.live);
            const int bulk = debugSyncInspectReadSlot (refs.bulk8101);
            const int baseline = bulk;

            LiveSynthState::ParamSource src = LiveSynthState::ParamSource::None;
            const int resolved = resolveParam (s, id, 0, src);

            block << "\n " << tag << " E1: live=" << debugSyncInspectFormatInt (live)
                  << " bulk=" << debugSyncInspectFormatInt (bulk)
                  << " resolved=" << debugSyncInspectFormatInt (resolved)
                  << " baseline=" << debugSyncInspectFormatInt (baseline);
        }
    }

    void debugLogTrackedOverwriteSnapshot (const LiveSynthState& s, const char* tag) noexcept
    {
        debugLogTrackedOverwriteSnapshotImpl (s, tag);
    }

    void debugWriteElvlSlot (int* slot, int value, const char* writer, int idx,
                             const char* stage, const char* layer) noexcept
    {
        debugWriteElvlSlotImpl (slot, value, writer, idx, stage, layer);
    }

    void debugWriteElnsSlot (int* slot, int value, const char* writer, int idx,
                             const char* stage, const char* layer) noexcept
    {
        debugWriteElnsSlotImpl (slot, value, writer, idx, stage, layer);
    }

    void debugWriteAuditCharWithStage (char* slot, int idx, char value, const char* writer,
                                       const char* stage) noexcept
    {
        debugWriteAuditCharSlotImpl (slot, idx, value, writer, stage);
    }

    void debugWriteAuditVnamBufferWithStage (char* dest, size_t destSize, const char* src,
                                             size_t srcLen, const char* writer,
                                             const char* stage) noexcept
    {
        debugWriteAuditVnamBufferImpl (dest, destSize, src, srcLen, writer, stage);
    }

    void debugDumpAuditedSyncState (const LiveSynthState& s, int slot,
                                    const char* editorVnam) noexcept
    {
        const juce::String lm8101 = juce::String (s.lmVoiceName).trimEnd();
        juce::String editor;

        if (editorVnam != nullptr && editorVnam[0] != '\0')
            editor = juce::String (editorVnam).trimEnd();
        else if (s.voiceNameCharCount > 0)
            editor = juce::String (s.voiceName, s.voiceNameCharCount).trimEnd();
        else
            editor = "-";

        const juce::String baseline = lm8101;

        int live[4] {}, bulk[4] {}, resolved[4] {}, baselineRow[4] {};

        debugSyncInspectCollectRow4 (s, Id::OUTSEL, live, bulk, resolved, baselineRow);
        juce::String block = "[SYNC INSPECT] slot=" + juce::String (slot)
                             + "\n VNAM: lm8101=\"" + lm8101 + "\" editor=\"" + editor
                             + "\" baseline=\"" + baseline + "\""
                             + "\n OUTSEL E1..E4: live=" + debugSyncInspectFormatIntArray4 (live)
                             + " bulk=" + debugSyncInspectFormatIntArray4 (bulk)
                             + " resolved=" + debugSyncInspectFormatIntArray4 (resolved)
                             + " baseline=" + debugSyncInspectFormatIntArray4 (baselineRow);

        debugSyncInspectCollectRow4 (s, Id::ELDT, live, bulk, resolved, baselineRow);
        block << "\n ELDT   E1..E4: live=" << debugSyncInspectFormatIntArray4 (live)
              << " bulk=" << debugSyncInspectFormatIntArray4 (bulk)
              << " resolved=" << debugSyncInspectFormatIntArray4 (resolved)
              << " baseline=" << debugSyncInspectFormatIntArray4 (baselineRow);

        debugSyncInspectAppendEfsendLine (s, Id::EFLN1EL, "EFLN1EL", block);
        debugSyncInspectAppendEfsendLine (s, Id::EFSDLV, "EFSDLV", block);
        debugSyncInspectAppendEfsendLine (s, Id::EFSDVSNS, "EFSDVSNS", block);
        debugSyncInspectAppendEfsendLine (s, Id::EFSDSCL, "EFSDSCL", block);

        juce::Logger::writeToLog (block);
    }

    void debugLogEldtOverwrite (const char* writer, int idx, int oldVal, int newVal,
                                const char* stage) noexcept
    {
        debugLogEldtOverwriteImpl (writer, idx, oldVal, newVal, stage);
    }

    void debugWriteEldtSlot (int* slot, int value, const char* writer, int idx,
                             const char* stage) noexcept
    {
        debugWriteEldtSlotImpl (slot, value, writer, idx, stage);
    }

    void debugWriteAuditInt (int* slot, const char* field, int idx, int value,
                             const char* writer, const char* stage) noexcept
    {
        debugWriteAuditIntSlotImpl (slot, field, idx, value, writer, stage);
    }

    void debugWriteAuditChar (char* slot, int idx, char value, const char* writer) noexcept
    {
        debugWriteAuditCharSlotImpl (slot, idx, value, writer, "UI");
    }

    void debugWriteAuditVnamBuffer (char* dest, size_t destSize, const char* src, size_t srcLen,
                                    const char* writer) noexcept
    {
        debugWriteAuditVnamBufferImpl (dest, destSize, src, srcLen, writer, "baseline");
    }

    void debugWriteEfsendSlot (int* slot, Id id, int value, const char* writer, int idx,
                               const char* stage) noexcept
    {
        debugWriteEfsendSlotImpl (slot, id, value, writer, idx, stage);
    }

    void debugLogUiBindingAudit (const char* control, int elementIndex, const char* source,
                                 Id id, int raw, int ui) noexcept
    {
        const char* idTag = "?";
        if (const Meta* m = metaFor (id))
            idTag = m->tag;

        juce::Logger::writeToLog ("[UI binding audit] control=" + juce::String (control)
                                  + " idx=" + juce::String (elementIndex)
                                  + " source=" + juce::String (source)
                                  + " id=" + juce::String (idTag)
                                  + " raw=" + juce::String (raw)
                                  + " ui=" + juce::String (ui));
    }

    void debugLogUiBindingAuditWrite (const char* control, int elementIndex, const char* dest,
                                      Id id, int ui) noexcept
    {
        const char* idTag = "?";
        if (const Meta* m = metaFor (id))
            idTag = m->tag;

        juce::Logger::writeToLog ("[UI binding audit] control=" + juce::String (control)
                                  + " idx=" + juce::String (elementIndex)
                                  + " direction=write dest=" + juce::String (dest)
                                  + " id=" + juce::String (idTag)
                                  + " ui=" + juce::String (ui));
    }

    void debugAuditLiveStateReset (LiveSynthState& s) noexcept
    {
        if (s.elementLevelRaw[0] != -1)
            debugLogWriteAuditImpl ("ELVL-live", 0, "LiveSynthState::reset", "reset",
                                    debugFormatWriteAuditInt (s.elementLevelRaw[0]), "-");
        if (s.lmElvlE1Raw != -1)
            debugLogWriteAuditImpl ("ELVL", 0, "LiveSynthState::reset", "reset",
                                    debugFormatWriteAuditInt (s.lmElvlE1Raw), "-");
        if (s.elementElnsRaw[0] != -1)
            debugLogWriteAuditImpl ("ELNS-live", 0, "LiveSynthState::reset", "reset",
                                    debugFormatWriteAuditInt (s.elementElnsRaw[0]), "-");
        if (s.lmElnsRaw[0] != -1)
            debugLogWriteAuditImpl ("ELNS", 0, "LiveSynthState::reset", "reset",
                                    debugFormatWriteAuditInt (s.lmElnsRaw[0]), "-");

        for (int i = 0; i < 4; ++i)
        {
            if (s.elementOutselRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("OUTSEL", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditInt (s.elementOutselRaw[(size_t) i]), "-");
            if (s.lmOutselRaw[i] != -1)
                debugLogWriteAuditImpl ("OUTSEL", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditInt (s.lmOutselRaw[i]), "-");
            if (s.elementEldtRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("ELDT", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditInt (s.elementEldtRaw[(size_t) i]), "-");
            if (s.lmEldtRaw[i] != -1)
                debugLogWriteAuditImpl ("ELDT", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditInt (s.lmEldtRaw[i]), "-");
            if (s.elementEfsendselRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("EFLN1EL", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditInt (s.elementEfsendselRaw[(size_t) i]), "-");
            if (s.elementEfsendlvlRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("EFSDLV", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditInt (s.elementEfsendlvlRaw[(size_t) i]), "-");
            if (s.elementEfsdvsnsRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("EFSDVSNS", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditInt (s.elementEfsdvsnsRaw[(size_t) i]), "-");
            if (s.elementEfsdsclRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("EFSDSCL", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditInt (s.elementEfsdsclRaw[(size_t) i]), "-");
        }

        if (s.lmOutselE1Raw != -1)
            debugLogWriteAuditImpl ("OUTSEL", 0, "LiveSynthState::reset", "reset",
                                    debugFormatWriteAuditInt (s.lmOutselE1Raw), "-");
        if (s.lmEfln1ElRaw != -1)
            debugLogWriteAuditImpl ("EFLN1EL", 0, "LiveSynthState::reset", "reset",
                                    debugFormatWriteAuditInt (s.lmEfln1ElRaw), "-");
        if (s.lmEfsdlvRaw != -1)
            debugLogWriteAuditImpl ("EFSDLV", 0, "LiveSynthState::reset", "reset",
                                    debugFormatWriteAuditInt (s.lmEfsdlvRaw), "-");
        if (s.lmEfsdvlRaw != -1)
            debugLogWriteAuditImpl ("EFSDVSNS", 0, "LiveSynthState::reset", "reset",
                                    debugFormatWriteAuditInt (s.lmEfsdvlRaw), "-");
        if (s.lmEfsdsclRaw != -1)
            debugLogWriteAuditImpl ("EFSDSCL", 0, "LiveSynthState::reset", "reset",
                                    debugFormatWriteAuditInt (s.lmEfsdsclRaw), "-");

        for (int i = 0; i < 11; ++i)
        {
            if (s.voiceName[i] != '\0')
                debugLogWriteAuditImpl ("VNAM", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditChar (s.voiceName[i]), "-");
            if (s.lmVoiceName[i] != '\0')
                debugLogWriteAuditImpl ("VNAM", i, "LiveSynthState::reset", "reset",
                                        debugFormatWriteAuditChar (s.lmVoiceName[i]), "-");
        }
    }

    void debugAuditLiveStateClearParam9 (LiveSynthState& s) noexcept
    {
        for (int i = 0; i < 10; ++i)
        {
            if (s.voiceName[i] != '\0')
                debugLogWriteAuditImpl ("VNAM", i, "clearLiveParam9Overrides", "clearLive",
                                        debugFormatWriteAuditChar (s.voiceName[i]), "-");
        }

        for (int i = 0; i < 4; ++i)
        {
            if (s.elementLevelRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl (i == 0 ? "ELVL-live" : "ELVL", i, "clearLiveParam9Overrides",
                                        "clearLive",
                                        debugFormatWriteAuditInt (s.elementLevelRaw[(size_t) i]), "-");
            if (s.elementElnsRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl (i == 0 ? "ELNS-live" : "ELNS", i, "clearLiveParam9Overrides",
                                        "clearLive",
                                        debugFormatWriteAuditInt (s.elementElnsRaw[(size_t) i]), "-");
            if (s.elementOutselRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("OUTSEL", i, "clearLiveParam9Overrides", "clearLive",
                                        debugFormatWriteAuditInt (s.elementOutselRaw[(size_t) i]), "-");
            if (s.elementEldtRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("ELDT", i, "clearLiveParam9Overrides", "clearLive",
                                        debugFormatWriteAuditInt (s.elementEldtRaw[(size_t) i]), "-");
            if (s.elementEfsendselRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("EFLN1EL", i, "clearLiveParam9Overrides", "clearLive",
                                        debugFormatWriteAuditInt (s.elementEfsendselRaw[(size_t) i]), "-");
            if (s.elementEfsendlvlRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("EFSDLV", i, "clearLiveParam9Overrides", "clearLive",
                                        debugFormatWriteAuditInt (s.elementEfsendlvlRaw[(size_t) i]), "-");
            if (s.elementEfsdvsnsRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("EFSDVSNS", i, "clearLiveParam9Overrides", "clearLive",
                                        debugFormatWriteAuditInt (s.elementEfsdvsnsRaw[(size_t) i]), "-");
            if (s.elementEfsdsclRaw[(size_t) i] != -1)
                debugLogWriteAuditImpl ("EFSDSCL", i, "clearLiveParam9Overrides", "clearLive",
                                        debugFormatWriteAuditInt (s.elementEfsdsclRaw[(size_t) i]), "-");
        }
    }
#endif

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
#if JUCE_DEBUG
        Sy99ParamRegistry::debugWriteAuditCharWithStage (&voiceName[idx], idx, (char) (b8 & 0x7f),
                                                         "ingestParameterFrame", "parse");
#else
        voiceName[idx] = (char) (b8 & 0x7f);
#endif
        voiceNameCharCount = juce::jmax (voiceNameCharCount, idx + 1);
    }
}

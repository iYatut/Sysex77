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
#include <functional>
#include <vector>

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
        EFSDSCL,
        EFMODE,
        Count
    };

    /** Send bit for El.1 effect send line (SY99 has Send 1 and Send 3 only). */
    static constexpr uint8 kEffectSend1Bit = 0x01;
    static constexpr uint8 kEffectSend3Bit = 0x04;

    /** Whether a send line is enabled for UI given EFMODE raw (0=Off, 1=Serial, 2=Parallel). */
    inline bool effectSendLineEnabled (int efmodeRaw, uint8 sendBit) noexcept
    {
        if (efmodeRaw < 0)
            return false;

        if (efmodeRaw == 0)
            return false;

        if (efmodeRaw == 1)
            return sendBit == kEffectSend1Bit;

        if (efmodeRaw == 2)
            return sendBit == kEffectSend1Bit || sendBit == kEffectSend3Bit;

        return false;
    }

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
        bool            perElement;
        const char*     uiLabel;
        const char*     group;
        Address         addr;
        int             rawMin;
        int             rawMax;
        int             (*decodeRawToUi) (int raw);
        int             (*encodeUiToRaw) (int ui);
        const char*     enumNames[16];
        ConfidenceFlags confidence;
    };

    /**
        Одна строка чистого каталога params_meta.json (ParamCatalogEntry).
        elementIndex < 0 — common или одна запись без per-element индекса.
    */
    struct ParamCatalogEntry
    {
        juce::String id;
        juce::String groupId;
        int          elementIndex = -1;
        int          rawMin         = 0;
        int          rawMax         = 127;
        int          uiMin          = 0;
        int          uiMax          = 127;
        juce::String decode;  /** "identity" | "signed" | "enum" */
        juce::String encode;
        juce::String sysexTemplate;
    };

    /** Load params_meta.json catalog rows; truth / sy99Verification fields ignored. */
    std::vector<ParamCatalogEntry> loadParamCatalogFromJsonFile (const juce::File& f);

#if JUCE_DEBUG
    /** Copy ui/fixtures/paramCatalog.json into params_meta.json when found (Debug only). */
    void debugInstallCanonicalParamCatalogIfPresent() noexcept;

    /** Compare compiled kMetaTable with params_meta.json (Debug builds only). */
    void debugSelfTestParamCatalogConsistency() noexcept;

    /** One [Sync authoritative] line per catalog Id (and per-element idx) after Sync from SY99. */
    void debugLogAllCatalogParamsForSync (const LiveSynthState& s) noexcept;

    /** [OUTSEL audit] one line: live/bulk8101/resolved (+ optional ui/baseline) for idx 0..3. */
    void debugLogOutselAudit (const LiveSynthState& s, const char* stage, int elementIndex,
                              int uiRaw = -1, int baselineRaw = -1) noexcept;

    void debugLogOutselAuditAll (const LiveSynthState& s, const char* stage,
                                 const int uiByIdx[4] = nullptr,
                                 const int baselineByIdx[4] = nullptr) noexcept;

    void debugLogOutselOverwrite (const char* writer, int idx, int oldVal, int newVal,
                                  const char* stage) noexcept;

    void debugLogOutselLiveDecode (uint8 b3, uint8 b4, uint8 b5, uint8 b6, uint8 b8) noexcept;

    /** [ELDT audit] stage=ui|baseline — controlValue vs rawUsed/baselineRaw. */
    void debugLogEldtAuditUi (const char* stage, int elementIndex, int controlValue,
                              int rawUsed) noexcept;

    void debugLogEldtOverwrite (const char* writer, int idx, int oldVal, int newVal,
                                const char* stage) noexcept;

    void debugWriteEldtSlot (int* slot, int value, const char* writer, int idx,
                             const char* stage) noexcept;

    /** [EFSEND audit] stage=ui — controlValue vs resolve rawUsed. */
    void debugLogEfsendAuditUi (Id id, int elementIndex, int controlValue, int rawUsed) noexcept;

    /** [EFSEND audit] stage=baseline — snapshot written to lm* / element* slots. */
    void debugLogEfsendAuditBaseline (Id id, int elementIndex, int baselineRaw) noexcept;

    /**
        One compact [SYNC INSPECT] block for audited sync params (VNAM, OUTSEL, ELDT, EFSEND×4).
        editorVnam: optional editName text; when null, falls back to live voiceName buffer.
    */
    void debugDumpAuditedSyncState (const LiveSynthState& s, int slot,
                                    const char* editorVnam = nullptr) noexcept;

    /**
        Compact tracked-field snapshot + last-writer table. tag=StopSync captures golden bulk8101;
        later tags compare and log [OVERWRITE REGRESS] when bulk values diverge.
    */
    void debugLogTrackedOverwriteSnapshot (const LiveSynthState& s, const char* tag) noexcept;

    void debugWriteOutselSlot (int* slot, int value, const char* writer, int idx,
                               const char* stage, const char* layer = "bulk8101") noexcept;

    void debugWriteElvlSlot (int* slot, int value, const char* writer, int idx,
                             const char* stage, const char* layer = "bulk8101") noexcept;

    void debugWriteElnsSlot (int* slot, int value, const char* writer, int idx,
                             const char* stage, const char* layer = "bulk8101") noexcept;

    void debugWriteAuditCharWithStage (char* slot, int idx, char value, const char* writer,
                                       const char* stage) noexcept;

    void debugWriteAuditVnamBufferWithStage (char* dest, size_t destSize, const char* src,
                                             size_t srcLen, const char* writer,
                                             const char* stage) noexcept;

    /** [WRITE audit] seq=<n> field=<tag> idx=<i> writer=<fn> stage=<…> old=<…> new=<…> */
    void debugWriteAuditInt (int* slot, const char* field, int idx, int value,
                             const char* writer, const char* stage = "?") noexcept;

    void debugWriteAuditChar (char* slot, int idx, char value, const char* writer) noexcept;

    void debugWriteAuditVnamBuffer (char* dest, size_t destSize, const char* src, size_t srcLen,
                                    const char* writer) noexcept;

    void debugWriteEfsendSlot (int* slot, Id id, int value, const char* writer, int idx,
                               const char* stage) noexcept;

    /** Log audited slot clears immediately before LiveSynthState::reset(). */
    void debugAuditLiveStateReset (LiveSynthState& s) noexcept;

    /** Log audited slot clears immediately before clearLiveParam9Overrides(). */
    void debugAuditLiveStateClearParam9 (LiveSynthState& s) noexcept;

    /** [UI binding audit] apply path: control display source during applyLiveSynthStateToEditor. */
    void debugLogUiBindingAudit (const char* control, int elementIndex, const char* source,
                                 Id id, int raw, int ui) noexcept;

    /** [UI binding audit] write path: outbound edit destination from editor controls. */
    void debugLogUiBindingAuditWrite (const char* control, int elementIndex, const char* dest,
                                      Id id, int ui) noexcept;
#endif

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

    /** live > confirmed bulk8101 > confirmed bulk0040.
        When hasParsedBulk8101/0040: dump slots win; canonicalRaw truth is not applied. */
    int resolveParam (const LiveSynthState& s, Id id, int elementIndex,
                      LiveSynthState::ParamSource& src) noexcept;

    /** One-line resolve trace: underlying live/bulk slots vs resolveParam result. */
    void debugLogResolvedParam (const LiveSynthState& s, Id id, int elementIndex,
                                const char* label) noexcept;

    /** Rebuild a ComboBox from active meta enumNames (item id = raw + 1). */
    void fillEnumComboFromMeta (juce::ComboBox& combo, Id id,
                                juce::NotificationType notify = juce::dontSendNotification) noexcept;

    /** Fired after applyMetaPatch / persist (wire from MidiDemo → VoicePage). */
    std::function<void()>& metaRegistryChangedCallback() noexcept;

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

    juce::String metaIdToString (Id id);
    juce::String codecFnToString (int (*fn) (int));
    juce::var metaToVar (const Meta& m);

    /** Meta fields plus optional sy99Verification (per element) from params_meta.json. */
    juce::var metaRecordToJsonVar (Id id);

    /** Parse enum id string, e.g. "ELMODE". Returns Id::Count if unknown. */
    Id idFromString (const juce::String& idStr);

    /** Default + optional appDirPath/params_meta.json overrides. Safe to call once at startup. */
    void initializeMetaRegistryAtStartup();

    juce::File paramsMetaJsonFile();

    std::vector<Meta> loadMetaFromJsonFile (const juce::File& f);
    void saveMetaToJsonFile (const juce::File& f, const std::vector<Meta>& metas);

    std::vector<Meta> defaultMetaTable();

    /** Merge editable fields from patch (uiLabel, group, enumNames, rawMin, rawMax, decode, encode). */
    bool applyMetaPatch (Id id, const juce::var& patchObject);

    /** Write active registry to params_meta.json. */
    void persistActiveMetaRegistry();

    /** Build 9-byte `43 nn 34 …` live parameter frame from registry Meta. */
    bool buildLiveParameterSysexFrame (Id id, int elementIndex, int rawValue,
                                       uint8 outFrame[9], uint8 sysexDeviceByte) noexcept;

    juce::var allParamsToJsonVar();
    juce::var currentLiveDumpToJsonVar();

    enum class PresentationKind
    {
        numeric,
        namedEnum,
        decoded
    };

    juce::String presentationKindToString (PresentationKind kind) noexcept;

    /** F0 + 9 data bytes + F7, uppercase hex pairs separated by spaces. */
    juce::String formatLiveSysexFrameHex (const uint8 frame[9]) noexcept;

    int decodeRawToUiValue (const Meta& meta, int raw) noexcept;

    struct ProgramUiLabel
    {
        juce::String label;
        PresentationKind kind = PresentationKind::numeric;
    };

    /** Mirrors Voice.h combo/slider/enum presentation for a raw value. */
    ProgramUiLabel programUiLabelForRaw (Id id, int raw) noexcept;

    juce::String registryEnumLabelForRaw (const Meta& meta, int raw) noexcept;

    /** Full ordered value map for API (rawMin..rawMax). */
    juce::var paramValueMapToJsonVar (Id id, int elementIndex,
                                      uint8 sysexDeviceByte = 0x10) noexcept;

    /**
        Parse SY99 observation log and compare with value map (registry + optional rowOverrides).
        rowOverridesObject: { "rawStr": { sysexHex?, programLabel?, ui?, presentationKind? } }.
    */
    juce::var compareObservationLogToJsonVar (Id id, int elementIndex, uint8 sysexDeviceByte,
                                              const juce::String& logText,
                                              const juce::var& rowOverridesObject) noexcept;

    /**
        Re-send current resolved raw on the wire to open the SY99 editor page for this parameter
        without intentionally changing the value. Returns false if no live value or MIDI unavailable.
    */
    bool focusSy99ParameterEditor (Id id, int elementIndex, uint8 sysexDeviceByte,
                                   int rawOverride, juce::String& errorOut) noexcept;

    enum class DumpCompareStatus : uint8
    {
        Match,
        Mismatch,
        Missing
    };

    juce::String dumpCompareStatusToString (DumpCompareStatus status) noexcept;

    struct DumpCompareRow
    {
        Id               id = Id::Count;
        int              elementIndex = 0;
        juce::String     group;
        juce::String     paramTag;
        juce::String     addressHex;
        int              raw8101 = -1;
        int              raw0040 = -1;
        int              rawLive = -1;
        int              resolved = -1;
        DumpCompareStatus status = DumpCompareStatus::Missing;
    };

    std::vector<DumpCompareRow> buildDumpCompareRows (const LiveSynthState& s);

    juce::var libraryDumpCompareToJsonVar();

    struct LibraryVoiceParamRow
    {
        juce::String     paramId;
        juce::String     registryId;
        Id               id = Id::Count;
        int              elementIndex = 0;
        int              sysexGroup = 0;
        juce::String     group;
        juce::String     uiLabel;
        juce::String     paramTag;
        juce::String     addressHex;
        int              sy99LcdPage = 0;
        juce::String     bindStatus;
        int              raw8101 = -1;
        int              raw0040 = -1;
        int              uiValue = -1;
        juce::String     valueLabel;
        bool             inDump = false;
        juce::String     autoCheckStatus;
        juce::String     autoCheckMessage;
        int              expectedUi = -1;
        juce::String     fixtureId;
    };

    struct LibraryCatalogStats
    {
        int totalRows = 0;
        int catalogParams = 0;
        int inDump = 0;
        int confirmed = 0;
        int manualOnly = 0;
        int candidate = 0;
    };

    LibraryCatalogStats computeLibraryCatalogStats (const std::vector<LibraryVoiceParamRow>& rows);
    juce::var libraryCatalogStatsToJsonVar (const LibraryCatalogStats& stats);
    juce::var libraryGlobalCatalogStatsToJsonVar();

    std::vector<LibraryVoiceParamRow> buildLibraryVoiceParamRows (
        const LiveSynthState& s,
        bool inDumpOnly = false,
        const YamahaLmVoiceDump::Lm8101VcMinimal* parsed8101 = nullptr,
        const YamahaLmVoiceDump::Lm0040VcMinimal* parsed0040 = nullptr,
        const juce::String& voiceName = {});

    juce::var libraryVoiceParamsToJsonVar (
        const LiveSynthState& s,
        bool inDumpOnly = false,
        const YamahaLmVoiceDump::Lm8101VcMinimal* parsed8101 = nullptr,
        const YamahaLmVoiceDump::Lm0040VcMinimal* parsed0040 = nullptr,
        const juce::String& voiceName = {});

    juce::var libraryVoiceParamRowsToJsonVar (const std::vector<LibraryVoiceParamRow>& rows);

} // namespace Sy99ParamRegistry

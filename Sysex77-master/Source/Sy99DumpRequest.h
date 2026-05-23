/*
  ==============================================================================

    Sy99DumpRequest.h
    Build Yamaha SY99 bulk dump REQUEST frames (F0 43 2n 7A … LM 0040xx).

    Spec: _agent_context/sy99_bulk_dump_request.md
    Tail variant A (mt/mm/checksum) — SY77 extrapolation, needs hardware verify.
    Tail variant B (header-only) — official minimal request from manual.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

/** 0 = header-only (variant B), 1 = mt/mm + checksum (variant A). */
enum Sy99DumpRequestTailVariant
{
    sy99DumpRequestTailHeaderOnly = 0,
    sy99DumpRequestTailMtMmChecksum = 1
};

inline std::function<void (const juce::MidiMessage&)>& sy99DumpRequestSendCallback() noexcept
{
    static std::function<void (const juce::MidiMessage&)> cb;
    return cb;
}

/** Non-empty string = MIDI ports not ready for dump-request test. */
inline std::function<juce::String()>& sy99MidiPortStatusCallback() noexcept
{
    static std::function<juce::String()> cb;
    return cb;
}

inline uint8 sy99MakeBulkRequestDeviceByte (uint8 sysexDeviceByte) noexcept
{
    return uint8 (0x20 | (sysexDeviceByte & 0x0F));
}

inline uint8 sy99MakeBulkDataDeviceByte (uint8 sysexDeviceByte) noexcept
{
    return uint8 (0x00 | (sysexDeviceByte & 0x0F));
}

inline uint8 sy99Yamaha7BitMaskedChecksum (const uint8* data,
                                           int fromIndex,
                                           int toIndexInclusive) noexcept
{
    int sum = 0;

    for (int i = fromIndex; i <= toIndexInclusive; ++i)
        sum += data[i];

    return uint8 (((-sum) + 1) & 0x7F);
}

/** Six-char LM payload type after "LM  ", e.g. "0040VC", "0040MU". */
inline juce::MidiMessage buildSy99Lm0040DumpRequest (uint8 sysexDeviceByte,
                                                     const char* lmType6,
                                                     uint8 memoryType,
                                                     uint8 memoryNumber,
                                                     int tailVariant)
{
    juce::Array<uint8> bytes;
    bytes.add (0xF0);
    bytes.add (0x43);
    bytes.add (sy99MakeBulkRequestDeviceByte (sysexDeviceByte));
    bytes.add (0x7A);
    bytes.add ('L');
    bytes.add ('M');
    bytes.add (' ');
    bytes.add (' ');

    for (int i = 0; i < 6; ++i)
        bytes.add (uint8 (lmType6 != nullptr ? lmType6[i] : 0));

    bytes.add (0x00);
    bytes.add (0x00);

    if (tailVariant == sy99DumpRequestTailHeaderOnly)
    {
        bytes.add (0xF7);
    }
    else
    {
        bytes.add (memoryType);
        bytes.add (memoryNumber);
        const uint8 cs = sy99Yamaha7BitMaskedChecksum (bytes.getRawDataPointer(),
                                                       6,
                                                       bytes.size() - 1);
        bytes.add (cs);
        bytes.add (0xF7);
    }

    return juce::MidiMessage (bytes.getRawDataPointer(), bytes.size());
}

inline juce::MidiMessage buildSy99VoiceDumpRequest (uint8 sysexDeviceByte,
                                                    uint8 memoryType,
                                                    uint8 memoryNumber,
                                                    int tailVariant)
{
    return buildSy99Lm0040DumpRequest (sysexDeviceByte, "0040VC", memoryType, memoryNumber, tailVariant);
}

inline juce::MidiMessage buildSy99MultiDumpRequest (uint8 sysexDeviceByte,
                                                    uint8 memoryType,
                                                    uint8 memoryNumber,
                                                    int tailVariant)
{
    return buildSy99Lm0040DumpRequest (sysexDeviceByte, "0040MU", memoryType, memoryNumber, tailVariant);
}

inline juce::MidiMessage buildSy99SystemDumpRequest (uint8 sysexDeviceByte,
                                                     uint8 memoryType,
                                                     uint8 memoryNumber,
                                                     int tailVariant)
{
    return buildSy99Lm0040DumpRequest (sysexDeviceByte, "0040SY", memoryType, memoryNumber, tailVariant);
}

inline juce::MidiMessage buildSy99EffectDumpRequest (uint8 sysexDeviceByte,
                                                     uint8 memoryType,
                                                     uint8 memoryNumber,
                                                     int tailVariant)
{
    return buildSy99Lm0040DumpRequest (sysexDeviceByte, "0040MS", memoryType, memoryNumber, tailVariant);
}

inline juce::String formatSy99DumpRequestHex (const juce::MidiMessage& msg)
{
    if (! msg.isSysEx())
        return {};

    juce::String hex;
    const int n = msg.getRawDataSize();

    for (int i = 0; i < n; ++i)
    {
        if (i > 0)
            hex << ' ';

        hex << juce::String::toHexString (msg.getRawData()[i]).paddedLeft ('0', 2).toUpperCase();
    }

    return hex;
}

inline void sendSy99Lm0040DumpRequest (uint8 sysexDeviceByte,
                                       const char* lmType6,
                                       uint8 memoryType,
                                       uint8 memoryNumber,
                                       int tailVariant)
{
    const juce::MidiMessage msg = buildSy99Lm0040DumpRequest (sysexDeviceByte,
                                                                lmType6,
                                                                memoryType,
                                                                memoryNumber,
                                                                tailVariant);
    const juce::String variantLabel = (tailVariant == sy99DumpRequestTailHeaderOnly)
                                          ? juce::String ("B/header")
                                          : juce::String ("A/mtmm");

    juce::Logger::writeToLog ("[DumpReq] TX " + variantLabel
                              + " type=" + juce::String (lmType6 != nullptr ? lmType6 : "?")
                              + " mt=" + juce::String::toHexString ((int) memoryType).paddedLeft ('0', 2)
                              + " mm=" + juce::String::toHexString ((int) memoryNumber).paddedLeft ('0', 2)
                              + " " + formatSy99DumpRequestHex (msg));

    if (auto& send = sy99DumpRequestSendCallback(); send != nullptr)
        send (msg);
    else
        juce::Logger::writeToLog ("[DumpReq] sy99DumpRequestSendCallback not wired — request not sent");
}

inline void sendSy99VoiceDumpRequest (uint8 sysexDeviceByte,
                                      uint8 memoryType,
                                      uint8 memoryNumber,
                                      int tailVariant)
{
    sendSy99Lm0040DumpRequest (sysexDeviceByte, "0040VC", memoryType, memoryNumber, tailVariant);
}

inline void sendSy99MultiDumpRequest (uint8 sysexDeviceByte,
                                      uint8 memoryType,
                                      uint8 memoryNumber,
                                      int tailVariant)
{
    sendSy99Lm0040DumpRequest (sysexDeviceByte, "0040MU", memoryType, memoryNumber, tailVariant);
}

inline void sendSy99SystemDumpRequest (uint8 sysexDeviceByte,
                                       uint8 memoryType,
                                       uint8 memoryNumber,
                                       int tailVariant)
{
    sendSy99Lm0040DumpRequest (sysexDeviceByte, "0040SY", memoryType, memoryNumber, tailVariant);
}

inline void sendSy99EffectDumpRequest (uint8 sysexDeviceByte,
                                       uint8 memoryType,
                                       uint8 memoryNumber,
                                       int tailVariant)
{
    sendSy99Lm0040DumpRequest (sysexDeviceByte, "0040MS", memoryType, memoryNumber, tailVariant);
}

/** Internal voice count for banks A–D (SY99). */
static constexpr int kSy99InternalVoiceCount = 64;

/** Send one internal voice dump request (mt=00, mm=0..63). */
inline void sendSy99InternalVoiceDumpRequest (uint8 sysexDeviceByte,
                                              int memoryNumber,
                                              int tailVariant)
{
    sendSy99VoiceDumpRequest (sysexDeviceByte,
                              0x00,
                              uint8 (juce::jlimit (0, 63, memoryNumber)),
                              tailVariant);
}

/** SYM7-style bulk request: F0 43 2n 7A LM␠␠ [6-char type] 00 00 00×12 [byte28] [mm] F7 (31 B). */
inline juce::MidiMessage buildSym78101BulkRequest (uint8 sysexDeviceByte,
                                                   const char* lmType6,
                                                   uint8 byte28,
                                                   uint8 memoryNumber)
{
    juce::Array<uint8> bytes;
    bytes.add (0xF0);
    bytes.add (0x43);
    bytes.add (sy99MakeBulkRequestDeviceByte (sysexDeviceByte));
    bytes.add (0x7A);
    bytes.add ('L');
    bytes.add ('M');
    bytes.add (' ');
    bytes.add (' ');

    for (int i = 0; i < 6; ++i)
        bytes.add (uint8 (lmType6 != nullptr ? lmType6[i] : 0));

    bytes.add (0x00);
    bytes.add (0x00);

    for (int i = 0; i < 12; ++i)
        bytes.add (0x00);

    bytes.add (byte28);
    bytes.add (memoryNumber);
    bytes.add (0xF7);

    return juce::MidiMessage (bytes.getRawDataPointer(), bytes.size());
}

inline void sendSym78101BulkRequest (uint8 sysexDeviceByte,
                                     const char* lmType6,
                                     uint8 byte28,
                                     uint8 memoryNumber)
{
    const juce::MidiMessage msg = buildSym78101BulkRequest (sysexDeviceByte,
                                                              lmType6,
                                                              byte28,
                                                              memoryNumber);

    juce::Logger::writeToLog ("[DumpReq] TX 8101 type=" + juce::String (lmType6 != nullptr ? lmType6 : "?")
                              + " b28=" + juce::String::toHexString ((int) byte28).paddedLeft ('0', 2)
                              + " mm=" + juce::String::toHexString ((int) memoryNumber).paddedLeft ('0', 2)
                              + " " + formatSy99DumpRequestHex (msg));

    if (auto& send = sy99DumpRequestSendCallback(); send != nullptr)
        send (msg);
    else
        juce::Logger::writeToLog ("[DumpReq] sy99DumpRequestSendCallback not wired — request not sent");
}

inline void sendSym78101VcRequest (uint8 sysexDeviceByte,
                                   uint8 memoryNumber,
                                   uint8 byte28 = 0) noexcept
{
    sendSym78101BulkRequest (sysexDeviceByte,
                             "8101VC",
                             byte28,
                             uint8 (juce::jlimit (0, 63, (int) memoryNumber)));
}

inline void sendSym78101MuRequest (uint8 sysexDeviceByte,
                                   uint8 memoryNumber,
                                   uint8 byte28 = 0) noexcept
{
    sendSym78101BulkRequest (sysexDeviceByte, "8101MU", byte28, memoryNumber);
}

inline void sendSym78101PnRequest (uint8 sysexDeviceByte,
                                   uint8 memoryNumber,
                                   uint8 byte28 = 0) noexcept
{
    sendSym78101BulkRequest (sysexDeviceByte, "8101PN", byte28, memoryNumber);
}

inline void sendSym78101MtRequest (uint8 sysexDeviceByte,
                                   uint8 memoryNumber,
                                   uint8 byte28 = 0) noexcept
{
    sendSym78101BulkRequest (sysexDeviceByte, "8101MT", byte28, memoryNumber);
}

inline void sendSym78101SyRequest (uint8 sysexDeviceByte) noexcept
{
    sendSym78101BulkRequest (sysexDeviceByte, "8101SY", 0x00, 0x00);
}

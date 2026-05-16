#pragma once

enum class VoiceMode {
    Mode1_1AFM_Poly = 1,
    Mode2_2AFM_Poly = 2,
    Mode3_4AFM_Mono = 3,
    Mode4_1AWM_Poly = 4,
    Mode5_2AWM_Poly = 5,
    Mode6_1AFM_1AWM = 6,
    Mode7_2AFM_1AWM = 7,
    Mode8_4AWM_Poly = 8,
    Mode9_1AFM_2AWM = 9,
    Mode10_2AFM_2AWM = 10
};

class VoiceModeUtils {
public:
    static int getAfmElementCount(VoiceMode mode) {
        switch (mode) {
            case VoiceMode::Mode4_1AWM_Poly:
            case VoiceMode::Mode5_2AWM_Poly:
            case VoiceMode::Mode8_4AWM_Poly: return 0;
            case VoiceMode::Mode1_1AFM_Poly:
            case VoiceMode::Mode6_1AFM_1AWM:
            case VoiceMode::Mode9_1AFM_2AWM: return 1;
            case VoiceMode::Mode2_2AFM_Poly:
            case VoiceMode::Mode7_2AFM_1AWM:
            case VoiceMode::Mode10_2AFM_2AWM: return 2;
            case VoiceMode::Mode3_4AFM_Mono: return 4;
            default: return 1;
        }
    }

    static int getAwmElementCount(VoiceMode mode) {
        switch (mode) {
            case VoiceMode::Mode1_1AFM_Poly:
            case VoiceMode::Mode2_2AFM_Poly:
            case VoiceMode::Mode3_4AFM_Mono: return 0;
            case VoiceMode::Mode4_1AWM_Poly:
            case VoiceMode::Mode6_1AFM_1AWM:
            case VoiceMode::Mode7_2AFM_1AWM: return 1;
            case VoiceMode::Mode5_2AWM_Poly:
            case VoiceMode::Mode9_1AFM_2AWM:
            case VoiceMode::Mode10_2AFM_2AWM: return 2;
            case VoiceMode::Mode8_4AWM_Poly: return 4;
            default: return 0;
        }
    }
};

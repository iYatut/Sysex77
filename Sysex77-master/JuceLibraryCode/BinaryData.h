/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   Sysex77_png;
    const int            Sysex77_pngSize = 9810;

    extern const char*   Sysex99_png;
    const int            Sysex99_pngSize = 10123;

    extern const char*   TableData_xml;
    const int            TableData_xmlSize = 7258;

    extern const char*   Audio_png;
    const int            Audio_pngSize = 13001;

    extern const char*   Librairie_xml;
    const int            Librairie_xmlSize = 669;

    extern const char*   Filter_png;
    const int            Filter_pngSize = 5252;

    extern const char*   SY77_YAMAHA_png;
    const int            SY77_YAMAHA_pngSize = 467577;

    extern const char*   AFM_png;
    const int            AFM_pngSize = 2757;

    extern const char*   French_txt;
    const int            French_txtSize = 693;

    extern const char*   VCA_png;
    const int            VCA_pngSize = 5982;

    extern const char*   SY77Waves_xml;
    const int            SY77Waves_xmlSize = 6738;

    extern const char*   SY99Waves_xml;
    const int            SY99Waves_xmlSize = 9043;

    extern const char*   SwitchFilter_png;
    const int            SwitchFilter_pngSize = 5252;

    extern const char*   FiltreControl_png;
    const int            FiltreControl_pngSize = 5252;

    extern const char*   ButtonFilter_png;
    const int            ButtonFilter_pngSize = 5252;

    extern const char*   OscAfm_png;
    const int            OscAfm_pngSize = 45645;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 16;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}

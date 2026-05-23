/*
  ==============================================================================

    Sy99LibraryApi.cpp

  ==============================================================================
*/

#include "Sy99LibraryApi.h"

#include "LiveSynthState.h"
#include "Sy99ParamRegistry.h"
#include "YamahaLmVoiceDump.h"

namespace Sy99LibraryApi
{
    namespace
    {
        enum class Page : std::uint8_t
        {
            internalVoices = 0,
            preset1Voices,
            preset2Voices,
            multiInternal
        };

        struct VoiceCaptureIndex
        {
            juce::Array<int> offsets;
            juce::Array<int> lengths;
            juce::StringArray names;
            juce::Array<int> elmodeRaws;
        };

        struct CaptureFileIndexCacheEntry
        {
            juce::int64 modTimeMs = 0;
            VoiceCaptureIndex index;
        };

        struct VoiceDetailCacheEntry
        {
            juce::int64 captureModTimeMs = 0;
            juce::var payload;
        };

        juce::CriticalSection gLibraryApiCacheLock;
        juce::HashMap<juce::String, CaptureFileIndexCacheEntry> gCaptureIndexCache;
        juce::HashMap<juce::String, VoiceDetailCacheEntry> gVoiceDetailCache;

        int pageSlotCount (Page page) noexcept
        {
            return page == Page::multiInternal ? 16 : 64;
        }

        juce::String pageSlug (Page page) noexcept
        {
            switch (page)
            {
                case Page::preset1Voices: return "preset1";
                case Page::preset2Voices: return "preset2";
                case Page::multiInternal: return "multi";
                case Page::internalVoices:
                default:                  return "internal";
            }
        }

        juce::String pageLabel (Page page) noexcept
        {
            switch (page)
            {
                case Page::preset1Voices: return "PRE1";
                case Page::preset2Voices: return "PRE2";
                case Page::multiInternal: return "Multi";
                case Page::internalVoices:
                default:                  return "Internal";
            }
        }

        bool pageFromSlug (const juce::String& slug, Page& pageOut) noexcept
        {
            if (slug.equalsIgnoreCase ("internal"))
            {
                pageOut = Page::internalVoices;
                return true;
            }

            if (slug.equalsIgnoreCase ("preset1"))
            {
                pageOut = Page::preset1Voices;
                return true;
            }

            if (slug.equalsIgnoreCase ("preset2"))
            {
                pageOut = Page::preset2Voices;
                return true;
            }

            if (slug.equalsIgnoreCase ("multi"))
            {
                pageOut = Page::multiInternal;
                return true;
            }

            return false;
        }

        juce::File stableCapturePath (const juce::String& namePrefix) noexcept
        {
            return libraryCapturesDirPath().getChildFile (namePrefix + ".syx");
        }

        juce::String previewNameFromSysexFrame (const juce::uint8* data, int size) noexcept
        {
            static const char* kPreviewLmTags[] = { "8101VC", "8101MU" };

            for (const char* tag : kPreviewLmTags)
            {
                if (! YamahaLmVoiceDump::frameContainsLmTag (data, size, tag))
                    continue;

                char name[11] {};
                YamahaLmVoiceDump::copyLm8101BlockName (data, size, tag, name, (int) sizeof (name));

                if (name[0] != '\0')
                    return juce::String (name).trimEnd();
            }

            return {};
        }

        juce::File captureFileForPage (Page page) noexcept
        {
            switch (page)
            {
                case Page::preset1Voices:
                    return stableCapturePath ("AUTOSYNC-VC-P2");
                case Page::preset2Voices:
                    return stableCapturePath ("AUTOSYNC-VC-P3");
                case Page::multiInternal:
                    return stableCapturePath ("AUTOSYNC-MU-INT");
                case Page::internalVoices:
                default:
                {
                    juce::File internal = stableCapturePath ("AUTOSYNC-VC-INT");

                    if (internal.existsAsFile())
                        return internal;

                    return stableCapturePath ("AUTOSYNC-VOICE64");
                }
            }
        }

        juce::String slotCodeForMm (Page page, int mm) noexcept
        {
            if (page == Page::multiInternal)
                return "M" + juce::String (mm + 1);

            if (mm < 0 || mm >= 64)
                return {};

            static const char banks[] = "ABCD";
            return juce::String::charToString (banks[mm / 16]) + juce::String ((mm % 16) + 1);
        }

        bool buildVoiceCaptureIndex (const juce::File& bankFile, VoiceCaptureIndex& index) noexcept
        {
            index.offsets.clear();
            index.lengths.clear();
            index.names.clear();
            index.elmodeRaws.clear();

            if (! bankFile.existsAsFile())
                return false;

            juce::MemoryBlock mb;

            if (! bankFile.loadFileAsData (mb))
                return false;

            const auto* const data = static_cast<const juce::uint8*> (mb.getData());
            const size_t total = mb.getSize();

            for (size_t i = 0; i < total;)
            {
                if (data[i] != 0xF0)
                {
                    ++i;
                    continue;
                }

                const size_t frameStart = i;
                size_t j = i + 1;

                while (j < total && data[j] != 0xF7)
                    ++j;

                if (j >= total)
                    break;

                const int frameLen = (int) (j - frameStart + 1);

                if (! YamahaLmVoiceDump::frameContainsLmTag (data + frameStart, frameLen, "8101VC"))
                {
                    i = j + 1;
                    continue;
                }

                juce::String name;
                YamahaLmVoiceDump::Lm8101VcMinimal parsed;
                int elmodeRaw = -1;

                if (YamahaLmVoiceDump::parseLm8101VcMinimal (data + frameStart, frameLen, parsed)
                    && parsed.name[0] != '\0')
                {
                    name = juce::String (parsed.name).trimEnd();
                    elmodeRaw = parsed.elmodeRaw;
                }
                else if (frameStart + 43 <= total)
                {
                    for (int a = 0; a < 10; ++a)
                    {
                        const juce::uint8 c = data[frameStart + 33 + a];
                        const char ch = (c >= 0x20 && c < 0x7F) ? (char) c : ' ';
                        name += juce::String::charToString ((juce_wchar) ch);
                    }

                    name = name.trimEnd();

                    if (frameStart + 32 < total)
                        elmodeRaw = (int) data[frameStart + 32];
                }

                index.names.add (name);
                index.elmodeRaws.add (elmodeRaw);
                index.offsets.add ((int) frameStart);
                index.lengths.add (frameLen);
                i = j + 1;
            }

            return index.offsets.size() > 0;
        }

        const VoiceCaptureIndex* findCachedVoiceCaptureIndex (const juce::File& capture) noexcept
        {
            if (! capture.existsAsFile())
                return nullptr;

            const juce::String key = capture.getFullPathName();
            const juce::int64 modTimeMs = capture.getLastModificationTime().toMilliseconds();

            const juce::ScopedLock lock (gLibraryApiCacheLock);

            if (gCaptureIndexCache.contains (key)
                && gCaptureIndexCache[key].modTimeMs == modTimeMs)
                return &gCaptureIndexCache[key].index;

            auto& entry = gCaptureIndexCache.getReference (key);
            entry.modTimeMs = modTimeMs;
            entry.index = {};
            buildVoiceCaptureIndex (capture, entry.index);

            if (entry.index.offsets.isEmpty())
                return nullptr;

            return &entry.index;
        }

        juce::String voiceDetailCacheKey (Page page, int mm, juce::int64 captureModTimeMs) noexcept
        {
            return pageSlug (page) + ":" + juce::String (mm) + "@" + juce::String (captureModTimeMs);
        }

        juce::StringArray loadMultiNamesFromCapture (const juce::File& file, int slotCount) noexcept
        {
            juce::StringArray names;

            for (int i = 0; i < slotCount; ++i)
                names.add ({});

            if (! file.existsAsFile())
                return names;

            juce::MemoryBlock data;

            if (! file.loadFileAsData (data))
                return names;

            const auto* bytes = static_cast<const juce::uint8*> (data.getData());
            const size_t size = data.getSize();
            int slot = 0;

            for (size_t i = 0; i < size && slot < slotCount; ++i)
            {
                if (bytes[i] != 0xF0)
                    continue;

                size_t end = i + 1;

                while (end < size && bytes[end] != 0xF7)
                    ++end;

                if (end >= size || bytes[end] != 0xF7)
                    break;

                const int frameLen = (int) (end - i + 1);
                names.set (slot, previewNameFromSysexFrame (bytes + i, frameLen));
                ++slot;
                i = end;
            }

            return names;
        }

        juce::String voiceNameForMm (Page page, int mm, const VoiceCaptureIndex& index) noexcept
        {
            if (page == Page::multiInternal)
            {
                const auto names = loadMultiNamesFromCapture (captureFileForPage (page), pageSlotCount (page));

                if (mm >= 0 && mm < names.size())
                    return names[mm].trimEnd();

                return {};
            }

            if (mm >= 0 && mm < index.names.size())
                return index.names[mm].trimEnd();

            return {};
        }

        bool libraryHasCaptures() noexcept
        {
            const Page pages[] = {
                Page::internalVoices,
                Page::preset1Voices,
                Page::preset2Voices,
                Page::multiInternal,
            };

            for (const auto page : pages)
            {
                if (captureFileForPage (page).existsAsFile())
                    return true;
            }

            return false;
        }
    }

    juce::var libraryTreeToJsonVar()
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("id", "sy99");
        root->setProperty ("label", "SY-99");
        root->setProperty ("active", libraryHasCaptures());
        root->setProperty ("capturesDir", libraryCapturesDirPath().getFullPathName());

        juce::Array<juce::var> pages;

        const Page pageOrder[] = {
            Page::internalVoices,
            Page::preset1Voices,
            Page::preset2Voices,
            Page::multiInternal,
        };

        for (const auto page : pageOrder)
        {
            const juce::File capture = captureFileForPage (page);

            auto* pageObj = new juce::DynamicObject();
            pageObj->setProperty ("id", pageSlug (page));
            pageObj->setProperty ("label", pageLabel (page));
            pageObj->setProperty ("slotCount", pageSlotCount (page));
            pageObj->setProperty ("isMulti", page == Page::multiInternal);
            pageObj->setProperty ("available", capture.existsAsFile());
            pageObj->setProperty ("captureFile", capture.existsAsFile()
                                                  ? capture.getFileName()
                                                  : juce::var());
            pages.add (pageObj);
        }

        root->setProperty ("pages", pages);
        return root;
    }

    juce::var libraryPageVoicesFromSlug (const juce::String& pageSlugText)
    {
        Page page;

        if (! pageFromSlug (pageSlugText, page))
            return {};

        const juce::File capture = captureFileForPage (page);
        const int slotCount = pageSlotCount (page);
        const VoiceCaptureIndex* indexPtr = findCachedVoiceCaptureIndex (capture);
        VoiceCaptureIndex emptyIndex;

        if (indexPtr == nullptr && page != Page::multiInternal)
        {
            buildVoiceCaptureIndex (capture, emptyIndex);
            indexPtr = emptyIndex.offsets.isEmpty() ? nullptr : &emptyIndex;
        }

        auto* root = new juce::DynamicObject();
        root->setProperty ("pageId", pageSlug (page));
        root->setProperty ("pageLabel", pageLabel (page));
        root->setProperty ("slotCount", slotCount);
        root->setProperty ("captureFile", capture.existsAsFile() ? capture.getFileName() : juce::var());
        root->setProperty ("available", capture.existsAsFile());

        juce::Array<juce::var> voices;

        for (int mm = 0; mm < slotCount; ++mm)
        {
            juce::String name;

            if (indexPtr != nullptr)
                name = voiceNameForMm (page, mm, *indexPtr);

            if (name.isEmpty())
                name = slotCodeForMm (page, mm);

            auto* voice = new juce::DynamicObject();
            voice->setProperty ("mm", mm);
            voice->setProperty ("slotCode", slotCodeForMm (page, mm));
            voice->setProperty ("name", name);

            if (indexPtr != nullptr && mm >= 0 && mm < indexPtr->elmodeRaws.size())
                voice->setProperty ("elmodeRaw", indexPtr->elmodeRaws[mm]);

            voices.add (voice);
        }

        root->setProperty ("voices", voices);
        return root;
    }

    juce::var libraryVoiceDetailFromSlug (const juce::String& pageSlugText, int mm, juce::String& errorOut)
    {
        errorOut.clear();

        Page page;

        if (! pageFromSlug (pageSlugText, page))
        {
            errorOut = "unknown library page";
            return {};
        }

        const int slotCount = pageSlotCount (page);

        if (mm < 0 || mm >= slotCount)
        {
            errorOut = "slot index out of range";
            return {};
        }

        if (page == Page::multiInternal)
        {
            errorOut = "Multi dump parameter table is not available yet";
            return {};
        }

        const juce::File capture = captureFileForPage (page);

        if (! capture.existsAsFile())
        {
            errorOut = "capture file missing: " + capture.getFileName();
            return {};
        }

        const juce::int64 captureModTimeMs = capture.getLastModificationTime().toMilliseconds();
        const juce::String cacheKey = voiceDetailCacheKey (page, mm, captureModTimeMs);

        {
            const juce::ScopedLock lock (gLibraryApiCacheLock);

            if (gVoiceDetailCache.contains (cacheKey))
                return gVoiceDetailCache[cacheKey].payload;
        }

        const VoiceCaptureIndex* indexPtr = findCachedVoiceCaptureIndex (capture);

        if (indexPtr == nullptr || mm >= indexPtr->offsets.size())
        {
            errorOut = "voice frame not found in capture";
            return {};
        }

        const VoiceCaptureIndex& index = *indexPtr;

        LiveSynthState state;
        state.reset();

        juce::MemoryBlock frame;
        juce::String sliceError;

        if (! extractBankVoiceFrameSlice (capture, mm, index.offsets, index.lengths, frame, sliceError))
        {
            errorOut = sliceError.isNotEmpty() ? sliceError : juce::String ("failed to read voice frame");
            return {};
        }

        YamahaLmVoiceDump::Lm8101VcMinimal parsed8101;
        YamahaLmVoiceDump::parseLm8101VcMinimal (frame.getData(), frame.getSize(), parsed8101);

        YamahaLmVoiceDump::Lm0040VcMinimal parsed0040;
        juce::MemoryBlock bankData;

        if (capture.loadFileAsData (bankData))
        {
            const auto* bytes = static_cast<const juce::uint8*> (bankData.getData());
            const int slotOffset = index.offsets[mm];
            const int slotLength = index.lengths[mm];
            int pairedOff = -1;
            int pairedLen = 0;

            if (YamahaLmVoiceDump::findPaired0040FrameAfter (bytes, bankData.getSize(),
                                                              slotOffset + slotLength,
                                                              pairedOff, pairedLen))
            {
                YamahaLmVoiceDump::parseLm0040VcMinimal (bytes + (size_t) pairedOff,
                                                         pairedLen, parsed0040);
            }
        }

        if (! ingestVoiceSlotIntoLiveSynthState (state, mm, capture, index.offsets, index.lengths))
        {
            errorOut = "failed to parse voice dump";
            return {};
        }

        juce::String voiceName = voiceNameForMm (page, mm, index);

        if (voiceName.isEmpty() && state.lmVoiceName[0] != '\0')
            voiceName = juce::String (state.lmVoiceName).trimEnd();

        if (voiceName.isEmpty())
            voiceName = slotCodeForMm (page, mm);

        auto* root = new juce::DynamicObject();
        root->setProperty ("pageId", pageSlug (page));
        root->setProperty ("pageLabel", pageLabel (page));
        root->setProperty ("mm", mm);
        root->setProperty ("slotCode", slotCodeForMm (page, mm));
        root->setProperty ("voiceName", voiceName);
        root->setProperty ("captureFile", capture.getFileName());

        const auto paramRows = Sy99ParamRegistry::buildLibraryVoiceParamRows (
            state, false, &parsed8101, parsed0040.found0040 ? &parsed0040 : nullptr, voiceName);
        root->setProperty ("params", Sy99ParamRegistry::libraryVoiceParamRowsToJsonVar (paramRows));
        root->setProperty ("catalogStats",
                           Sy99ParamRegistry::libraryCatalogStatsToJsonVar (
                               Sy99ParamRegistry::computeLibraryCatalogStats (paramRows)));

        juce::var result (root);

        {
            const juce::ScopedLock lock (gLibraryApiCacheLock);

            if (gVoiceDetailCache.size() > 32)
                gVoiceDetailCache.clear();

            auto& entry = gVoiceDetailCache.getReference (cacheKey);
            entry.captureModTimeMs = captureModTimeMs;
            entry.payload = result;
        }

        return result;
    }

    LibraryRouteParts parseLibraryPagesPath (const juce::String& path) noexcept
    {
        LibraryRouteParts parts;
        const juce::String prefix = "/api/library/pages/";

        if (! path.startsWithIgnoreCase (prefix))
            return parts;

        juce::String rest = path.substring (prefix.length());

        if (rest.endsWithIgnoreCase ("/voices"))
        {
            parts.slug = rest.upToLastOccurrenceOf ("/", false, false);

            if (parts.slug.isEmpty())
                return parts;

            Page page;
            parts.valid = pageFromSlug (parts.slug, page);
            return parts;
        }

        const juce::String voicesMarker = "/voices/";

        if (rest.containsIgnoreCase (voicesMarker))
        {
            parts.slug = rest.upToFirstOccurrenceOf ("/", false, false);
            juce::String tail = rest.fromFirstOccurrenceOf (voicesMarker, false, false);

            parts.mm = tail.upToFirstOccurrenceOf ("/", false, false).getIntValue();
            parts.isVoiceDetail = true;

            Page page;
            parts.valid = pageFromSlug (parts.slug, page) && parts.slug.isNotEmpty();
            return parts;
        }

        return parts;
    }
}

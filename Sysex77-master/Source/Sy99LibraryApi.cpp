/*
  ==============================================================================

    Sy99LibraryApi.cpp

  ==============================================================================
*/

#include "Sy99LibraryApi.h"

#include "LiveSynthState.h"
#include "Sy99ParamRegistry.h"
#include "Sy99VoiceCaptureIndexCache.h"
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

        struct VoiceDetailCacheEntry
        {
            juce::int64 captureModTimeMs = 0;
            juce::var payload;
        };

        juce::CriticalSection gLibraryApiCacheLock;
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

        const Sy99CaptureManifestSlot* manifestSlotForMm (const Sy99CaptureManifest& manifest,
                                                          Page page,
                                                          int mm) noexcept
        {
            if (const Sy99CaptureManifestSlot* direct = manifest.slotAt (mm))
                return direct;

            if (page == Page::multiInternal)
            {
                for (const auto& slot : manifest.slots)
                    if (slot.mm == mm)
                        return &slot;
            }

            if (mm >= 0 && mm < manifest.slots.size())
                return &manifest.slots.getReference (mm);

            return nullptr;
        }

        juce::String voiceNameFromManifest (Page page,
                                            int mm,
                                            const Sy99CaptureManifest& manifest) noexcept
        {
            if (const auto* slot = manifestSlotForMm (manifest, page, mm))
                return slot->name.trimEnd();

            return {};
        }

        bool manifestVoiceIndexArrays (const Sy99CaptureManifest& manifest,
                                       juce::Array<int>& offsets,
                                       juce::Array<int>& lengths) noexcept
        {
            offsets.clear();
            lengths.clear();

            for (const auto& slot : manifest.slots)
            {
                if (! slot.tag.equalsIgnoreCase ("8101VC"))
                    continue;

                offsets.add (slot.offset);
                lengths.add (slot.length);
            }

            return offsets.size() > 0;
        }

        juce::String voiceDetailCacheKey (Page page, int mm, juce::int64 captureModTimeMs) noexcept
        {
            return pageSlug (page) + ":" + juce::String (mm) + "@" + juce::String (captureModTimeMs);
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
        const Sy99CaptureManifest* manifestPtr = sy99GetCaptureManifest (capture, true);

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

            if (manifestPtr != nullptr)
                name = voiceNameFromManifest (page, mm, *manifestPtr);

            if (name.isEmpty())
                name = slotCodeForMm (page, mm);

            auto* voice = new juce::DynamicObject();
            voice->setProperty ("mm", mm);
            voice->setProperty ("slotCode", slotCodeForMm (page, mm));
            voice->setProperty ("name", name);

            if (manifestPtr != nullptr)
            {
                if (const auto* slot = manifestSlotForMm (*manifestPtr, page, mm))
                {
                    if (slot->elmodeRaw >= 0)
                        voice->setProperty ("elmodeRaw", slot->elmodeRaw);
                }
            }

            voice->setProperty ("hasDetail", capture.existsAsFile());
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

        const Sy99CaptureManifest* manifestPtr = sy99GetCaptureManifest (capture, true);

        if (manifestPtr == nullptr)
        {
            errorOut = "capture manifest unavailable";
            return {};
        }

        const Sy99CaptureManifest& manifest = *manifestPtr;
        const Sy99CaptureManifestSlot* slot = manifestSlotForMm (manifest, page, mm);

        if (slot == nullptr || slot->length <= 0)
        {
            errorOut = "voice frame not found in capture manifest";
            return {};
        }

        juce::Array<int> offsets;
        juce::Array<int> lengths;
        manifestVoiceIndexArrays (manifest, offsets, lengths);

        const int voiceIndex = offsets.indexOf (slot->offset);

        if (voiceIndex < 0)
        {
            errorOut = "voice frame offset missing in manifest";
            return {};
        }

        LiveSynthState state;
        state.reset();

        juce::MemoryBlock frame;
        juce::String sliceError;

        if (! extractBankVoiceFrameSlice (capture, voiceIndex, offsets, lengths, frame, sliceError))
        {
            errorOut = sliceError.isNotEmpty() ? sliceError : juce::String ("failed to read voice frame");
            return {};
        }

        YamahaLmVoiceDump::Lm8101VcMinimal parsed8101;
        YamahaLmVoiceDump::parseLm8101VcMinimal (frame.getData(), frame.getSize(), parsed8101,
                                                 YamahaLmVoiceDump::Lm8101ParseMode::ingest);

        YamahaLmVoiceDump::Lm0040VcMinimal parsed0040;
        juce::MemoryBlock bankData;

        if (capture.loadFileAsData (bankData))
        {
            const auto* bytes = static_cast<const juce::uint8*> (bankData.getData());
            int pairedOff = -1;
            int pairedLen = 0;

            if (YamahaLmVoiceDump::findPaired0040FrameAfter (bytes, bankData.getSize(),
                                                              slot->offset + slot->length,
                                                              pairedOff, pairedLen))
            {
                YamahaLmVoiceDump::parseLm0040VcMinimal (bytes + (size_t) pairedOff,
                                                         pairedLen, parsed0040);
            }
        }

        if (! ingestVoiceSlotIntoLiveSynthState (state, voiceIndex, capture, offsets, lengths))
        {
            errorOut = "failed to parse voice dump";
            return {};
        }

        juce::String voiceName = slot->name.trimEnd();

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

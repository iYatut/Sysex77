/*
  ==============================================================================

    Sy99VoiceCaptureIndexCache.cpp

  ==============================================================================
*/

#include "Sy99VoiceCaptureIndexCache.h"

#include "LiveSynthState.h"

namespace
{
    constexpr int kManifestLruCap = 16;
    constexpr int kManifestJsonVersion = 1;

    juce::CriticalSection gManifestLock;
    juce::HashMap<juce::String, Sy99CaptureManifest> gManifestLru;
    juce::StringArray gManifestLruOrder;

    void touchManifestLruKey (const juce::String& key) noexcept
    {
        const int idx = gManifestLruOrder.indexOf (key);

        if (idx >= 0)
            gManifestLruOrder.remove (idx);

        gManifestLruOrder.add (key);

        while (gManifestLruOrder.size() > kManifestLruCap)
        {
            const juce::String evict = gManifestLruOrder[0];
            gManifestLruOrder.remove (0);
            gManifestLru.remove (evict);
        }
    }

    bool appendSlotFromFrame (const juce::uint8* data,
                              int frameLen,
                              int mm,
                              Sy99CaptureManifest& manifest) noexcept
    {
        static const char* kVoiceTags[] = { "8101VC", "8101MU" };
        const char* matchedTag = nullptr;

        for (const char* tag : kVoiceTags)
        {
            if (YamahaLmVoiceDump::frameContainsLmTag (data, frameLen, tag))
            {
                matchedTag = tag;
                break;
            }
        }

        if (matchedTag == nullptr)
            return false;

        Sy99CaptureManifestSlot slot;
        slot.mm = mm;
        slot.offset = 0;
        slot.length = frameLen;
        slot.tag = matchedTag;

        char nameBuf[11] {};
        int elmodeRaw = -1;

        if (juce::String (matchedTag).equalsIgnoreCase ("8101VC"))
        {
            if (YamahaLmVoiceDump::readLm8101VcIndexFields (data, frameLen,
                                                            nameBuf, (int) sizeof (nameBuf),
                                                            elmodeRaw))
            {
                slot.name = juce::String (nameBuf).trimEnd();
            }
            else if (frameLen > 43)
            {
                for (int a = 0; a < 10; ++a)
                {
                    const juce::uint8 c = data[33 + a];
                    const char ch = (c >= 0x20 && c < 0x7F) ? (char) c : ' ';
                    slot.name += juce::String::charToString ((juce_wchar) ch);
                }

                slot.name = slot.name.trimEnd();

                if (frameLen > 32)
                    elmodeRaw = (int) data[32];
            }
        }
        else
        {
            YamahaLmVoiceDump::copyLm8101BlockName (data, frameLen, matchedTag,
                                                    nameBuf, (int) sizeof (nameBuf));
            slot.name = juce::String (nameBuf).trimEnd();
        }

        slot.elmodeRaw = elmodeRaw;
        manifest.slots.add (slot);
        return true;
    }

    bool buildManifestFromFileBytes (const juce::File& captureFile,
                                     const juce::uint8* data,
                                     size_t total,
                                     Sy99CaptureManifest& manifest) noexcept
    {
        manifest = {};
        manifest.capturePath = captureFile.getFullPathName();
        manifest.sourceModTimeMs = captureFile.getLastModificationTime().toMilliseconds();

        int mm = 0;

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

            if (appendSlotFromFrame (data + frameStart, frameLen, mm, manifest))
            {
                manifest.slots.getReference (manifest.slots.size() - 1).offset = (int) frameStart;
                ++mm;
            }

            i = j + 1;
        }

        manifest.built = manifest.slots.size() > 0;
        return manifest.built;
    }

    bool read8101AddressFromFrame (const juce::uint8* data,
                                   int frameLen,
                                   const char* tag6,
                                   juce::uint8& byte28Out,
                                   juce::uint8& mmOut) noexcept
    {
        if (data == nullptr || tag6 == nullptr || frameLen < 26)
            return false;

        for (int i = 0; i <= frameLen - 26; ++i)
        {
            if (data[i] != 'L' || data[i + 1] != 'M')
                continue;

            if (! YamahaLmVoiceDump::tagsEqual6 (data + i + YamahaLmVoiceDump::kLmTagOffsetFromLm, tag6))
                continue;

            const int addr = i + 24;

            if (addr + 1 >= frameLen)
                return false;

            byte28Out = data[addr];
            mmOut = data[addr + 1];
            return true;
        }

        return false;
    }

    bool is0040AuditCaptureFileName (const juce::String& name) noexcept
    {
        return name.containsIgnoreCase ("0040VC")
            || name.containsIgnoreCase ("0040MU")
            || name.containsIgnoreCase ("0040SA");
    }

    int inferExpectedMmCountFromCaptureFile (const juce::File& captureFile) noexcept
    {
        const juce::String name = captureFile.getFileName();

        if (is0040AuditCaptureFileName (name))
            return 0;

        if (name.containsIgnoreCase ("8101MU") || name.contains ("MU-"))
            return 16;

        if (name.containsIgnoreCase ("8101VC") || name.containsIgnoreCase ("VOICE64"))
            return 64;

        if (name.contains ("VC-"))
            return 64;

        return 0;
    }

    bool isSlotSliceValid (const juce::uint8* fileData,
                           size_t fileSize,
                           const Sy99CaptureManifestSlot& slot) noexcept
    {
        if (fileData == nullptr || slot.offset < 0 || slot.length <= 0)
            return false;

        const size_t off = (size_t) slot.offset;
        const size_t len = (size_t) slot.length;

        if (off + len > fileSize)
            return false;

        return fileData[off] == 0xF0 && fileData[off + len - 1] == 0xF7;
    }

    int countInvalidManifestSlots (const juce::uint8* fileData,
                                   size_t fileSize,
                                   const Sy99CaptureManifest& manifest) noexcept
    {
        int invalid = 0;

        for (const auto& s : manifest.slots)
            if (! isSlotSliceValid (fileData, fileSize, s))
                ++invalid;

        return invalid;
    }

    bool buildManifestDedupedFromFileBytes (const juce::File& captureFile,
                                            const juce::uint8* data,
                                            size_t total,
                                            int expectedMmCount,
                                            Sy99CaptureManifest& manifest) noexcept
    {
        manifest = {};
        manifest.capturePath = captureFile.getFullPathName();
        manifest.sourceModTimeMs = captureFile.getLastModificationTime().toMilliseconds();

        juce::Array<Sy99CaptureManifestSlot> bestByMm;

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
            const juce::uint8* frame = data + frameStart;

            static const char* kVoiceTags[] = { "8101VC", "8101MU" };

            for (const char* tag : kVoiceTags)
            {
                if (! YamahaLmVoiceDump::frameContainsLmTag (frame, frameLen, tag))
                    continue;

                juce::uint8 byte28 = 0;
                juce::uint8 mmRaw = 0;

                if (! read8101AddressFromFrame (frame, frameLen, tag, byte28, mmRaw))
                    break;

                const int mm = (int) mmRaw;

                if (expectedMmCount > 0 && (mm < 0 || mm >= expectedMmCount))
                    break;

                Sy99CaptureManifestSlot candidate;
                candidate.mm = mm;
                candidate.offset = (int) frameStart;
                candidate.length = frameLen;
                candidate.tag = tag;

                char nameBuf[11] {};
                int elmodeRaw = -1;

                if (juce::String (tag).equalsIgnoreCase ("8101VC"))
                {
                    if (YamahaLmVoiceDump::readLm8101VcIndexFields (frame, frameLen,
                                                                    nameBuf, (int) sizeof (nameBuf),
                                                                    elmodeRaw))
                    {
                        candidate.name = juce::String (nameBuf).trimEnd();
                    }
                    else if (frameLen > 43)
                    {
                        for (int a = 0; a < 10; ++a)
                        {
                            const juce::uint8 c = frame[33 + a];
                            const char ch = (c >= 0x20 && c < 0x7F) ? (char) c : ' ';
                            candidate.name += juce::String::charToString ((juce_wchar) ch);
                        }

                        candidate.name = candidate.name.trimEnd();

                        if (frameLen > 32)
                            elmodeRaw = (int) frame[32];
                    }
                }
                else
                {
                    YamahaLmVoiceDump::copyLm8101BlockName (frame, frameLen, tag,
                                                            nameBuf, (int) sizeof (nameBuf));
                    candidate.name = juce::String (nameBuf).trimEnd();
                }

                candidate.elmodeRaw = elmodeRaw;

                int existingIdx = -1;

                for (int b = 0; b < bestByMm.size(); ++b)
                {
                    if (bestByMm.getReference (b).mm == mm)
                    {
                        existingIdx = b;
                        break;
                    }
                }

                if (existingIdx >= 0)
                {
                    if (candidate.offset > bestByMm.getReference (existingIdx).offset)
                        bestByMm.getReference (existingIdx) = candidate;
                }
                else
                {
                    bestByMm.add (candidate);
                }

                break;
            }

            i = j + 1;
        }

        if (bestByMm.isEmpty())
            return false;

        struct MmSlotSorter
        {
            static int compareElements (const Sy99CaptureManifestSlot& a,
                                      const Sy99CaptureManifestSlot& b) noexcept
            {
                return a.mm - b.mm;
            }
        };

        MmSlotSorter sorter;
        bestByMm.sort (sorter);

        for (const auto& slot : bestByMm)
            manifest.slots.add (slot);

        manifest.built = manifest.slots.size() > 0;
        return manifest.built;
    }

    juce::var manifestToJsonVar (const Sy99CaptureManifest& manifest) noexcept
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("version", kManifestJsonVersion);
        root->setProperty ("sourceModTimeMs", manifest.sourceModTimeMs);
        root->setProperty ("captureFile", juce::File (manifest.capturePath).getFileName());

        juce::Array<juce::var> slots;

        for (const auto& s : manifest.slots)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("mm", s.mm);
            obj->setProperty ("name", s.name);
            obj->setProperty ("elmodeRaw", s.elmodeRaw);
            obj->setProperty ("offset", s.offset);
            obj->setProperty ("length", s.length);
            obj->setProperty ("tag", s.tag);
            slots.add (obj);
        }

        root->setProperty ("slots", slots);
        return root;
    }

    bool manifestFromJsonVar (const juce::var& json, Sy99CaptureManifest& manifestOut) noexcept
    {
        if (! json.isObject())
            return false;

        manifestOut = {};
        manifestOut.sourceModTimeMs = (juce::int64) json.getProperty ("sourceModTimeMs", (juce::int64) 0);

        if (auto* slots = json.getProperty ("slots", {}).getArray())
        {
            for (const auto& item : *slots)
            {
                Sy99CaptureManifestSlot slot;
                slot.mm = (int) item.getProperty ("mm", 0);
                slot.name = item.getProperty ("name", {}).toString();
                slot.elmodeRaw = (int) item.getProperty ("elmodeRaw", -1);
                slot.offset = (int) item.getProperty ("offset", 0);
                slot.length = (int) item.getProperty ("length", 0);
                slot.tag = item.getProperty ("tag", "8101VC").toString();
                manifestOut.slots.add (slot);
            }
        }

        manifestOut.built = manifestOut.slots.size() > 0;
        return manifestOut.built;
    }
}

juce::File sy99CaptureManifestSidecarPath (const juce::File& captureFile) noexcept
{
    return captureFile.getSiblingFile (captureFile.getFileNameWithoutExtension() + ".syx.idx");
}

bool sy99WriteCaptureManifestToDisk (const juce::File& captureFile,
                                     const Sy99CaptureManifest& manifest) noexcept
{
    if (! captureFile.existsAsFile() || ! manifest.built)
        return false;

    const juce::File sidecar = sy99CaptureManifestSidecarPath (captureFile);
    const juce::var json = manifestToJsonVar (manifest);
    const juce::String text = juce::JSON::toString (json, true);

    if (! sidecar.getParentDirectory().exists())
        sidecar.getParentDirectory().createDirectory();

    if (! sidecar.replaceWithText (text))
        return false;

    juce::Logger::writeToLog ("[LibraryIndex] wrote manifest "
                              + sidecar.getFileName()
                              + " slots=" + juce::String (manifest.slots.size()));
    return true;
}

bool sy99LoadCaptureManifestFromDisk (const juce::File& captureFile,
                                      Sy99CaptureManifest& manifestOut) noexcept
{
    manifestOut = {};

    if (! captureFile.existsAsFile())
        return false;

    const juce::File sidecar = sy99CaptureManifestSidecarPath (captureFile);

    if (! sidecar.existsAsFile())
        return false;

    const juce::var json = juce::JSON::parse (sidecar.loadFileAsString());

    if (! manifestFromJsonVar (json, manifestOut))
        return false;

    const juce::int64 captureMtime = captureFile.getLastModificationTime().toMilliseconds();

    if (manifestOut.sourceModTimeMs != captureMtime)
        return false;

    manifestOut.capturePath = captureFile.getFullPathName();
    manifestOut.built = true;
    return true;
}

bool sy99WriteCaptureManifestFromMessages (const juce::Array<juce::MidiMessage>& messages,
                                           const juce::File& captureFile) noexcept
{
    if (! captureFile.existsAsFile() || messages.isEmpty())
        return false;

    Sy99CaptureManifest manifest;
    manifest.capturePath = captureFile.getFullPathName();
    manifest.sourceModTimeMs = captureFile.getLastModificationTime().toMilliseconds();

    int mm = 0;
    int fileOffset = 0;

    for (const auto& m : messages)
    {
        if (! m.isSysEx())
            continue;

        const auto* data = m.getRawData();
        const int frameLen = m.getRawDataSize();

        if (appendSlotFromFrame (data, frameLen, mm, manifest))
        {
            manifest.slots.getReference (manifest.slots.size() - 1).offset = fileOffset;
            ++mm;
        }

        fileOffset += frameLen;
    }

    manifest.built = manifest.slots.size() > 0;

    if (! manifest.built)
        return false;

    const juce::ScopedLock lock (gManifestLock);
    const juce::String key = captureFile.getFullPathName();
    gManifestLru.set (key, manifest);
    touchManifestLruKey (key);

    return sy99WriteCaptureManifestToDisk (captureFile, manifest);
}

bool sy99WriteCaptureManifestFromScanDeduped (const juce::File& captureFile,
                                              int expectedMmCount) noexcept
{
    if (! captureFile.existsAsFile())
        return false;

    if (expectedMmCount <= 0)
        expectedMmCount = inferExpectedMmCountFromCaptureFile (captureFile);

    if (expectedMmCount == 0 && is0040AuditCaptureFileName (captureFile.getFileName()))
    {
        juce::Logger::writeToLog ("[LibraryIndex] skip 0040 audit "
                                  + captureFile.getFileName());
        return false;
    }

    const auto t0 = juce::Time::getMillisecondCounterHiRes();

    juce::MemoryBlock mb;

    if (! captureFile.loadFileAsData (mb))
        return false;

    Sy99CaptureManifest manifest;

    if (! buildManifestDedupedFromFileBytes (captureFile,
                                            static_cast<const juce::uint8*> (mb.getData()),
                                            mb.getSize(),
                                            expectedMmCount,
                                            manifest))
        return false;

    const double ms = juce::Time::getMillisecondCounterHiRes() - t0;

    juce::Logger::writeToLog ("[LibraryIndex] repair dedup "
                              + captureFile.getFileName()
                              + " slots=" + juce::String (manifest.slots.size())
                              + " expectedMm=" + juce::String (expectedMmCount)
                              + " ms=" + juce::String (ms, 1));

    const juce::ScopedLock lock (gManifestLock);
    const juce::String key = captureFile.getFullPathName();
    gManifestLru.set (key, manifest);
    touchManifestLruKey (key);

    return sy99WriteCaptureManifestToDisk (captureFile, manifest);
}

bool sy99WriteCaptureManifestFromScan (const juce::File& captureFile) noexcept
{
    const int expected = inferExpectedMmCountFromCaptureFile (captureFile);
    return sy99WriteCaptureManifestFromScanDeduped (captureFile, expected);
}

void sy99CopyManifestToVoiceIndex (const Sy99CaptureManifest& manifest,
                                   Sy99VoiceCaptureIndex& indexOut) noexcept
{
    indexOut.offsets.clear();
    indexOut.lengths.clear();
    indexOut.names.clear();
    indexOut.elmodeRaws.clear();
    indexOut.sourceModTimeMs = manifest.sourceModTimeMs;
    indexOut.built = manifest.built;

    if (! manifest.built || manifest.slots.isEmpty())
        return;

    int slotCount = 0;

    for (const auto& s : manifest.slots)
        slotCount = juce::jmax (slotCount, s.mm + 1);

    indexOut.offsets.insertMultiple (0, -1, slotCount);
    indexOut.lengths.insertMultiple (0, 0, slotCount);
    indexOut.elmodeRaws.insertMultiple (0, -1, slotCount);

    for (int i = 0; i < slotCount; ++i)
        indexOut.names.add ({});

    for (const auto& s : manifest.slots)
    {
        if (s.mm < 0 || s.mm >= slotCount)
            continue;

        indexOut.offsets.set (s.mm, s.offset);
        indexOut.lengths.set (s.mm, s.length);
        indexOut.names.set (s.mm, s.name);
        indexOut.elmodeRaws.set (s.mm, s.elmodeRaw);
    }

    indexOut.built = true;
}

const Sy99CaptureManifest* sy99GetCaptureManifest (const juce::File& captureFile,
                                                   bool allowRepairScan) noexcept
{
    if (! captureFile.existsAsFile())
        return nullptr;

    const juce::String key = captureFile.getFullPathName();
    const juce::int64 modTimeMs = captureFile.getLastModificationTime().toMilliseconds();

    {
        const juce::ScopedLock lock (gManifestLock);

        if (gManifestLru.contains (key))
        {
            auto& cached = gManifestLru.getReference (key);

            if (cached.sourceModTimeMs == modTimeMs && cached.built)
            {
                juce::MemoryBlock mb;

                if (captureFile.loadFileAsData (mb))
                {
                    const int invalid = countInvalidManifestSlots (
                        static_cast<const juce::uint8*> (mb.getData()),
                        mb.getSize(),
                        cached);

                    if (invalid == 0)
                    {
                        touchManifestLruKey (key);
                        return &cached;
                    }
                }

                gManifestLru.remove (key);
                gManifestLruOrder.removeString (key);
            }
            else
            {
                gManifestLru.remove (key);
                gManifestLruOrder.removeString (key);
            }
        }
    }

    Sy99CaptureManifest loaded;

    if (sy99LoadCaptureManifestFromDisk (captureFile, loaded))
    {
        loaded.capturePath = key;

        juce::MemoryBlock mb;

        if (captureFile.loadFileAsData (mb))
        {
            const int invalid = countInvalidManifestSlots (static_cast<const juce::uint8*> (mb.getData()),
                                                           mb.getSize(),
                                                           loaded);

            if (invalid > 0)
            {
                juce::Logger::writeToLog ("[LibraryIndex] manifest repair invalid="
                                          + juce::String (invalid)
                                          + " file="
                                          + captureFile.getFileName());

                sy99CaptureManifestSidecarPath (captureFile).deleteFile();

                {
                    const juce::ScopedLock lock (gManifestLock);
                    gManifestLru.remove (key);
                    gManifestLruOrder.removeString (key);
                }

                if (! allowRepairScan)
                    return nullptr;

                const int expected = inferExpectedMmCountFromCaptureFile (captureFile);

                if (! sy99WriteCaptureManifestFromScanDeduped (captureFile, expected))
                    return nullptr;

                const juce::ScopedLock lock (gManifestLock);

                if (gManifestLru.contains (key))
                    return &gManifestLru.getReference (key);

                return nullptr;
            }
        }

        const juce::ScopedLock lock (gManifestLock);
        gManifestLru.set (key, loaded);
        touchManifestLruKey (key);
        return &gManifestLru.getReference (key);
    }

    if (! allowRepairScan)
        return nullptr;

    if (! sy99WriteCaptureManifestFromScanDeduped (captureFile,
                                                   inferExpectedMmCountFromCaptureFile (captureFile)))
        return nullptr;

    const juce::ScopedLock lock (gManifestLock);

    if (gManifestLru.contains (key))
        return &gManifestLru.getReference (key);

    return nullptr;
}

void sy99InvalidateCaptureManifest (const juce::File& captureFile) noexcept
{
    const juce::String key = captureFile.getFullPathName();

    {
        const juce::ScopedLock lock (gManifestLock);
        gManifestLru.remove (key);
        gManifestLruOrder.removeString (key);
    }

    sy99CaptureManifestSidecarPath (captureFile).deleteFile();
}

void sy99InvalidateAllCanonicalCaptureManifests() noexcept
{
    static const char* const prefixes[] =
    {
        "AUTOSYNC-VC-INT",
        "AUTOSYNC-VC-P2",
        "AUTOSYNC-VC-P3",
        "AUTOSYNC-MU-INT",
        "AUTOSYNC-MU-P2",
        "AUTOSYNC-VOICE64"
    };

    const juce::File cap = libraryCapturesDirPath();

    for (const char* prefix : prefixes)
        sy99InvalidateCaptureManifest (cap.getChildFile (juce::String (prefix) + ".syx"));
}

void sy99LoadManifestSlotNamesIntoArray (const juce::File& captureFile,
                                         juce::StringArray& namesOut,
                                         int slotCount) noexcept
{
    namesOut.clear();

    for (int i = 0; i < slotCount; ++i)
        namesOut.add ({});

    const Sy99CaptureManifest* manifest = sy99GetCaptureManifest (captureFile, true);

    if (manifest == nullptr)
        return;

    for (const auto& slot : manifest->slots)
    {
        if (slot.mm >= 0 && slot.mm < slotCount)
            namesOut.set (slot.mm, slot.name);
    }
}

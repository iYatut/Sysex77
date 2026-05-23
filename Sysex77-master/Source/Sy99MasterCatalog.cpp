/*
  ==============================================================================

    Sy99MasterCatalog.cpp

  ==============================================================================
*/

#include "Sy99MasterCatalog.h"

namespace Sy99MasterCatalog
{
    namespace
    {
        juce::Array<Entry> gEntries;
        juce::HashMap<juce::String, Binding> gBindings;
        bool gLoaded = false;

        juce::File resolveFixtureFile (const juce::String& fileName)
        {
            const juce::File appData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                           .getChildFile ("Application Support")
                                           .getChildFile ("Sysex77")
                                           .getChildFile (fileName);

            if (appData.existsAsFile())
                return appData;

            juce::File dir = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                                 .getParentDirectory();

            for (int depth = 0; depth < 10; ++depth)
            {
                const juce::File ctx = dir.getChildFile ("_agent_context")
                                           .getChildFile ("fixtures")
                                           .getChildFile (fileName);

                if (ctx.existsAsFile())
                    return ctx;

                const juce::File uiFix = dir.getChildFile ("ui")
                                              .getChildFile ("fixtures")
                                              .getChildFile (fileName);

                if (uiFix.existsAsFile())
                    return uiFix;

                const juce::File parent = dir.getParentDirectory();

                if (parent == dir)
                    break;

                dir = parent;
            }

            return appData;
        }

        void parseCatalogArray (const juce::var& parsed)
        {
            gEntries.clear();

            if (! parsed.isArray())
                return;

            for (const auto& item : *parsed.getArray())
            {
                auto* obj = item.getDynamicObject();

                if (obj == nullptr)
                    continue;

                Entry e;
                e.id = obj->getProperty ("id").toString();
                e.tag = obj->getProperty ("tag").toString();
                if (e.tag.isEmpty())
                    e.tag = e.id;

                e.uiLabel = obj->getProperty ("uiLabel").toString();
                if (e.uiLabel.isEmpty())
                    e.uiLabel = e.tag;

                e.group = obj->getProperty ("group").toString();
                e.sysexGroup = (int) obj->getProperty ("sysexGroup");
                e.nn = (int) obj->getProperty ("nn");
                e.perElement = (bool) obj->getProperty ("perElement");
                e.elementCount = (int) obj->getProperty ("elementCount");

                if (e.elementCount < 1)
                    e.elementCount = e.perElement ? 4 : 1;

                e.sy99LcdPage = (int) obj->getProperty ("sy99LcdPage");
                e.addressHex = obj->getProperty ("addressHex").toString();
                e.codec = obj->getProperty ("codec").toString();
                e.registryId = obj->getProperty ("registryId").toString();

                if (e.id.isNotEmpty())
                    gEntries.add (e);
            }
        }

        void parseBindingsArray (const juce::var& parsed)
        {
            gBindings.clear();

            if (! parsed.isArray())
                return;

            for (const auto& item : *parsed.getArray())
            {
                auto* obj = item.getDynamicObject();

                if (obj == nullptr)
                    continue;

                Binding b;
                b.id = obj->getProperty ("id").toString();
                b.bindStatus = obj->getProperty ("bindStatus").toString();
                b.registryId = obj->getProperty ("registryId").toString();
                b.bulkSource = obj->getProperty ("bulkSource").toString();
                b.notes = obj->getProperty ("notes").toString();

                if (auto* br = obj->getProperty ("bulkRead").getDynamicObject())
                {
                    b.bulkRead.frame = br->getProperty ("frame").toString();
                    b.bulkRead.field = br->getProperty ("field").toString();
                    b.bulkRead.perElement = (bool) br->getProperty ("perElement");

                    if (br->hasProperty ("maxElementIndex"))
                        b.bulkRead.maxElementIndex = (int) br->getProperty ("maxElementIndex");
                }

                if (b.id.isNotEmpty())
                    gBindings.set (b.id, b);
            }
        }
    }

    void ensureLoaded()
    {
        if (gLoaded)
            return;

        gLoaded = true;

        const juce::File catalogFile = resolveFixtureFile ("sy99_master_catalog.json");
        const juce::File bindingsFile = resolveFixtureFile ("sy99_param_bindings.json");

        if (catalogFile.existsAsFile())
        {
            const juce::var parsed = juce::JSON::parse (catalogFile.loadFileAsString());
            parseCatalogArray (parsed);
            juce::Logger::writeToLog ("[MasterCatalog] loaded " + juce::String (gEntries.size())
                                      + " entries from " + catalogFile.getFullPathName());
        }
        else
        {
            juce::Logger::writeToLog ("[MasterCatalog] missing sy99_master_catalog.json");
        }

        if (bindingsFile.existsAsFile())
        {
            const juce::var parsed = juce::JSON::parse (bindingsFile.loadFileAsString());
            parseBindingsArray (parsed);
            juce::Logger::writeToLog ("[MasterCatalog] bindings loaded " + juce::String (gBindings.size())
                                      + " entries from " + bindingsFile.getFullPathName());
        }
        else
        {
            juce::Logger::writeToLog ("[MasterCatalog] missing sy99_param_bindings.json");
        }
    }

    const juce::Array<Entry>& entries()
    {
        ensureLoaded();
        return gEntries;
    }

    juce::String bindStatusFor (const juce::String& paramId)
    {
        ensureLoaded();

        if (gBindings.contains (paramId))
            return gBindings[paramId].bindStatus;

        return "manual_only";
    }

    bool tryGetBinding (const juce::String& paramId, Binding& out)
    {
        ensureLoaded();

        if (! gBindings.contains (paramId))
            return false;

        out = gBindings[paramId];
        return true;
    }

    juce::String formatAddressForElement (const Entry& entry, int elementIndex)
    {
        juce::String addr = entry.addressHex;

        if (entry.perElement && elementIndex >= 0 && elementIndex < 4)
        {
            const int b4 = elementIndex * 0x20;
            addr = addr.replace ("EE", juce::String::toHexString (b4).toUpperCase().paddedLeft ('0', 2));
        }

        return addr;
    }

    juce::var statsToJsonVar (const Stats& stats)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("totalRows", stats.totalRows);
        obj->setProperty ("catalogParams", stats.catalogParams);
        obj->setProperty ("inDump", stats.inDump);
        obj->setProperty ("confirmed", stats.confirmed);
        obj->setProperty ("manualOnly", stats.manualOnly);
        obj->setProperty ("candidate", stats.candidate);
        return obj;
    }
} // namespace Sy99MasterCatalog

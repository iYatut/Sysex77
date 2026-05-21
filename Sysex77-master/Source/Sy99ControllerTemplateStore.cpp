/*
  ==============================================================================

    Sy99ControllerTemplateStore.cpp

  ==============================================================================
*/

#include "Sy99ControllerTemplateStore.h"

#include "LiveSynthState.h"
#include "Sy99ParamRegistry.h"

namespace
{
    juce::Array<juce::var> gTemplates;
    juce::CriticalSection gLock;
    bool gInitialized = false;

    juce::var defaultPush2Template()
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("id", "push2-default");
        root->setProperty ("name", "Push 2 default");

        juce::Array<juce::var> slots;
        const char* slotDefs[][3] = {
            { "WOL", "WOL", "0" },
            { "E1 Level", "ELVL", "0" },
            { "E2 Level", "ELVL", "1" },
            { "E3 Level", "ELVL", "2" },
            { "E4 Level", "ELVL", "3" },
        };

        for (const auto& def : slotDefs)
        {
            auto* slot = new juce::DynamicObject();
            slot->setProperty ("label", juce::String (def[0]));
            slot->setProperty ("paramId", juce::String (def[1]));
            slot->setProperty ("elementIndex", juce::String (def[2]).getIntValue());
            slots.add (slot);
        }

        auto* page = new juce::DynamicObject();
        page->setProperty ("name", "Mixer");
        page->setProperty ("slots", slots);

        juce::Array<juce::var> pages;
        pages.add (page);
        root->setProperty ("pages", pages);
        return root;
    }

    bool validateTemplateBody (const juce::var& body, juce::String& errorOut)
    {
        auto* obj = body.getDynamicObject();

        if (obj == nullptr)
        {
            errorOut = "expected JSON object";
            return false;
        }

        if (! obj->hasProperty ("id") || obj->getProperty ("id").toString().trim().isEmpty())
        {
            errorOut = "missing id";
            return false;
        }

        if (! obj->hasProperty ("name") || obj->getProperty ("name").toString().trim().isEmpty())
        {
            errorOut = "missing name";
            return false;
        }

        const juce::var pagesVar = obj->getProperty ("pages");

        if (! pagesVar.isArray())
        {
            errorOut = "pages must be an array";
            return false;
        }

        for (const auto& pageVar : *pagesVar.getArray())
        {
            auto* page = pageVar.getDynamicObject();

            if (page == nullptr || ! page->hasProperty ("name"))
            {
                errorOut = "each page requires name";
                return false;
            }

            const juce::var slotsVar = page->getProperty ("slots");

            if (! slotsVar.isArray())
            {
                errorOut = "page slots must be an array";
                return false;
            }

            for (const auto& slotVar : *slotsVar.getArray())
            {
                auto* slot = slotVar.getDynamicObject();

                if (slot == nullptr)
                {
                    errorOut = "invalid slot object";
                    return false;
                }

                const juce::String paramId = slot->getProperty ("paramId").toString().trim();

                if (paramId.isEmpty())
                {
                    errorOut = "slot requires paramId";
                    return false;
                }

                if (Sy99ParamRegistry::idFromString (paramId) >= Sy99ParamRegistry::Id::Count)
                {
                    errorOut = "unknown paramId: " + paramId;
                    return false;
                }

                const int elementIndex = (int) slot->getProperty ("elementIndex");

                if (elementIndex < 0 || elementIndex > 3)
                {
                    errorOut = "elementIndex out of range (0..3)";
                    return false;
                }
            }
        }

        return true;
    }

    void persistUnlocked()
    {
        const juce::File file = Sy99ControllerTemplates::templatesJsonFile();
        const juce::File parent = file.getParentDirectory();

        if (parent != juce::File() && ! parent.exists())
            parent.createDirectory();

        file.replaceWithText (juce::JSON::toString (gTemplates, true));
    }

    void loadFromDiskUnlocked()
    {
        gTemplates.clear();
        const juce::File file = Sy99ControllerTemplates::templatesJsonFile();

        if (! file.existsAsFile())
        {
            gTemplates.add (defaultPush2Template());
            persistUnlocked();
            return;
        }

        const juce::var parsed = juce::JSON::parse (file.loadFileAsString());

        if (parsed.isArray())
        {
            for (const auto& item : *parsed.getArray())
                if (item.isObject())
                    gTemplates.add (item);
        }

        if (gTemplates.isEmpty())
            gTemplates.add (defaultPush2Template());
    }

    int indexOfIdUnlocked (const juce::String& id)
    {
        for (int i = 0; i < gTemplates.size(); ++i)
            if (gTemplates[i].getProperty ("id", {}).toString().equalsIgnoreCase (id))
                return i;

        return -1;
    }

    void ensureInitialized()
    {
        if (gInitialized)
            return;

        loadFromDiskUnlocked();
        gInitialized = true;
    }
}

namespace Sy99ControllerTemplates
{
    void initializeAtStartup()
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();
        juce::Logger::writeToLog ("[ControllerTemplates] loaded "
                                  + juce::String (gTemplates.size()) + " template(s)");
    }

    juce::File templatesJsonFile()
    {
        return appDirPath.getChildFile ("controller_templates.json");
    }

    juce::var allToJsonVar()
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();
        return gTemplates;
    }

    juce::var getById (const juce::String& id)
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();

        const int idx = indexOfIdUnlocked (id.trim());

        if (idx < 0)
            return {};

        return gTemplates[idx];
    }

    bool createTemplate (const juce::String& id, const juce::var& body, juce::String& errorOut)
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();

        const juce::String trimmedId = id.trim();

        if (trimmedId.isEmpty())
        {
            errorOut = "missing template id";
            return false;
        }

        if (indexOfIdUnlocked (trimmedId) >= 0)
        {
            errorOut = "template already exists";
            return false;
        }

        if (! validateTemplateBody (body, errorOut))
            return false;

        auto* src = body.getDynamicObject();

        if (src == nullptr)
        {
            errorOut = "expected JSON object";
            return false;
        }

        juce::var stored = juce::JSON::parse (juce::JSON::toString (body));
        stored.getDynamicObject()->setProperty ("id", trimmedId);
        gTemplates.add (stored);
        persistUnlocked();
        return true;
    }

    bool upsertTemplate (const juce::String& id, const juce::var& body, juce::String& errorOut)
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();

        const juce::String trimmedId = id.trim();

        if (trimmedId.isEmpty())
        {
            errorOut = "missing template id";
            return false;
        }

        if (! validateTemplateBody (body, errorOut))
            return false;

        if (body.getProperty ("id", {}).toString().trim().isNotEmpty()
            && ! body.getProperty ("id", {}).toString().equalsIgnoreCase (trimmedId))
        {
            errorOut = "body id does not match URL id";
            return false;
        }

        auto* src = body.getDynamicObject();

        if (src == nullptr)
        {
            errorOut = "expected JSON object";
            return false;
        }

        juce::var stored = juce::JSON::parse (juce::JSON::toString (body));
        stored.getDynamicObject()->setProperty ("id", trimmedId);

        const int idx = indexOfIdUnlocked (trimmedId);

        if (idx >= 0)
            gTemplates.set (idx, stored);
        else
            gTemplates.add (stored);

        persistUnlocked();
        return true;
    }
}

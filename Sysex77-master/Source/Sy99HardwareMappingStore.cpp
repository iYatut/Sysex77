/*
  ==============================================================================

    Sy99HardwareMappingStore.cpp

  ==============================================================================
*/

#include "Sy99HardwareMappingStore.h"

#include "LiveSynthState.h"
#include "Sy99ControllerTemplateStore.h"
#include "Sy99ParamRegistry.h"

namespace
{
    juce::Array<juce::var> gMappings;
    juce::CriticalSection gLock;
    bool gInitialized = false;

    juce::var defaultLaunchControlMapping()
    {
        auto* root = new juce::DynamicObject();
        root->setProperty ("deviceId", "launchcontrol-xl");
        root->setProperty ("templateId", "push2-default");
        root->setProperty ("inputPortMatch", "Launch Control");
        root->setProperty ("feedbackPortMatch", "Launch Control");
        root->setProperty ("activePage", 0);

        juce::Array<juce::var> bindings;
        const int ccNums[] = { 21, 22, 23, 24, 25 };

        for (int i = 0; i < 5; ++i)
        {
            auto* binding = new juce::DynamicObject();
            binding->setProperty ("cc", ccNums[i]);
            binding->setProperty ("channel", 1);
            binding->setProperty ("page", 0);
            binding->setProperty ("slotIndex", i);
            bindings.add (binding);
        }

        root->setProperty ("bindings", bindings);
        return root;
    }

    bool validateBindingObject (const juce::var& bindingVar, juce::String& errorOut)
    {
        auto* binding = bindingVar.getDynamicObject();

        if (binding == nullptr)
        {
            errorOut = "invalid binding object";
            return false;
        }

        const int cc = (int) binding->getProperty ("cc");
        const int channel = (int) binding->getProperty ("channel");
        const int page = (int) binding->getProperty ("page");
        const int slotIndex = (int) binding->getProperty ("slotIndex");

        if (cc < 0 || cc > 127)
        {
            errorOut = "cc out of range (0..127)";
            return false;
        }

        if (channel < 1 || channel > 16)
        {
            errorOut = "channel out of range (1..16)";
            return false;
        }

        if (page < 0 || slotIndex < 0)
        {
            errorOut = "page and slotIndex must be >= 0";
            return false;
        }

        return true;
    }

    bool validateMappingBody (const juce::var& body, juce::String& errorOut)
    {
        auto* obj = body.getDynamicObject();

        if (obj == nullptr)
        {
            errorOut = "expected JSON object";
            return false;
        }

        if (! obj->hasProperty ("deviceId") || obj->getProperty ("deviceId").toString().trim().isEmpty())
        {
            errorOut = "missing deviceId";
            return false;
        }

        const juce::String templateId = obj->getProperty ("templateId").toString().trim();

        if (templateId.isEmpty())
        {
            errorOut = "missing templateId";
            return false;
        }

        if (Sy99ControllerTemplates::getById (templateId).isVoid())
        {
            errorOut = "unknown templateId: " + templateId;
            return false;
        }

        const juce::var bindingsVar = obj->getProperty ("bindings");

        if (! bindingsVar.isArray())
        {
            errorOut = "bindings must be an array";
            return false;
        }

        for (const auto& item : *bindingsVar.getArray())
            if (! validateBindingObject (item, errorOut))
                return false;

        return true;
    }

    void persistUnlocked()
    {
        const juce::File file = Sy99HardwareMappings::mappingsJsonFile();
        const juce::File parent = file.getParentDirectory();

        if (parent != juce::File() && ! parent.exists())
            parent.createDirectory();

        file.replaceWithText (juce::JSON::toString (gMappings, true));
    }

    void loadFromDiskUnlocked()
    {
        gMappings.clear();
        const juce::File file = Sy99HardwareMappings::mappingsJsonFile();

        if (! file.existsAsFile())
        {
            gMappings.add (defaultLaunchControlMapping());
            persistUnlocked();
            return;
        }

        const juce::var parsed = juce::JSON::parse (file.loadFileAsString());

        if (parsed.isArray())
            for (const auto& item : *parsed.getArray())
                if (item.isObject())
                    gMappings.add (item);

        if (gMappings.isEmpty())
            gMappings.add (defaultLaunchControlMapping());
    }

    int indexOfDeviceUnlocked (const juce::String& deviceId)
    {
        for (int i = 0; i < gMappings.size(); ++i)
            if (gMappings[i].getProperty ("deviceId", {}).toString().equalsIgnoreCase (deviceId))
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

namespace Sy99HardwareMappings
{
    void initializeAtStartup()
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();
        juce::Logger::writeToLog ("[HardwareMapping] loaded "
                                  + juce::String (gMappings.size()) + " mapping(s)");
    }

    juce::File mappingsJsonFile()
    {
        return appDirPath.getChildFile ("hardware_mappings.json");
    }

    juce::var allToJsonVar()
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();
        return gMappings;
    }

    juce::var getByDeviceId (const juce::String& deviceId)
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();

        const int idx = indexOfDeviceUnlocked (deviceId.trim());

        if (idx < 0)
            return {};

        return gMappings[idx];
    }

    bool createMapping (const juce::String& deviceId, const juce::var& body, juce::String& errorOut)
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();

        const juce::String trimmed = deviceId.trim();

        if (trimmed.isEmpty())
        {
            errorOut = "missing deviceId";
            return false;
        }

        if (indexOfDeviceUnlocked (trimmed) >= 0)
        {
            errorOut = "mapping already exists";
            return false;
        }

        if (! validateMappingBody (body, errorOut))
            return false;

        juce::var stored = juce::JSON::parse (juce::JSON::toString (body));
        stored.getDynamicObject()->setProperty ("deviceId", trimmed);
        gMappings.add (stored);
        persistUnlocked();
        return true;
    }

    bool upsertMapping (const juce::String& deviceId, const juce::var& body, juce::String& errorOut)
    {
        const juce::ScopedLock lock (gLock);
        ensureInitialized();

        const juce::String trimmed = deviceId.trim();

        if (trimmed.isEmpty())
        {
            errorOut = "missing deviceId";
            return false;
        }

        if (! validateMappingBody (body, errorOut))
            return false;

        if (body.getProperty ("deviceId", {}).toString().trim().isNotEmpty()
            && ! body.getProperty ("deviceId", {}).toString().equalsIgnoreCase (trimmed))
        {
            errorOut = "body deviceId does not match URL deviceId";
            return false;
        }

        juce::var stored = juce::JSON::parse (juce::JSON::toString (body));
        stored.getDynamicObject()->setProperty ("deviceId", trimmed);

        const int idx = indexOfDeviceUnlocked (trimmed);

        if (idx >= 0)
            gMappings.set (idx, stored);
        else
            gMappings.add (stored);

        persistUnlocked();
        return true;
    }
}

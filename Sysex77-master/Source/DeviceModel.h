/*
  ==============================================================================

    DeviceModel.h
    Created: 2026
    Author:  NseaProtector

    Singleton model representing the currently selected Yamaha synthesiser.
    Provides device-specific constants (voice count, wave XML, SysEx ID byte)
    and a Listener interface so other components can react to device changes.

  ==============================================================================
*/

#pragma once

//==============================================================================
enum class YamahaDevice
{
    SY77 = 1,
    TG77 = 2,
    SY99 = 3
};

//==============================================================================
class DeviceModel
{
public:
    //==========================================================================
    struct Listener
    {
        virtual ~Listener() = default;

        /** Called on the message thread whenever the active device changes. */
        virtual void deviceChanged (YamahaDevice newDevice) = 0;
    };

    //==========================================================================
    static DeviceModel& getInstance()
    {
        static DeviceModel instance;
        return instance;
    }

    //==========================================================================
    void setDevice (YamahaDevice device)
    {
        if (currentDevice != device)
        {
            currentDevice = device;
            listeners.call ([device] (Listener& l) { l.deviceChanged (device); });
        }
    }

    YamahaDevice getDevice() const noexcept { return currentDevice; }

    //==========================================================================
    /** Total number of internal voice slots for the active device. */
    int getVoiceCount() const noexcept
    {
        return (currentDevice == YamahaDevice::SY99) ? 128 : 64;
    }

    /** All three models include a RAM bank. */
    bool hasRAM() const noexcept { return true; }

    /** Name of the binary-resource XML that lists the PCM wave table. */
    String getWavesXML() const
    {
        return (currentDevice == YamahaDevice::SY99) ? "SY99Waves.xml"
                                                     : "SY77Waves.xml";
    }

    /**
     * Returns the SysEx device-ID byte (second byte of Yamaha parameter-change
     * messages, e.g. 0x10 for unit/channel 1).  The value follows sysexEngine
     * so it stays consistent with the channel selector in ConfigPage.
     */
    uint8 getSysExDeviceID() const noexcept { return sysexEngine; }

    //==========================================================================
    void addListener    (Listener* l) { listeners.add    (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

private:
    DeviceModel() = default;
    DeviceModel (const DeviceModel&) = delete;
    DeviceModel& operator= (const DeviceModel&) = delete;

    YamahaDevice          currentDevice { YamahaDevice::SY77 };
    ListenerList<Listener> listeners;
};

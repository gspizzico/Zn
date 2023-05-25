#include <Znpch.h>
#include "Core/Log/OutputDeviceManager.h"

Zn::OutputDeviceManager& Zn::OutputDeviceManager::Get()
{
    static Zn::OutputDeviceManager s_Instance;
    return s_Instance;
}

bool Zn::OutputDeviceManager::OutputMessage(const char* message)
{
    for (auto&& Device : OutputDevices)
    {
        Device->OutputMessage(message);
    }

    return OutputDevices.size() > 0;
}

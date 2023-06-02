#include <RHI/RHIDevice.h>
#include <Core/CoreAssert.h>

using namespace Zn;

namespace
{
RHIDevice* GRHIDevice = nullptr;
}

RHIDevice& RHIDevice::Get()
{
    checkMsg(GRHIDevice, "Trying to get RHI Device before it's initialized!");
    return *GRHIDevice;
}

void RHIDevice::Create()
{
    check(GRHIDevice == nullptr);
    GRHIDevice = new RHIDevice();
}

void Zn::RHIDevice::Destroy()
{
    check(GRHIDevice);

    delete GRHIDevice;
    GRHIDevice = nullptr;
}

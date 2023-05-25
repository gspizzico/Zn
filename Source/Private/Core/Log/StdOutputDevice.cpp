#include <Znpch.h>
#include <Core/Log/StdOutputDevice.h>

#include <iostream>

using namespace Zn;

void StdOutputDevice::OutputMessage(const char* message)
{
    std::cout << message;
}
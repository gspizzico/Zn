#include "Core/Windows/WindowsDebugOutput.h"
#include "Core/Windows/WindowsCommon.h"


void Zn::WindowsDebugOutput::OutputMessage(const char* message)
{
    OutputDebugStringA(message);
}
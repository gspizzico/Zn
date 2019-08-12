#include "Core/Windows/WindowsDebugOutput.h"
#include <windows.h>

void Zn::WindowsDebugOutput::OutputMessage(const char* message)
{
    OutputDebugStringA(message);
}
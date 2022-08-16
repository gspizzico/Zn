#include "Windows/WindowsDebugOutput.h"
#include "Windows/WindowsCommon.h"


void Zn::WindowsDebugOutput::OutputMessage(const char* message)
{
	OutputDebugStringA(message);
}
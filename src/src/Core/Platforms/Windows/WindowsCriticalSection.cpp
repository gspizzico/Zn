#include <Core/Corepch.h>
#include "Windows/WindowsCriticalSection.h"

namespace Zn
{
WindowsCriticalSection::WindowsCriticalSection()
{
    ::InitializeCriticalSectionAndSpinCount(&nativeHandle, 2000);
}
WindowsCriticalSection::~WindowsCriticalSection()
{
    ::DeleteCriticalSection(&nativeHandle);
}
void WindowsCriticalSection::Lock()
{
    ::EnterCriticalSection(&nativeHandle);
}
void WindowsCriticalSection::Unlock()
{
    ::LeaveCriticalSection(&nativeHandle);
}
bool WindowsCriticalSection::TryLock()
{
    return bool(::TryEnterCriticalSection(&nativeHandle));
}
} // namespace Zn

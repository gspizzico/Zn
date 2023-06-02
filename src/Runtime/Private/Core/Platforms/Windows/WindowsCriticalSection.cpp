#include <Windows/WindowsCriticalSection.h>
#include <Windows/WindowsPrivate.h>

namespace Zn
{
struct WindowsCriticalSection::CriticalSectionHandle
{
    CRITICAL_SECTION handle;
};
WindowsCriticalSection::WindowsCriticalSection()
{
    criticalSection = new CriticalSectionHandle();

    ::InitializeCriticalSectionAndSpinCount(&criticalSection->handle, 2000);
}
WindowsCriticalSection::~WindowsCriticalSection()
{
    ::DeleteCriticalSection(&criticalSection->handle);
    delete criticalSection;
}
void WindowsCriticalSection::Lock()
{
    ::EnterCriticalSection(&criticalSection->handle);
}
void WindowsCriticalSection::Unlock()
{
    ::LeaveCriticalSection(&criticalSection->handle);
}
bool WindowsCriticalSection::TryLock()
{
    return bool(::TryEnterCriticalSection(&criticalSection->handle));
}
} // namespace Zn

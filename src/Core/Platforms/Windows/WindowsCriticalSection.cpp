#include <Corepch.h>
#include "Windows/WindowsCriticalSection.h"

namespace Zn
{
WindowsCriticalSection::WindowsCriticalSection()
{
    ::InitializeCriticalSectionAndSpinCount(&m_NativeCriticalSection, 2000);
}
WindowsCriticalSection::~WindowsCriticalSection()
{
    ::DeleteCriticalSection(&m_NativeCriticalSection);
}
void WindowsCriticalSection::Lock()
{
    ::EnterCriticalSection(&m_NativeCriticalSection);
}
void WindowsCriticalSection::Unlock()
{
    ::LeaveCriticalSection(&m_NativeCriticalSection);
}
bool WindowsCriticalSection::TryLock()
{
    return bool(::TryEnterCriticalSection(&m_NativeCriticalSection));
}
} // namespace Zn

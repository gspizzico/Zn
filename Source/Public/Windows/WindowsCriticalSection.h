#pragma once

#include "Windows/WindowsCommon.h"

namespace Zn
{
struct WindowsCriticalSection
{
    WindowsCriticalSection();
    ~WindowsCriticalSection();

    void Lock();

    void Unlock();

    bool TryLock();

  private:
    CRITICAL_SECTION m_NativeCriticalSection;
};
} // namespace Zn

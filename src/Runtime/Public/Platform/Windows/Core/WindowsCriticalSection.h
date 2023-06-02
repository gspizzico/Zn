#pragma once

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
    struct CriticalSectionHandle;
    CriticalSectionHandle* criticalSection;
};
} // namespace Zn

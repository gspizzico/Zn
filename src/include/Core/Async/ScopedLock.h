#pragma once

namespace Zn
{
template<typename TLockable>
struct TScopedLock
{
    TScopedLock(TLockable& lockable_)
        : lockable(&lockable_)
    {
        lockable->Lock();
    }

    ~TScopedLock()
    {
        lockable->Unlock();
    }

  private:
    TLockable* lockable;
};
} // namespace Zn

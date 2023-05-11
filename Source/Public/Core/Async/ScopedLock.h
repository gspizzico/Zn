#pragma once

namespace Zn
{
template<typename TLockable> struct TScopedLock
{
    TScopedLock(TLockable* lockable)
        : m_Lockable(lockable)
    {
        m_Lockable->Lock();
    }

    ~TScopedLock()
    {
        m_Lockable->Unlock();
    }

  private:
    TLockable* m_Lockable;
};
} // namespace Zn

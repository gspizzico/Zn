#pragma once

#include <Core/CoreTypes.h>
#include <Core/CoreAssert.h>
#include <Core/Misc/Hash.h>
#include <optional>

namespace Zn
{
struct EventHandle
{
    uint64 handle = 0;

    operator bool()
    {
        return handle != 0;
    }
};
template<typename Signature>
class TSingleEvent;

template<typename R, typename... Args>
class TSingleEvent<R(Args...)>
{
  public:
    TSingleEvent() = default;

    TSingleEvent(TDelegate<R(Args...)>&& delegate_)
        : delegate(delegate_)
    {
    }

    inline bool IsBound() const
    {
        return delegate;
    }

    inline std::optional<R> ExecuteSafe(Args&&... args_) noexcept
    {
        if (IsBound())
        {
            return delegate(std::forward<Args>(args_)...);
        }

        return std::optional<R>();
    }

    inline R ExecuteUnsafe(Args&&... args_)
    {
        return delegate(std::forward<Args>(args_)...);
    }

  private:
    TDelegate<R(Args...)> delegate;
};

template<typename... Args>
class TMulticastEvent
{
  public:
    TMulticastEvent() = default;

    inline EventHandle Bind(TDelegate<void(Args...)>&& delegate_)
    {
        if (delegate_)
        {
            EventHandle handle {HashCombine(HashCalculate(delegate_), HashCalculate(*this))};
            delegates.push_back({handle, std::move(delegate_)});
            return handle;
        }

        return EventHandle {};
    }

    inline void Unbind(EventHandle& handle_)
    {
        for (auto it = delegates.begin(); it != delegates.end(); ++it)
        {
            if ((*it).first == handle_)
            {
                delegates.erase(it);
                handle_ = EventHandle();
                break;
            }
        }
    }

    inline void Broadcast(const Args&... args_) const
    {
        for (const std::pair<EventHandle, TDelegate<void(Args...)>>& delegate : delegates)
        {
            check(delegate.second);

            delegate.second(std::forward<const Args&>(args_)...);
        }
    }

    inline void Reset()
    {
        delegates.clear();
    }

  private:
    Vector<std::pair<EventHandle, TDelegate<void(Args...)>>> delegates;
};
} // namespace Zn

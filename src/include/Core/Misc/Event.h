#pragma once

#include <CoreTypes.h>
#include <CoreAssert.h>
#include <optional>

namespace Zn
{
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

    inline void Bind(TDelegate<void(Args...)>&& delegate_)
    {
        if (delegate_)
        {
            delegates.push_back(std::move(delegate_));
        }
    }

    inline void Broadcast(Args&&... args_) const
    {
        for (const TDelegate<void(Args...)>& delegate : delegates)
        {
            check(delegate);

            delegate(std::forward<Args>(args_)...);
        }
    }

    inline void Reset()
    {
        delegates.clear();
    }

  private:
    Vector<TDelegate<void(Args...)>> delegates;
};
} // namespace Zn

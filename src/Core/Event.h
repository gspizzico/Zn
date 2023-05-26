#pragma once

#include <Types.h>
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

    TSingleEvent(TDelegate<R(Args...)>&& inDelegate)
        : delegate(inDelegate)
    {
    }

    inline bool IsBound() const
    {
        return delegate;
    }

    inline std::optional<R> ExecuteSafe(Args&&... arguments) noexcept
    {
        if (IsBound())
        {
            return delegate(std::forward<Args>(arguments)...);
        }

        return std::optional<R>();
    }

    inline R ExecuteUnsafe(Args&&... arguments)
    {
        return delegate(std::forward<Args>(arguments)...);
    }

  private:
    TDelegate<R(Args...)> delegate;
};

template<typename... Args>
class TMulticastEvent
{
  public:
    TMulticastEvent() = default;

    inline void Bind(TDelegate<void(Args...)>&& inDelegate)
    {
        if (inDelegate)
        {
            delegates.push_back(std::move(inDelegate));
        }
    }

    inline void Broadcast(Args&&... arguments) const
    {
        for (const TDelegate<void(Args...)>& delegate : delegates)
        {
            check(delegate);

            delegate(std::forward<Args>(arguments)...);
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

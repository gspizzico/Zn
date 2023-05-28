#pragma once

#include "Async/ThreadedJob.h"
#include <functional>

namespace Zn
{
class CallableJob : public ThreadedJob
{
    CallableJob(std::function<void()>&& callable_)
        : callable(std::move(callable_))
    {
    }

    virtual void DoWork() override
    {
        callable();
    }

    virtual void Finalize() override
    {
        callable = nullptr;
    }

  private:
    std::function<void()> callable;
};
} // namespace Zn

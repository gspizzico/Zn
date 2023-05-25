#pragma once

namespace Zn
{
class ThreadedJob
{
  public:
    virtual void Prepare() {};
    virtual void DoWork() = 0;
    virtual void Finalize() {};
};
} // namespace Zn

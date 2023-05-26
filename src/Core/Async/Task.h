#pragma once
#include "Async/ITaskGraphNode.h"

namespace Zn
{
class Task abstract : public ITaskGraphNode, public std::enable_shared_from_this<Task>
{
  public:
    enum class State : uint8_t
    {
        Idle,
        InProgress,
        Completed,
        Aborted
    };

    virtual void Run() = 0;

    virtual void Abort() = 0;

  private:
    State m_State = State::Idle;
};
} // namespace Zn
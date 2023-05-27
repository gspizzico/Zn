#pragma once
#include "Core/Name.h"
#include "Core/HAL/BasicTypes.h"

namespace Zn
{
class ITaskGraphNode abstract
{
  public:
    friend class TaskManager;

    virtual void Execute() = 0;

    virtual Name GetName() const = 0;

    virtual void DumpNode() const = 0;

  private:
    SharedPtr<class TaskHandle> m_Handle;
};
} // namespace Zn
#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
class ApplicationEventHandler
{
  public:
    static ApplicationEventHandler& Get();
    static void                     SetEventHandler(ApplicationEventHandler* eventHandler_);

    virtual ~ApplicationEventHandler() = default;

    virtual void OnWindowSizeChanged(uint32 width_, uint32 height_)
    {
    }

    virtual void OnWindowMinimized()
    {
    }

    virtual void OnWindowRestored()
    {
    }
};
} // namespace Zn

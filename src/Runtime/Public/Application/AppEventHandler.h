#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
class AppEventHandler abstract
{
  public:
    static AppEventHandler& Get();
    static void             SetEventHandler(AppEventHandler* eventHandler_);

    virtual ~AppEventHandler() = default;

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

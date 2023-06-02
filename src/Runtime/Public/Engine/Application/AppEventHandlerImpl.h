#pragma once

#include <Core/CoreTypes.h>
#include <Application/AppEventHandler.h>

namespace Zn
{
class AppEventHandlerImpl : public AppEventHandler
{
  public:
    virtual void OnWindowSizeChanged(uint32 width_, uint32 height_) override;
    virtual void OnWindowMinimized() override;
    virtual void OnWindowRestored() override;
};
} // namespace Zn

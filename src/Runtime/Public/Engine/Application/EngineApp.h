#pragma once

#include <Core/CoreTypes.h>
#include <Application/ApplicationEventHandler.h>

namespace Zn
{
class EngineApp : public ApplicationEventHandler
{
  public:
    virtual void OnWindowSizeChanged(uint32 width_, uint32 height_) override;
    virtual void OnWindowMinimized() override;
    virtual void OnWindowRestored() override;
};
} // namespace Zn

#pragma once

#include <CoreTypes.h>
#include <ApplicationEventHandler.h>

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

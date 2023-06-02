#pragma once

#include <Core/CoreMinimal.h>
#include <Application/Application.h>

namespace Zn
{
class SDLApplication : public Application
{
  public:
    void                          Initialize();
    void                          Shutdown();
    virtual bool                  ProcessOSEvents(float deltaTime_) override;
    virtual WindowHandle          GetWindowHandle() const override;
    virtual SharedPtr<InputState> GetInputState() const override;
    virtual void                  RequestExit(cstring exitReason_) override;
    virtual bool                  WantsToExit() const override;

  private:
    cstring                    title;
    SharedPtr<class SDLWindow> window {};
    SharedPtr<InputState>      inputState {};
    bool                       isInitialized   = false;
    bool                       isExitRequested = false;
};
} // namespace Zn

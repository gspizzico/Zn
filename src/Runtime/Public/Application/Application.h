#pragma once

#include <Core/CoreTypes.h>

namespace Zn
{
struct WindowHandle
{
    void*  handle;
    uint64 id;
};

class Application
{
  public:
    static Application& Get();
    static void         Create();
    static void         Destroy();

    virtual void                         Initialize()                      = 0;
    virtual void                         Shutdown()                        = 0;
    virtual bool                         ProcessOSEvents(float deltaTime_) = 0;
    virtual WindowHandle                 GetWindowHandle() const           = 0;
    virtual SharedPtr<struct InputState> GetInputState() const             = 0;
    virtual void                         RequestExit(cstring exitReason_)  = 0;
    virtual bool                         WantsToExit() const               = 0;

  protected:
    static void SetApplication(Application* application_);
};
} // namespace Zn

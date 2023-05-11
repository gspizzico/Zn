#pragma once

#include <Core/HAL/BasicTypes.h>

namespace Zn
{
class Window;
class Engine;
struct InputState;

class Application
{
  public:
    static Application& Get();

    void Initialize();

    void Shutdown();

    bool ProcessOSEvents(float deltaTime);

    SharedPtr<Window> GetWindow() const;

    SharedPtr<InputState> GetInputState() const;

    void RequestExit(String exitReason);

    bool WantsToExit() const;

  private:
    Application() = default;

    SharedPtr<Window> window;

    SharedPtr<InputState> input;

    bool is_initialized = false;

    bool is_exit_requested = false;
};
} // namespace Zn

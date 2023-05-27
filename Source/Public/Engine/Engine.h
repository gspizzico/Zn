#pragma once
#include <Core/Types.h>

namespace Zn
{
class Camera;

class EngineFrontend;
class Window;

class Engine
{
  public:
    void Initialize();

    void Update(float deltaTime);

    void Shutdown();

  private:
    // Render editor ImGui
    void RenderUI(float deltaTime);

    void ProcessInput();

    bool PumpMessages();

    float m_DeltaTime {0.f};

    SharedPtr<Window> m_Window;

    SharedPtr<Camera> activeCamera;

    SharedPtr<EngineFrontend> m_FrontEnd;
};
} // namespace Zn

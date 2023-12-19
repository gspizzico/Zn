#pragma once

#include <Core/CoreMinimal.h>
#include <glm/vec3.hpp>
#include <Rendering/RendererTypes.h>

namespace Zn
{
enum class RendererBackendType : uint8
{
    Vulkan
};

class Renderer
{
  public:
    static Renderer& Get();

    static bool Create(RendererBackendType type_);
    static bool Destroy();

    virtual bool Initialize()                                                 = 0;
    virtual void Shutdown()                                                   = 0;
    virtual bool BeginFrame()                                                 = 0;
    virtual bool Render(float deltaTime_, std::function<void(float)> render_) = 0;
    virtual bool EndFrame()                                                   = 0;
    virtual void OnWindowResized(uint32 width_, uint32 height_)               = 0;
    virtual void OnWindowMinimized()                                          = 0;
    virtual void OnWindowRestored()                                           = 0;
    virtual void SetCamera(const ViewInfo& viewInfo)                          = 0;
    virtual void SetLight(glm::vec3 light, float distance, float intensity)   = 0;

  private:
    static UniquePtr<Renderer> instance;

  protected:
    Renderer() = default;
};
} // namespace Zn

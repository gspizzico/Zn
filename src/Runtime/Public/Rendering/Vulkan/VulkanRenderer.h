#pragma once

#include <Core/CoreTypes.h>
#include <Rendering/Vulkan/VulkanTypes.h>
#include <Rendering/Vulkan/VulkanDevice.h>
#include <Rendering/Renderer.h>

namespace Zn
{
class VulkanRenderer : public Renderer
{
  public:
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual bool BeginFrame() override;
    virtual bool Render(float deltaTime, std::function<void(float)> render) override;
    virtual bool EndFrame() override;
    virtual void OnWindowResized(uint32 width_, uint32 height_) override;
    virtual void OnWindowMinimized() override;
    virtual void OnWindowRestored() override;
    virtual void SetCamera(const ViewInfo& viewInfo) override;
    virtual void SetLight(glm::vec3 light, float distance, float intensity) override;

  private:
    UniquePtr<VulkanDevice> device;
};
} // namespace Zn

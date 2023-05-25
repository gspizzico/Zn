#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Rendering/Vulkan/VulkanTypes.h>
#include <Rendering/Vulkan/VulkanDevice.h>
#include <Rendering/Renderer.h>

namespace Zn
{
class VulkanRenderer : public Renderer
{
  public:
    virtual bool initialize(RendererInitParams params) override;
    virtual void shutdown() override;
    virtual bool begin_frame() override;
    virtual bool render_frame(float deltaTime, std::function<void(float)> render) override;
    virtual bool end_frame() override;
    virtual void on_window_resized() override;
    virtual void on_window_minimized() override;
    virtual void on_window_restored() override;
    virtual void set_camera(const ViewInfo& viewInfo) override;
    virtual void set_light(glm::vec3 light, float distance, float intensity) override;

  private:
    UniquePtr<VulkanDevice> device;
};
} // namespace Zn

#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Rendering/RendererBackend.h>
#include <Rendering/Vulkan/VulkanTypes.h>
#include <Rendering/Vulkan/VulkanDevice.h>

namespace Zn
{
	class VulkanBackend : public RendererBackend
	{
	public:

		virtual bool initialize(RendererBackendInitData data) override;
		virtual void shutdown() override;
		virtual bool begin_frame() override;
		virtual bool render_frame() override;
		virtual bool end_frame() override;
		virtual void on_window_resized() override;
		virtual void on_window_minimized() override;
		virtual void on_window_restored() override;
		virtual void set_camera(glm::vec3 position, glm::vec3 direction) override;

	private:

		VkInstance instance{ VK_NULL_HANDLE };
		VkSurfaceKHR surface{ VK_NULL_HANDLE };

		UniquePtr<VulkanDevice> device;
	};
}
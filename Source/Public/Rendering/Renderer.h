#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Rendering/RendererTypes.h>

namespace Zn
{
	enum RendererBackendType
	{
		Vulkan,
		DX12
	};

	struct Camera;

	class Renderer
	{
	public:

		static Renderer& get();

		static bool initialize(RendererBackendType type, RendererInitParams data);
		static bool destroy();

		virtual bool initialize(RendererInitParams data) = 0;
		virtual void shutdown() = 0;
		virtual bool begin_frame() = 0;
		virtual bool render_frame(float deltaTime, std::function<void(float)> render) = 0;
		virtual bool end_frame() = 0;
		virtual void on_window_resized() = 0;
		virtual void on_window_minimized() = 0;
		virtual void on_window_restored() = 0;
		virtual void set_camera(glm::vec3 position, glm::vec3 direction) = 0;
		
		// static void destroy();

		// static bool render_frame(float deltaTime, std::function<void(float)> render);

		// static void set_camera(Camera camera);

	private:

		static UniquePtr<Renderer> instance;

	protected:

		Renderer() = default;
	};
}
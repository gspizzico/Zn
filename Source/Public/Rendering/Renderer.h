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

		static bool create(RendererBackendType type);

		static bool initialize(RendererBackendInitData data);
		
		static void destroy();

		static bool begin_frame();

		static bool render_frame();

		static bool end_frame();

		static void on_window_resized();
		
		static void on_window_minimized();

		static void on_window_restored();

		static void set_camera(Camera camera);

	private:

		Renderer() = default;
	};
}
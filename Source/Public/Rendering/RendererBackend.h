#pragma once

#include <Rendering/RendererTypes.h>
#include <glm/glm.hpp>

namespace Zn
{
	class RendererBackend abstract
	{
	public:

		virtual bool initialize(RendererBackendInitData data) = 0;
		virtual void shutdown() = 0;
		virtual bool begin_frame() = 0;
		virtual bool render_frame() = 0;
		virtual bool end_frame() = 0;
		virtual void on_window_resized() = 0;
		virtual void on_window_minimized() = 0;
		virtual void on_window_restored() = 0;
		virtual void set_camera(glm::vec3 position, glm::vec3 direction) = 0;
	};
}
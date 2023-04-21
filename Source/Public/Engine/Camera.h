#pragma once

#include <Core/HAL/BasicTypes.h>
#include <glm/glm.hpp>

union SDL_Event;

namespace Zn
{
	struct Camera
	{
		glm::vec3 position;
		glm::vec3 direction;
	};

	void camera_move(glm::vec3 direction, Camera& camera);
	void camera_rotate(glm::vec2 rotation, Camera& camera);

	void camera_process_input(const SDL_Event& event, f32 deltaTime, Camera& camera);
}

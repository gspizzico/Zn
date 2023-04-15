#pragma once

#include <Core/HAL/BasicTypes.h>
#include <glm/glm.hpp>

namespace Zn
{
	struct Camera
	{
		glm::vec3 position;
		glm::vec3 direction;
	};

	void move_camera(glm::vec3 direction, Camera& camera);
	void rotate_camera(glm::vec2 rotation, Camera& camera);
}

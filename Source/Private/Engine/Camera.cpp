#include <Znpch.h>
#include <Engine/Camera.h>

#include <glm/ext.hpp>
#include <SDL.h>

using namespace Zn;

static const glm::vec3 kUpVector{ 0.0f, 1.f, 0.f };

void Zn::camera_move(glm::vec3 direction, Camera& camera)
{
	camera.direction = glm::normalize(camera.direction);
	glm::vec3 right = glm::normalize(glm::cross(camera.direction, kUpVector));

	camera.position += (direction.z * camera.direction);
	camera.position += (direction.y * kUpVector);
	camera.position += (direction.x * right);
}

void Zn::camera_rotate(glm::vec2 rotation, Camera & camera)
{
	glm::vec3 direction = glm::normalize(camera.direction);
	glm::vec3 right = glm::normalize(glm::cross(direction, kUpVector));
	glm::vec3 up = glm::normalize(glm::cross(right, direction));

	glm::mat4 rotationMatrix = glm::mat4(1.f);
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), right);
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(-rotation.x), up);

	camera.direction = glm::vec3(rotationMatrix * glm::vec4(camera.direction, 0.f));
}

void Zn::camera_process_input(const SDL_Event& event, f32 deltaTime, Camera& camera)
{
	static const float kSpeed = 15.f;

	switch (event.type)
	{
	case SDL_KEYDOWN:
	{
		glm::vec3 delta_movement{ 0.f };

		switch (event.key.keysym.scancode)
		{
		case SDL_SCANCODE_A:
			delta_movement = (glm::vec3(-kSpeed * deltaTime, 0.f, 0.f));
			break;
		case SDL_SCANCODE_D:
			delta_movement = (glm::vec3(kSpeed * deltaTime, 0.f, 0.f));
			break;
		case SDL_SCANCODE_S:
			delta_movement = (glm::vec3(0.f, 0.f, -kSpeed * deltaTime));
			break;
		case SDL_SCANCODE_W:
			delta_movement = (glm::vec3(0.f, 0.f, kSpeed * deltaTime));
			break;
		case SDL_SCANCODE_Q:
			delta_movement = (glm::vec3(0.f, -kSpeed * deltaTime, 0.f));
			break;
		case SDL_SCANCODE_E:
			delta_movement = (glm::vec3(0.f, kSpeed * deltaTime, 0.f));
			break;
		}

		if (delta_movement.length() > 0.f)
		{
			camera_move(delta_movement, camera);
		}
	}
	break;
	case SDL_KEYUP:
		break;

	case SDL_MOUSEBUTTONDOWN:
	{
	}

	case SDL_MOUSEMOTION:
	{
		if (event.motion.state & SDL_BUTTON_RMASK)
		{
			float sensitivity = 0.1f;

			glm::vec2 rotation
			{
				event.motion.xrel * sensitivity,
				-event.motion.yrel * sensitivity
			};

			camera_rotate(rotation, camera);

			SDL_SetRelativeMouseMode(SDL_TRUE);
		}
		else
		{
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}
		break;
	}
	default:
		break;
	}
}

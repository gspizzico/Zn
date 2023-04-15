#include <Znpch.h>
#include <Input/Input.h>
#include <SDL2/SDL.h>
#include <Engine/Camera.h>

#include <glm/glm.hpp>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogInput, ELogVerbosity::Log);

bool Zn::Input::sdl_process_input(SDL_Event& event, float deltaTime, Camera& camera)
{
	static const float kSpeed = 15.f;

	switch (event.type)
	{
		case SDL_KEYDOWN:
		{
			ZN_LOG(LogInput, ELogVerbosity::Log, "Pressed key %d", event.key.keysym.scancode);

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
				delta_movement= (glm::vec3(0.f, -kSpeed * deltaTime, 0.f));
				break;
				case SDL_SCANCODE_E:
				delta_movement =(glm::vec3(0.f, kSpeed * deltaTime, 0.f));
				break;
			}

			if (delta_movement.length() > 0.f)
			{
				move_camera(delta_movement, camera);
			}
		}
		break;
		case SDL_KEYUP:
		ZN_LOG(LogInput, ELogVerbosity::Log, "Released key %d", event.key.keysym.scancode);
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

				rotate_camera(rotation, camera);

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

	return true;
}
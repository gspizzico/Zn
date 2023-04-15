#pragma once

#include <Core/HAL/BasicTypes.h>
#include <SDL.h>

namespace Zn
{
	struct Camera;

	class Input
	{
	public:

		static bool sdl_process_input(SDL_Event& event, float deltaTime, Camera& camera);
	};
}
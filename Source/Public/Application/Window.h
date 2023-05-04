#pragma once

#include <Core/HAL/BasicTypes.h>

struct SDL_Window;
struct SDL_Surface;
union SDL_Event;

namespace Zn
{
	class Window
	{
	public:

		Window(const int width, const int height, const String& title);

		~Window();

		bool ProcessEvent(SDL_Event event);

		u32 GetSDLWindowID() const { return windowID; }

	private:

		SDL_Window* window{ nullptr };
		uint32 windowID{ 0 };
		void* nativeHandle{nullptr};
	};
}

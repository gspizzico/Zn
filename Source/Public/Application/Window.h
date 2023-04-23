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

		u32 GetSDLWindowID() const { return m_SDLWindowID; }

	private:

		SDL_Window* m_Window{ nullptr };
		SDL_Surface* m_Canvas{ nullptr };
		uint32 m_SDLWindowID{ 0 };

		uint64 m_FrameNumber{ 0 };

		void* m_NativeHandle{nullptr};

		bool m_IsMinimized{ false };
	};
}

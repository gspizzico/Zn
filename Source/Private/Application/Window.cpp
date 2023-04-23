#include <Znpch.h>
#include <Application/Window.h>
#include <Core/HAL/SDL/SDLWrapper.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <ImGui/ImGuiWrapper.h>
#include <Rendering/Renderer.h>

DEFINE_STATIC_LOG_CATEGORY(LogWindow, ELogVerbosity::Log);

using namespace Zn;

Window::Window(const int width, const int height, const String& title)
{
	_ASSERT(SDLWrapper::IsInitialized());

	// Create window at default position

	static auto ZN_SDL_WINDOW_FLAGS = (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);

	m_Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, ZN_SDL_WINDOW_FLAGS);

	if (m_Window == NULL)
	{
		ZN_LOG(LogWindow, ELogVerbosity::Error, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return; // #todo - exception / crash / invalid
	}

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(m_Window, &wmInfo);
	m_NativeHandle = (HWND) wmInfo.info.win.window;
	m_SDLWindowID = SDL_GetWindowID(m_Window);
}

Window::~Window()
{

	if (m_Window != nullptr)
	{
		SDL_DestroyWindow(m_Window);

		m_Window = nullptr;
	}
}

bool Zn::Window::ProcessEvent(SDL_Event event)
{
	if (event.window.windowID == m_SDLWindowID)
	{
		if (event.window.event == SDL_WINDOWEVENT_CLOSE)
		{
			return false;
		}

		// TODO: Implement as events, so that we remove the strong-reference to Renderer
		if (event.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			Renderer::on_window_resized();
		}
		else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
		{
			Renderer::on_window_minimized();
		}
		else if (event.window.event == SDL_WINDOWEVENT_RESTORED)
		{
			Renderer::on_window_restored();
		}
	}

	return true;
}

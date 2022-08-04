#include "Engine/Window.h"
#include "Core/Log/Log.h"
#include "Core/Log/LogMacros.h"
#include <Core/HAL/SDL/SDLWrapper.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <Rendering/D3D11/D3D11.h>
#include <ImGui/ImGuiWrapper.h>
#include <Core/Trace/Trace.h>

DEFINE_STATIC_LOG_CATEGORY(LogWindow, ELogVerbosity::Log);

using namespace Zn;

Window::Window(const int width, const int height, const String& title)
{
	_ASSERT(SDLWrapper::IsInitialized());

	// Create window at default position

	static auto ZN_SDL_WINDOW_FLAGS = (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	m_Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, ZN_SDL_WINDOW_FLAGS);

	if (m_Window == NULL)
	{
		ZN_LOG(LogWindow, ELogVerbosity::Error, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return; // #todo - exception / crash / invalid
	}

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(m_Window, &wmInfo);
	HWND hwnd = (HWND)wmInfo.info.win.window;

	m_D3DDevice = UniquePtr<D3D11Device>(D3D11Device::CreateDevice(hwnd));

	m_ImGui = std::make_unique<ImGuiWrapper>();
	m_ImGui->Initialize(m_Window, m_D3DDevice->GetDevice(), m_D3DDevice->GetDeviceContext());
}

Window::~Window()
{
	if (m_Window != nullptr)
	{
		SDL_DestroyWindow(m_Window);

		m_Window = nullptr;
	}

	if (m_D3DDevice)
	{
		m_D3DDevice->Cleanup();
		m_D3DDevice = nullptr;
	}
}

void Window::PollEvents()
{
	if (!m_HasPolledEventsThisFrame)
	{
		SDL_Event event;

		while (SDL_PollEvent(&event) != 0)
		{
			m_ImGui->ProcessEvent(event);
			if (event.type == SDL_QUIT)
			{
				m_IsRequestingExit = true;
			}
			else if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(m_Window))
			{
				if (event.window.event == SDL_WINDOWEVENT_CLOSE)
				{
					m_IsRequestingExit = true;
				}
				else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					m_D3DDevice->ResizeWindow();
				}
			}
		}

		m_HasPolledEventsThisFrame = true;
	}
	else
	{
		ZN_LOG(LogWindow, ELogVerbosity::Warning, "Window events have been polled twice this frame.");
	}
}

void Window::NewFrame()
{
	PollEvents();

	m_ImGui->NewFrame();
}

void Window::EndFrame()
{
	ZN_TRACE_QUICKSCOPE();

	m_D3DDevice->ClearRenderTarget();

	m_ImGui->EndFrame();

	m_D3DDevice->Present(false);

	m_FrameNumber++;

	m_HasPolledEventsThisFrame = false;
}

bool Window::IsRequestingExit() const
{
	return m_IsRequestingExit;
}
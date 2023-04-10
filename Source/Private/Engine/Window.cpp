#include <Znpch.h>
#include "Engine/Window.h"
#include <Core/HAL/SDL/SDLWrapper.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <Rendering/D3D11/D3D11.h>
#include <Rendering/Vulkan/VulkanDevice.h>
#include <ImGui/ImGuiWrapper.h>

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

	m_ImGui = std::make_unique<ImGuiWrapper>();
	m_ImGui->Initialize();

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(m_Window, &wmInfo);
	m_NativeHandle = (HWND) wmInfo.info.win.window;

	m_VulkanDevice = std::make_unique<VulkanDevice>();
	m_VulkanDevice->Initialize(m_Window);
}

Window::~Window()
{
	if (m_VulkanDevice != nullptr)
	{
		m_VulkanDevice->Cleanup();
		m_VulkanDevice = nullptr;
	}

	if (m_Window != nullptr)
	{
		SDL_DestroyWindow(m_Window);

		m_Window = nullptr;
	}
}

void Window::PollEvents(float delta_time)
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
					m_VulkanDevice->ResizeWindow();
				}
				else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED)
				{
					m_VulkanDevice->OnWindowMinimized();
					m_IsMinimized = true;
				}
				else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
				{
					if (m_IsMinimized)
					{
						m_IsMinimized = false;
						m_VulkanDevice->OnWindowRestored();
					}
				}
			}
			else
			{
				HandleInput(event, delta_time);
			}
		}

		m_HasPolledEventsThisFrame = true;
	}
	else
	{
		ZN_LOG(LogWindow, ELogVerbosity::Warning, "Window events have been polled twice this frame.");
	}
}

void Window::NewFrame(float delta_time)
{
	PollEvents(delta_time);

	m_ImGui->NewFrame();
}

void Window::EndFrame()
{
	ZN_TRACE_QUICKSCOPE();

	m_VulkanDevice->Draw();

	m_FrameNumber++;

	m_HasPolledEventsThisFrame = false;
}

bool Window::IsRequestingExit() const
{
	return m_IsRequestingExit;
}

void Window::HandleInput(const SDL_Event& InEvent, float delta_time)
{
	static const float kSpeed = 15.f;
	switch (InEvent.type)
	{
		case SDL_KEYDOWN:
		{
			ZN_LOG(LogWindow, ELogVerbosity::Log, "Pressed key %d", InEvent.key.keysym.scancode);

			switch (InEvent.key.keysym.scancode)
			{
				case SDL_SCANCODE_A:
				m_VulkanDevice->MoveCamera(glm::vec3(-kSpeed * delta_time, 0.f, 0.f));
				break;
				case SDL_SCANCODE_D:
				m_VulkanDevice->MoveCamera(glm::vec3(kSpeed * delta_time, 0.f, 0.f));
				break;
				case SDL_SCANCODE_S:
				m_VulkanDevice->MoveCamera(glm::vec3(0.f, 0.f, -kSpeed * delta_time));
				break;
				case SDL_SCANCODE_W:
				m_VulkanDevice->MoveCamera(glm::vec3(0.f, 0.f, kSpeed * delta_time));
				break;
				case SDL_SCANCODE_Q:
				m_VulkanDevice->MoveCamera(glm::vec3(0.f, -kSpeed * delta_time, 0.f));
				break;
				case SDL_SCANCODE_E:
				m_VulkanDevice->MoveCamera(glm::vec3(0.f, kSpeed * delta_time, 0.f));
				break;
			}
		}
		break;
		case SDL_KEYUP:
		ZN_LOG(LogWindow, ELogVerbosity::Log, "Released key %d", InEvent.key.keysym.scancode);
		break;

		case SDL_MOUSEBUTTONDOWN:
		{	
		}

		case SDL_MOUSEMOTION:
		{
			if (InEvent.motion.state & SDL_BUTTON_RMASK)
			{
				float sensitivity = 0.1f;

				glm::vec2 rotation
				{
					InEvent.motion.xrel * sensitivity,
					-InEvent.motion.yrel * sensitivity
				};

				m_VulkanDevice->RotateCamera(rotation);

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

#include <Znpch.h>
#include "Engine/Window.h"
#include <Core/HAL/SDL/SDLWrapper.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <Rendering/D3D11/D3D11.h>
#include <Rendering/Vulkan/VulkanDevice.h>
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

//void Window::HandleInput(const SDL_Event& InEvent, float delta_time)
//{
//	static const float kSpeed = 15.f;
//	switch (InEvent.type)
//	{
//		case SDL_KEYDOWN:
//		{
//			ZN_LOG(LogWindow, ELogVerbosity::Log, "Pressed key %d", InEvent.key.keysym.scancode);
//
//			switch (InEvent.key.keysym.scancode)
//			{
//				case SDL_SCANCODE_A:
//				m_VulkanDevice->MoveCamera(glm::vec3(-kSpeed * delta_time, 0.f, 0.f));
//				break;
//				case SDL_SCANCODE_D:
//				m_VulkanDevice->MoveCamera(glm::vec3(kSpeed * delta_time, 0.f, 0.f));
//				break;
//				case SDL_SCANCODE_S:
//				m_VulkanDevice->MoveCamera(glm::vec3(0.f, 0.f, -kSpeed * delta_time));
//				break;
//				case SDL_SCANCODE_W:
//				m_VulkanDevice->MoveCamera(glm::vec3(0.f, 0.f, kSpeed * delta_time));
//				break;
//				case SDL_SCANCODE_Q:
//				m_VulkanDevice->MoveCamera(glm::vec3(0.f, -kSpeed * delta_time, 0.f));
//				break;
//				case SDL_SCANCODE_E:
//				m_VulkanDevice->MoveCamera(glm::vec3(0.f, kSpeed * delta_time, 0.f));
//				break;
//			}
//		}
//		break;
//		case SDL_KEYUP:
//		ZN_LOG(LogWindow, ELogVerbosity::Log, "Released key %d", InEvent.key.keysym.scancode);
//		break;
//
//		case SDL_MOUSEBUTTONDOWN:
//		{	
//		}
//
//		case SDL_MOUSEMOTION:
//		{
//			if (InEvent.motion.state & SDL_BUTTON_RMASK)
//			{
//				float sensitivity = 0.1f;
//
//				glm::vec2 rotation
//				{
//					InEvent.motion.xrel * sensitivity,
//					-InEvent.motion.yrel * sensitivity
//				};
//
//				m_VulkanDevice->RotateCamera(rotation);
//
//				SDL_SetRelativeMouseMode(SDL_TRUE);
//			}
//			else
//			{
//				SDL_SetRelativeMouseMode(SDL_FALSE);
//			}
//			break;
//		}
//		default:
//		break;
//	}
//}

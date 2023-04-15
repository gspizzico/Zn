#include <Znpch.h>
#include <Engine/Engine.h>
#include <Core/Log/OutputDeviceManager.h>
#include <Windows/WindowsDebugOutput.h> // #TODO Move to somewhere not platform specific
#include <Core/Log/StdOutputDevice.h>
#include <Automation/AutomationTestManager.h>
#include <Core/CommandLine.h>
#include <Core/HAL/SDL/SDLWrapper.h>
#include <Core/Time/Time.h>
#include <Engine/Window.h>
#include <Editor/Editor.h>
#include <Core/IO/IO.h>
#include <SDL.h>
#include <Rendering/Renderer.h>
#include <ImGui/ImGuiWrapper.h>
#include <Input/Input.h>
#include <glm/glm.hpp>
#include <Engine/Camera.h>

DEFINE_STATIC_LOG_CATEGORY(LogEngine, ELogVerbosity::Log);

using namespace Zn;

void Engine::Start()
{
	IO::Initialize();

	OutputDeviceManager::Get().RegisterOutputDevice<WindowsDebugOutput>();

	if (CommandLine::Get().Param("-std"))
	{
		OutputDeviceManager::Get().RegisterOutputDevice<StdOutputDevice>();
	}

	SDLWrapper::Initialize();

	// Create Window

	m_Window = std::make_shared<Window>(640, 480, "Zn-Engine");

	// Initialize Renderer

	if (!Renderer::create(RendererBackendType::Vulkan))
	{
		ZN_LOG(LogEngine, ELogVerbosity::Error, "Failed to create renderer.");
		return;
	}

	if (!Renderer::initialize(Zn::RendererBackendInitData{ m_Window }))
	{
		ZN_LOG(LogEngine, ELogVerbosity::Error, "Failed to initialize renderer.");
		return;
	}

	ZN_LOG(LogEngine, ELogVerbosity::Log, "Engine initialized.");

	ZN_TRACE_INFO("Zn Engine");

	// TEMP - Moving Camera

	m_Camera = std::make_shared<Camera>();
	m_Camera->position = glm::vec3(0.f, 0.f, -10.f);
	m_Camera->direction = glm::vec3(0.0f, 0.0f, -1.f);

	while (!m_IsRequestingExit)
	{
		ZN_TRACE_QUICKSCOPE();

		double startFrame = Time::Seconds();

		if (!PumpMessages())
		{
			m_IsRequestingExit = true;
			break;
		}

		// TEMP - Moving Camera
		Renderer::set_camera(*m_Camera.get());

		Automation::AutomationTestManager::Get().Tick(m_DeltaTime);		

		auto engine_render = [](float deltaTime)
		{
			auto& editor = Editor::Get();
			editor.PreUpdate(deltaTime);
			editor.Update(deltaTime);
			editor.PostUpdate(deltaTime);
		};

		if (!Renderer::render_frame(m_DeltaTime, engine_render))
		{
			m_IsRequestingExit = true;
		}

		m_IsRequestingExit |= Editor::Get().IsRequestingExit();

		m_DeltaTime = static_cast<float>(Time::Seconds() - startFrame);

		ZN_END_FRAME();
	}
}

void Engine::Shutdown()
{
	SDLWrapper::Shutdown();
}

bool Engine::PumpMessages()
{
	bool bWantsToExit = false;
	SDL_Event event;

	while (SDL_PollEvent(&event) != 0)
	{
		imgui_process_event(event);

		if (event.type == SDL_QUIT)
		{
			bWantsToExit = true;
		}
		else if (event.type == SDL_WINDOWEVENT)
		{
			bWantsToExit |= !m_Window->ProcessEvent(event);
		}
		else if (event.type >= SDL_KEYDOWN && event.type <= SDL_CONTROLLERSENSORUPDATE)
		{
			// TODO: camera is being processed by input ...
			Input::sdl_process_input(event, m_DeltaTime, *m_Camera.get());
		}
	}

	return !bWantsToExit;
}

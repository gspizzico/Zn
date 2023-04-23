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
#include <Core/IO/IO.h>
#include <SDL.h>
#include <Rendering/Renderer.h>
#include <ImGui/ImGuiWrapper.h>
#include <glm/glm.hpp>
#include <Engine/Camera.h>
#include <Engine/EngineFrontend.h>
#include <Application/Application.h>
#include <Application/ApplicationInput.h>

DEFINE_STATIC_LOG_CATEGORY(LogEngine, ELogVerbosity::Log);

using namespace Zn;


void Engine::Initialize()
{
	// Initialize Renderer

	if (!Renderer::create(RendererBackendType::Vulkan))
	{
		ZN_LOG(LogEngine, ELogVerbosity::Error, "Failed to create renderer.");
		return;
	}

	if (!Renderer::initialize(Zn::RendererBackendInitData{ Application::Get().GetWindow()}))
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

	m_FrontEnd = std::make_shared<EngineFrontend>();
}

void Engine::Update(float deltaTime)
{
	ZN_TRACE_QUICKSCOPE();
	
	m_DeltaTime = deltaTime;

	bool wantsToExit = false;

	ProcessInput();

	// TEMP - Moving Camera
	Renderer::set_camera(*m_Camera.get());

	Automation::AutomationTestManager::Get().Tick(deltaTime);

	auto engine_render = [=](float dTime)
	{
		RenderUI(dTime);
	};

	if (!Renderer::render_frame(deltaTime, engine_render))
	{
		Application::Get().RequestExit("Error - Rendering has failed.");
	}

	if (m_FrontEnd->bIsRequestingExit)
	{
		Application::Get().RequestExit("User wants to exit.");
	}

	ZN_END_FRAME();
}

void Engine::Shutdown()
{
	m_FrontEnd = nullptr;

	Renderer::destroy();
}

void Engine::RenderUI(float deltaTime)
{
	ZN_TRACE_QUICKSCOPE();

	m_FrontEnd->DrawMainMenu();

	m_FrontEnd->DrawAutomationWindow();
}

void Engine::ProcessInput()
{
	SharedPtr<InputState> input = Application::Get().GetInputState();

	for (const SDL_Event& event : input->events)
	{
		camera_process_input(event, m_DeltaTime, *m_Camera.get());
	}
}
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


DEFINE_STATIC_LOG_CATEGORY(LogEngine, ELogVerbosity::Log);

using namespace Zn;

void Engine::Initialize()
{
	OutputDeviceManager::Get().RegisterOutputDevice<WindowsDebugOutput>();

	if (CommandLine::Get().Param("-std"))
	{
		OutputDeviceManager::Get().RegisterOutputDevice<StdOutputDevice>();
	}

	SDLWrapper::Initialize();

	// Create Window

	m_Window = std::make_unique<Window>(640, 480, "Zn-Engine");

	ZN_LOG(LogEngine, ELogVerbosity::Log, "Engine initialized.");

	ZN_TRACE_INFO("Zn Engine");
}

void Engine::Start()
{
	//Automation::AutomationTestManager::Get().ExecuteStartupTests();

	while (!m_IsRequestingExit)
	{
		ZN_TRACE_QUICKSCOPE();

		double startFrame = Time::Seconds();

		m_Window->NewFrame();

		Editor& editor = Editor::Get();

		// Pre Update Work

		editor.PreUpdate(m_DeltaTime);

		// Update Work

		editor.Update(m_DeltaTime);

		// Post Update Work

		editor.PostUpdate(m_DeltaTime);

		Automation::AutomationTestManager::Get().Tick(m_DeltaTime);

		m_IsRequestingExit = m_Window->IsRequestingExit() || editor.IsRequestingExit();

		// Render

		m_Window->EndFrame();

		m_DeltaTime = static_cast<float>(Time::Seconds() - startFrame);

		ZN_END_FRAME();
	}
}

void Engine::Shutdown()
{
	SDLWrapper::Shutdown();
}

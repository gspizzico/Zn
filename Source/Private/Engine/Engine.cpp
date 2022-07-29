#include <Engine/Engine.h>
#include <Core/Log/OutputDeviceManager.h>
#include <Windows/WindowsDebugOutput.h> // #TODO Move to somewhere not platform specific
#include <Automation/AutomationTestManager.h>
#include <Core/HAL/SDL/SDLWrapper.h>
#include <Core/Time/Time.h>
#include <Core/Log/Log.h>
#include <Core/Log/LogMacros.h>
#include <Engine/Window.h>
#include <Editor/Editor.h>


DEFINE_STATIC_LOG_CATEGORY(LogEngine, ELogVerbosity::Log);

using namespace Zn;

void Engine::Initialize()
{
	OutputDeviceManager::Get().RegisterOutputDevice<WindowsDebugOutput>();

	SDLWrapper::Initialize();

	// Create Window

	m_Window = std::make_unique<Window>(640, 480, "Zn-Engine");

	ZN_LOG(LogEngine, ELogVerbosity::Log, "Engine initialized.");
}

void Engine::Start()
{
	//Automation::AutomationTestManager::Get().ExecuteStartupTests();

	while (!m_IsRequestingExit)
	{
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
	}
}

void Engine::Shutdown()
{	
	SDLWrapper::Shutdown();
}

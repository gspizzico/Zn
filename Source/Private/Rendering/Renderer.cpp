#include "Znpch.h"
#include "Rendering/Renderer.h"
#include "Rendering/RendererBackend.h"
#include "ImGui/ImGuiWrapper.h"

#include "Rendering/Vulkan/VulkanBackend.h"
#include <Engine/Camera.h>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogRenderer, ELogVerbosity::Log);

static RendererBackend* GRenderer = nullptr;
static bool GInitialized = false;

bool Zn::Renderer::create(RendererBackendType type)
{
	_ASSERT(GRenderer == nullptr);

	switch (type)
	{
		case RendererBackendType::Vulkan:
			GRenderer = new VulkanBackend();
		break;
		case RendererBackendType::DX12:
			// TODO: Implement DX12
			return false;
	}

	return true;
}

bool Zn::Renderer::initialize(RendererBackendInitData data)
{
	_ASSERT(GRenderer && GInitialized == false);

	Zn::imgui_initialize();

	if (!GRenderer->initialize(data))
	{
		Zn::imgui_shutdown();

		return false;
	}

	GInitialized = true;

	return true;
}

void Zn::Renderer::destroy()
{
	if (GInitialized)
	{
		GRenderer->shutdown();

		Zn::imgui_shutdown();

		delete GRenderer;

		GRenderer = nullptr;
	}

	GInitialized = false;
}


bool Zn::Renderer::render_frame(float deltaTime, std::function<void(float)> render)
{
	_ASSERT(GRenderer);

	if (!begin_frame())
	{
		ZN_LOG(LogRenderer, ELogVerbosity::Error, "Failed to begin_frame.");
		return false;
	}

	if (render)
	{
		render(deltaTime);
	}

	if (!GRenderer->render_frame())
	{
		ZN_LOG(LogRenderer, ELogVerbosity::Error, "Failed to render_frame.");
	}

	if (!end_frame())
	{
		ZN_LOG(LogRenderer, ELogVerbosity::Error, "Failed to end_frame.");
	}

	return true;
}

void Zn::Renderer::on_window_resized()
{
	_ASSERT(GRenderer);

	GRenderer->on_window_resized();
}

void Zn::Renderer::on_window_minimized()
{
	_ASSERT(GRenderer);

	GRenderer->on_window_minimized();
}

void Zn::Renderer::on_window_restored()
{
	_ASSERT(GRenderer);

	GRenderer->on_window_restored();
}

void Zn::Renderer::set_camera(Camera camera)
{
	_ASSERT(GRenderer);

	GRenderer->set_camera(camera.position, camera.direction);
}

bool Zn::Renderer::begin_frame()
{
	_ASSERT(GInitialized);

	Zn::imgui_begin_frame();

	GRenderer->begin_frame();

	return true;
}

bool Zn::Renderer::end_frame()
{
	_ASSERT(GInitialized);

	Zn::imgui_end_frame();

	GRenderer->end_frame();

	return true;
}
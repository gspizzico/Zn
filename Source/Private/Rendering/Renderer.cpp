#include "Znpch.h"
#include "Rendering/Renderer.h"
#include "Rendering/RendererBackend.h"
#include "ImGui/ImGuiWrapper.h"

#include "Rendering/Vulkan/VulkanBackend.h"
#include <Engine/Camera.h>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogRenderer, ELogVerbosity::Log);

static RendererBackend* GRenderer = nullptr;

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
	_ASSERT(GRenderer);

	Zn::imgui_initialize();

	if (!GRenderer->initialize(data))
	{
		Zn::imgui_shutdown();

		return false;
	}

	return true;
}

void Zn::Renderer::destroy()
{
	_ASSERT(GRenderer);

	Zn::imgui_shutdown();

	GRenderer->shutdown();

	delete GRenderer;

	GRenderer = nullptr;
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
	Zn::imgui_begin_frame();

	return true;
}

bool Zn::Renderer::end_frame()
{
	Zn::imgui_end_frame();

	return true;
}
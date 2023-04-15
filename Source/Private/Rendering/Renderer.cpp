#include "Znpch.h"
#include "Rendering/Renderer.h"
#include "Rendering/RendererBackend.h"
#include "ImGui/ImGuiWrapper.h"

#include "Rendering/Vulkan/VulkanBackend.h"
#include <Engine/Camera.h>

using namespace Zn;

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

bool Zn::Renderer::begin_frame()
{
	_ASSERT(GRenderer);

	Zn::imgui_begin_frame();

	return true;
}

bool Zn::Renderer::render_frame()
{
	_ASSERT(GRenderer);

	return GRenderer->render_frame();
}

bool Zn::Renderer::end_frame()
{
	_ASSERT(GRenderer);

	Zn::imgui_end_frame();

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

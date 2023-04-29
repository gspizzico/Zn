#include "Znpch.h"
#include "Rendering/Renderer.h"
#include "ImGui/ImGuiWrapper.h"

#include "Rendering/Vulkan/VulkanRenderer.h"
#include <Engine/Camera.h>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogRenderer, ELogVerbosity::Log);

UniquePtr<Renderer> Zn::Renderer::instance;

Renderer& Zn::Renderer::get()
{
	if (!instance)
	{
		throw std::runtime_error("Renderer not initialized.");
	}

	return *instance;
}

bool Zn::Renderer::initialize(RendererBackendType type, RendererInitParams data)
{
	_ASSERT(!instance);

	switch (type)
	{
		case RendererBackendType::Vulkan:
		instance.reset(new VulkanRenderer());
		break;
		case RendererBackendType::DX12:
		// TODO: Implement DX12
		return false;
	}

	Zn::imgui_initialize();

	if (!instance->initialize(data))
	{
		Zn::imgui_shutdown();

		return false;
	}

	return true;
}

bool Zn::Renderer::destroy()
{
	if (instance)
	{
		instance->shutdown();
		
		instance = nullptr;

		return true;
	}

	return false;
}

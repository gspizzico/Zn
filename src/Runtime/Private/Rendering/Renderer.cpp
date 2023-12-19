#include "Rendering/Renderer.h"

#include "Rendering/Vulkan/VulkanRenderer.h"
// #include <Engine/Camera.h>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogRenderer, ELogVerbosity::Log);

UniquePtr<Renderer> Zn::Renderer::instance;

Renderer& Zn::Renderer::Get()
{
    if (!instance)
    {
        throw std::runtime_error("Renderer not initialized.");
    }

    return *instance;
}

bool Zn::Renderer::initialize(RendererBackendType type, RendererInitParams data)
{
    ZN_TRACE_QUICKSCOPE();

    check(!instance);

    switch (type)
    {
    case RendererBackendType::Vulkan:
        instance.reset(new VulkanRenderer());
        break;
    case RendererBackendType::DX12:
        // TODO: Implement DX12
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

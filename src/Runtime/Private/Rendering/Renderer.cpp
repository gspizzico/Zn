#include "Rendering/Renderer.h"

#include "Rendering/Vulkan/VulkanRenderer.h"
#include <Application/AppEvents.h>
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

bool Zn::Renderer::Create(RendererBackendType type)
{
    ZN_TRACE_QUICKSCOPE();

    check(!instance);

    switch (type)
    {
    case RendererBackendType::Vulkan:
        instance.reset(new VulkanRenderer());
        break;
        return false;
    }

    AppEvents::OnWindowSizeChanged = cpp::bind<&Renderer::OnWindowResized>(instance.get());
    AppEvents::OnWindowMinimized   = cpp::bind<&Renderer::OnWindowMinimized>(instance.get());
    AppEvents::OnWindowRestored    = cpp::bind<&Renderer::OnWindowRestored>(instance.get());

    instance->Initialize();

    return true;
}

bool Zn::Renderer::Destroy()
{
    if (instance)
    {
        instance->Shutdown();

        AppEvents::OnWindowSizeChanged.reset();
        AppEvents::OnWindowMinimized.reset();
        AppEvents::OnWindowRestored.reset();

        instance = nullptr;

        return true;
    }

    return false;
}

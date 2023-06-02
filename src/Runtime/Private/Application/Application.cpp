#include <Application/Application.h>
#include <Core/CoreAssert.h>
#if PLATFORM_WINDOWS
    #include <Application/Platforms/Windows/SDLApplication.h>
#endif

using namespace Zn;

namespace
{
Application* GApplication = nullptr;
}

Zn::Application& Zn::Application::Get()
{
    checkMsg(GApplication, "Trying to get Application before it's initialized!");
    return *GApplication;
}

void Zn::Application::Create()
{
    check(GApplication == nullptr);

#if PLATFORM_WINDOWS
    GApplication = new SDLApplication();
#endif

    GApplication->Initialize();
}

void Zn::Application::Destroy()
{
    check(GApplication);

    GApplication->Shutdown();

    delete GApplication;
}

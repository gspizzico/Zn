#include <Application.h>
#include <CoreAssert.h>

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

void Zn::Application::SetApplication(Application* application_)
{
    check(GApplication == nullptr || application_ == nullptr);

    if (GApplication)
    {
        delete GApplication;
    }

    GApplication = application_;
}

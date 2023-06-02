#include <Core/CommandLine.h>
#include <Application/Application.h>

#include <Engine/Engine.h>

using namespace Zn;

int main(int argc_, char* args_[])
{
    CommandLine::Get().Initialize(argc_, args_);

    Platform_InitializeApplication();
    int32 errorCode = Engine::Launch();
    Platform_ShutdownApplication();
    return errorCode;
}

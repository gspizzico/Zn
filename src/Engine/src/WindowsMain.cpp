#include <Application.h>

#include <Engine.h>

using namespace Zn;

int main(int argc_, char* args_[])
{
    Platform_InitializeApplication();
    int32 errorCode = Engine::Launch();
    Platform_ShutdownApplication();
    return errorCode;
}

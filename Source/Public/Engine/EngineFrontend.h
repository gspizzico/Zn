#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Core/Name.h>
#include <Core/Containers/Map.h>

namespace Zn
{
class EngineFrontend
{
  public:
    EngineFrontend() = default;

    void DrawMainMenu();
    void DrawAutomationWindow();

    bool bAutomationWindow = false;
    bool bIsRequestingExit = false;

    Map<Name, bool> SelectedTests;

    glm::vec3 light     = glm::vec3(0, 4, 0);
    float     intensity = 80;
    float     distance  = 20;
};

} // namespace Zn

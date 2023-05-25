#pragma once

#include <Core/HAL/BasicTypes.h>

namespace Zn
{
struct RendererInitParams
{
    SharedPtr<class Window> window;
};

struct ViewInfo
{
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;
    f32       fov; // in degrees
    f32       aspectRatio;
    f32       nearClip;
    f32       farClip;
};
} // namespace Zn

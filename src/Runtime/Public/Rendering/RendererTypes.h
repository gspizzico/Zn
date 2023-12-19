#pragma once

#include <Core/CoreTypes.h>
#include <glm/vec3.hpp>

namespace Zn
{
struct ViewInfo
{
    ViewInfo() = default;

    glm::vec3 position {0.f};
    glm::vec3 forward {1.0f, 0.0f, 0.0f};
    glm::vec3 up {0.0f, 1.f, 0.f};
    glm::vec3 right {0.0f, 0.0f, 1.0f};
    f32       fov         = 60.f; // in degrees
    f32       aspectRatio = 16.f / 9.f;
    f32       nearClip    = 0.10f;
    f32       farClip     = 10000.f;
};
} // namespace Zn

#pragma once

#include <Core/HAL/BasicTypes.h>

namespace Zn
{
struct RHIVertex
{
    glm::vec3 position {0.f};
    glm::vec3 normal {0.f};
    glm::vec3 color {0.f};
    glm::vec2 uv {0.f};
};
} // namespace Zn
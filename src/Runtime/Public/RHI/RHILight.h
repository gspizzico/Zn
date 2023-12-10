#pragma once

#include <glm/vec4.hpp>

namespace Zn
{
struct RHILight
{
    glm::vec4 position;
    float     intensity;
    float     range;
    float     unused0;
    float     unused1;
};
} // namespace Zn

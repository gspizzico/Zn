#pragma once

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace Zn
{
struct RHIView
{
    glm::vec4 position;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
};
} // namespace Zn

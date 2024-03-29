#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Rendering/RHI/RHI.h>

namespace Zn
{
struct RHIInputLayout
{
    Vector<vk::VertexInputBindingDescription>   bindings;
    Vector<vk::VertexInputAttributeDescription> attributes;
    vk::PipelineVertexInputStateCreateFlags     flags {0};
};
} // namespace Zn
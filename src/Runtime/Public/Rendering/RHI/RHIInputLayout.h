#pragma once

#include <Core/CoreTypes.h>
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

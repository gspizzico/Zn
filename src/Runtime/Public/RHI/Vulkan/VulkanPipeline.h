#pragma once

#include <RHI/RHIPipeline.h>
#include <RHI/Vulkan/Vulkan.h>

namespace Zn::RHI
{
inline vk::PolygonMode TranslatePolygonMode(RHI::PolygonMode polygonMode_)
{
    switch (polygonMode_)
    {
    case RHI::PolygonMode::Fill:
        return vk::PolygonMode::eFill;
    case RHI::PolygonMode::Line:
        return vk::PolygonMode::eLine;
    case RHI::PolygonMode::Point:
        return vk::PolygonMode::ePoint;
    }

    check(false);

    return vk::PolygonMode::eFill;
}

inline vk::CullModeFlags TranslateCullMode(RHI::CullMode cullMode_)
{
    return (vk::CullModeFlags)((uint32) cullMode_);
}

} // namespace Zn::RHI

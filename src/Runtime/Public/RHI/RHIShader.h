#pragma once

#include <RHI/RHI.h>
#include <RHI/RHIResource.h>

namespace Zn::RHI
{
struct Shader
{
    ShaderStage        stage;
    ShaderModuleHandle module;
    cstring            entryPoint;
};
} // namespace Zn::RHI

#include <Engine/Engine.h>
#include <Application/Application.h>
#include <Core/Time/Time.h>
#include <Core/CoreAssert.h>

#include <Application/AppEventHandler.h>
#include <Engine/Application/AppEventHandlerImpl.h>
#include <RHI/RHIDevice.h>
#include <RHI/RHIDescriptor.h>
#include <RHI/RHIView.h>
#include <RHI/RHILight.h>

using namespace Zn;

DEFINE_STATIC_LOG_CATEGORY(LogEngine, ELogVerbosity::Log);

namespace
{
float               GDeltaTime         = 0.f;
constexpr uint32    kMaxFramesInFlight = 2;
UBOHandle           CameraBufferHandles[kMaxFramesInFlight];
UBOHandle           LightingBufferHandles[kMaxFramesInFlight];
DescriptorSetHandle GlobalDescriptorSets[kMaxFramesInFlight];
} // namespace

void InitializeRenderer()
{
    using namespace RHI;

    RHIDevice& device = RHIDevice::Get();

    // Create global descriptor pool
    DescriptorPoolHandle GlobalDescriptorPoolHandle = device.CreateDescriptorPool(DescriptorPoolDescription {
        .desc    = {{DescriptorType::CombinedImageSampler, 1000}, {DescriptorType::UniformBuffer, 1000}},
        .flags   = DescriptorPoolCreateFlag::None,
        .maxSets = 1000,
    });

    // Create global descriptor set layout
    // 0 -> Camera
    // 1 -> Lighting
    DescriptorSetLayoutHandle GlobalDescriptorSetLayoutHandle = device.CreateDescriptorSetLayout(
        DescriptorSetLayoutDescription {.bindings = {{
                                                         .binding         = 0,
                                                         .descriptorType  = DescriptorType::UniformBuffer,
                                                         .descriptorCount = 1,
                                                         .shaderStages    = ShaderStage::Vertex | ShaderStage::Fragment,
                                                     },
                                                     {
                                                         .binding         = 1,
                                                         .descriptorType  = DescriptorType::UniformBuffer,
                                                         .descriptorCount = 1,
                                                         .shaderStages    = ShaderStage::Fragment,
                                                     }}});

    // Allocate descriptor sets in global layout

    for (uint32 index = 0; index < kMaxFramesInFlight; ++index)
    {
        CameraBufferHandles[index]   = device.CreateUniformBuffer(sizeof(RHIView), RHIResourceUsage::CpuToGpu);
        LightingBufferHandles[index] = device.CreateUniformBuffer(sizeof(RHILight), RHIResourceUsage::CpuToGpu);

        Vector<DescriptorSetHandle> handles =
            device.AllocateDescriptorSets(DescriptorSetAllocationDescription {.descriptorPool       = GlobalDescriptorPoolHandle,
                                                                              .descriptorSetLayouts = {
                                                                                  GlobalDescriptorSetLayoutHandle,
                                                                              }});

        check(handles.size() == 1);

        GlobalDescriptorSets[index] = handles[0];

        device.UpdateDescriptorSets({{.descriptorSet        = GlobalDescriptorSets[index],
                                      .binding              = 0,
                                      .arrayElement         = 0,
                                      .descriptorType       = DescriptorType::UniformBuffer,
                                      .descriptorBufferInfo = {{
                                          .ubo    = CameraBufferHandles[index],
                                          .offset = 0,
                                          .range  = sizeof(RHIView),
                                      }}},
                                     {.descriptorSet        = GlobalDescriptorSets[index],
                                      .binding              = 1,
                                      .arrayElement         = 0,
                                      .descriptorType       = DescriptorType::UniformBuffer,
                                      .descriptorBufferInfo = {{
                                          .ubo    = LightingBufferHandles[index],
                                          .offset = 0,
                                          .range  = sizeof(RHILight),
                                      }}}});
    }

    ZN_LOG(LogEngine, ELogVerbosity::Log, "Renderer initialized.");
}

void Zn::Engine::Create()
{
    AppEventHandler::SetEventHandler(new AppEventHandlerImpl());

    InitializeRenderer();

    ZN_LOG(LogEngine, ELogVerbosity::Log, "Engine initialized.");
}

void Zn::Engine::Destroy()
{
}

void Zn::Engine::Tick(float deltaTime_)
{
}

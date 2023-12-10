#pragma once

#include <Core/CoreTypes.h>
#include <RHI/RHIResource.h>
#include <RHI/RHITexture.h>
#include <RHI/RHIRenderPass.h>
#include <RHI/RHIDescriptor.h>
#include <RHI/RHIPipeline.h>

namespace Zn
{
class RHIDevice
{
  public:
    static void       Create();
    static void       Destroy();
    static RHIDevice& Get();

    TextureHandle               CreateTexture(const RHITextureDescriptor& descriptor_);
    RenderPassHandle            CreateRenderPass(const RHIRenderPassDescription& description_);
    DescriptorPoolHandle        CreateDescriptorPool(const RHI::DescriptorPoolDescription& description_);
    DescriptorSetLayoutHandle   CreateDescriptorSetLayout(const RHI::DescriptorSetLayoutDescription& description_);
    ShaderModuleHandle          CreateShaderModule(Span<const uint8> shaderBytes_);
    PipelineHandle              CreatePipeline(const RHI::PipelineDescription& description_);
    UBOHandle                   CreateUniformBuffer(uint32 size_, RHIResourceUsage memoryUsage_);
    void                        DestroyUniformBuffer(UBOHandle handle_);
    Vector<DescriptorSetHandle> AllocateDescriptorSets(const RHI::DescriptorSetAllocationDescription& description_);
    void                        UpdateDescriptorSets(Span<const RHI::DescriptorSetUpdateDescription> description_);

  private:
    RHIDevice();
    ~RHIDevice();
    void CreateSwapChain();
    void CleanupSwapChain();
};
} // namespace Zn

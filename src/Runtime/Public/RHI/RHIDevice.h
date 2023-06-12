#pragma once

#include <Core/CoreTypes.h>
#include <RHI/RHIResource.h>
#include <RHI/RHITexture.h>
#include <RHI/RHIRenderPass.h>

namespace Zn
{
class RHIDevice
{
  public:
    static void       Create();
    static void       Destroy();
    static RHIDevice& Get();

    TextureHandle    CreateTexture(const RHITextureDescriptor& descriptor_);
    RenderPassHandle CreateRenderPass(const RHIRenderPassDescription& description_);

  private:
    RHIDevice();
    ~RHIDevice();
    void CreateSwapChain();
    void CleanupSwapChain();
};
} // namespace Zn

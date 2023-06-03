#pragma once

#include <Core/CoreTypes.h>
#include <RHI/RHIResource.h>
#include <RHI/RHITexture.h>

namespace Zn
{
class RHIDevice
{
  public:
    static void       Create();
    static void       Destroy();
    static RHIDevice& Get();

    TextureHandle CreateTexture(const RHITextureDescriptor& descriptor_);

  private:
    RHIDevice();
    ~RHIDevice();
    void CreateSwapChain();
    void CleanupSwapChain();

    void CreateFrameBuffers();
    void CleanupFrameBuffers();
};
} // namespace Zn

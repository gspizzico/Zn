#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Core/Containers/Map.h>

namespace Zn
{
struct Material;

class VulkanMaterialManager
{
  public:
    static VulkanMaterialManager& Get();

    Material* CreateMaterial(const String& name);
    Material* GetMaterial(const String& name) const;

  private:
    UnorderedMap<String, UniquePtr<Material>> materials;
};
} // namespace Zn

#pragma once

#include <Core/CoreTypes.h>
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
    Map<String, UniquePtr<Material>> materials;
};
} // namespace Zn

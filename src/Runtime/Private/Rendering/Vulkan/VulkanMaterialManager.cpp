#include <Rendering/Vulkan/VulkanMaterialManager.h>
#include <Rendering/Material.h>
#include <Rendering/RHI/RHI.h>

DEFINE_STATIC_LOG_CATEGORY(LogVulkanMaterialManager, ELogVerbosity::Log);

using namespace Zn;

Zn::VulkanMaterialManager& Zn::VulkanMaterialManager::Get()
{
    static VulkanMaterialManager instance;
    return instance;
}

Zn::Material* Zn::VulkanMaterialManager::CreateMaterial(const String& name)
{
    Material* foundMaterial = GetMaterial(name);

    if (foundMaterial == nullptr)
    {
        UniquePtr<Material> material = std::make_unique<Material>();

        foundMaterial = material.get();

        materials[name] = std::move(material);

        ZN_LOG(LogVulkanMaterialManager, ELogVerbosity::Verbose, "Vk::Material %s created.", name.c_str());
    }
    else
    {
        ZN_LOG(LogVulkanMaterialManager, ELogVerbosity::Warning, "Vk::Material %s already exists. Returning existing value.", name.c_str());
    }

    return foundMaterial;
}

Zn::Material* Zn::VulkanMaterialManager::GetMaterial(const String& name) const
{
    if (auto pMaterial = materials.find(name); pMaterial != materials.end())
    {
        return (*pMaterial).second.get();
    }

    return nullptr;
}

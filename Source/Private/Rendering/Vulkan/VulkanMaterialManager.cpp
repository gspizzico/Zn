#include <Znpch.h>
#include <Rendering/Vulkan/VulkanMaterialManager.h>
#include <Rendering/Vulkan/VulkanTypes.h>

DEFINE_STATIC_LOG_CATEGORY(LogVulkanMaterialManager, ELogVerbosity::Log);

using namespace Zn;

Zn::VulkanMaterialManager& Zn::VulkanMaterialManager::get()
{
	static VulkanMaterialManager instance;
	return instance;
}

Zn::Vk::Material* Zn::VulkanMaterialManager::create_material(const String& name)
{
	Vk::Material* foundMaterial = get_material(name);

	if (foundMaterial == nullptr)
	{
		UniquePtr<Vk::Material> material = std::make_unique<Vk::Material>();

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

Zn::Vk::Material* Zn::VulkanMaterialManager::get_material(const String& name) const
{
	if (auto pMaterial = materials.find(name); pMaterial != materials.end())
	{
		return (*pMaterial).second.get();
	}

	return nullptr;
}

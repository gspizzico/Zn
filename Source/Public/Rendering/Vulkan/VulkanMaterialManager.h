#pragma once

#include <Core/HAL/BasicTypes.h>
#include <Core/Containers/Map.h>

namespace Zn
{
	namespace Vk
	{
		struct Material;
	}

	class VulkanMaterialManager
	{
	public:

		static VulkanMaterialManager& Get();

		Vk::Material* CreateMaterial(const String& name);
		Vk::Material* GetMaterial(const String& name) const;

	private:

		UnorderedMap<String, UniquePtr<Vk::Material>> materials;
	};
}

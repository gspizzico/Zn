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

		static VulkanMaterialManager& get();

		Vk::Material* create_material(const String& name);
		Vk::Material* get_material(const String& name) const;

	private:

		UnorderedMap<String, UniquePtr<Vk::Material>> materials;
	};
}

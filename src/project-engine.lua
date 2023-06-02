-- Files
configuration "*"
files {
  "./Runtime/*/Engine/**",
  "./ThirdParty/vk_mem_alloc/**"
}

local vulkan_sdk_dir = os.getenv("VULKAN_SDK")

-- Includes
configuration "*"
includedirs {
  "./Runtime/Public/",
  "./ThirdParty/vk_mem_alloc/",
  vulkan_sdk_dir .. "/Include",
}

libdirs {
	vulkan_sdk_dir .. "/Lib",
}

links {"vulkan-1"}
-- Files
configuration "*"
files {
  "./Runtime/Public/RHI/*.h",
  "./Runtime/Public/RHI/*.hpp",
  "./Runtime/Public/RHI/*.inl",
  "./Runtime/Private/RHI/*.cpp",
  "./Runtime/*/RHI/Vulkan/**",
  "./ThirdParty/vk_mem_alloc/**"
}

configuration "win-*"
files {
  "./Runtime/*/PlatformRHI/Windows/Vulkan/**"
}

local vulkan_sdk_dir = os.getenv("VULKAN_SDK")

-- Includes
configuration "*"
includedirs {
  "/Runtime/Public/",
  "./ThirdParty/vk_mem_alloc/",
  vulkan_sdk_dir .. "/Include",
}

configuration "win-*"
includedirs{
  "./Runtime/Public/PlatformRHI/"
}

libdirs {
	vulkan_sdk_dir .. "/Lib",
}

links {"vulkan-1"}
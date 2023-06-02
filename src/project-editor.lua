-- Files
configuration "*"
files {
  "./Editor/**",
  "./ThirdParty/imgui/*",
  "./ThirdParty/imgui/backends/*sdl.**",
  "./ThirdParty/imgui/backends/*vulkan.**",
}

defines {
  "WITH_IMGUI"
}

-- Includes
configuration "*"
includedirs {
  "./Editor/Public/",
  "./ThirdParty/",
  "./ThirdParty/imgui/",
  "./ThirdParty/imgui/backends/",
}
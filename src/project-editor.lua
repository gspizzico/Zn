-- Editor is Windows only for now.
-- Files
configuration "win-*"
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
configuration "win-*"
includedirs {
  "./Editor/Public/",
  "./ThirdParty/",
  "./ThirdParty/imgui/",
  "./ThirdParty/imgui/backends/",
}
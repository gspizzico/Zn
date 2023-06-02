-- Files
configuration "*"
files {
  "./Runtime/*/Application/**",
  "./ThirdParty/imgui/*",
  "./ThirdParty/imgui/backends/*sdl.**",
}

-- Defines
defines {
  "WITH_IMGUI=1"
}

configuration "win-*"
files {
  "./Runtime/*/Application/Platforms/Windows/**",
}

-- Includes
configuration "*"
includedirs {
  "./Runtime/Public/Application/",   
  "./ThirdParty/",
  "./ThirdParty/sdl/",
  "./ThirdParty/imgui/",
  "./ThirdParty/imgui/backends/",
}

configuration "win-*"  
includedirs {
  "./Runtime/Public/Application/Platforms/",
}

-- Links
configuration "win-*"  

links {"SDL2"}
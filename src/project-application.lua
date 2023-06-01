-- Files
configuration "*"
files {
  "./Application/**",
  "./ThirdParty/imgui/*",
  "./ThirdParty/imgui/backends/*sdl.**",
}

-- Defines
defines {
  "WITH_IMGUI=1"
}

vpaths{
  ["Headers/Application/*"] = "Application/include/**",
  ["Source/Application/*"] = "Application/src/**",
}

configuration "win-*"
files {
  "./Application/include/Platforms/Windows/**",
  "./Application/src/Platforms/Windows/**",
}

-- Includes
configuration "*"
includedirs {
  "./Application/include/",   
  "./ThirdParty/",
  "./ThirdParty/sdl/",
  "./ThirdParty/imgui/",
  "./ThirdParty/imgui/backends/",
}

configuration "win-*"  
includedirs {
  "./Application/include/Platforms/",
}

-- Links
configuration "win-*"  

links {"SDL2"}
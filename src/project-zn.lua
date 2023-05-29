project("zn")

dofile("project-core.lua")

kind "ConsoleApp"

-- Files
configuration "*"
files {
  "./Application/**",
  "./ThirdParty/wyhash/*",
  "./ThirdParty/delegate/*",
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
}

configuration "win-*"  
includedirs {
  "./Application/include/Platforms/",
}

-- Links
configuration "win-*"  

links {"SDL2"}
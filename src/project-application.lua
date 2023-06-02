-- Files
configuration "*"
files {
  "./Runtime/*/Application/**",
}

configuration "win-*"
files {
  "./Runtime/*/Application/Platforms/Windows/**",
}

-- Includes
configuration "*"
includedirs {
  "./Runtime/Public/",
  "./ThirdParty/",
  "./ThirdParty/sdl/",
}

configuration "win-*"  
includedirs {
  "./Runtime/Public/Application/Platforms/",
}

-- Links
configuration "win-*"  

links {"SDL2"}
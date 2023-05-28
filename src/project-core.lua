project("core")

links {
  "kernel32",
  "user32",
  "gdi32",
  "rpcrt4",
}

-- solution "ZnGenie"
-- location = os.getcwd()

kind "StaticLib"

-- pchheader "Corepch.h"
-- pchsource "src/Core/Corepch.cpp"

-- Files
configuration "*"
files {
  "./Core/**",  
  "./ThirdParty/wyhash/*",
  "./ThirdParty/delegate/*",
}

vpaths{
  ["Headers/Core/*"] = "Core/include/**",
  ["Source/Core/*"] = "Core/src/**",
}

configuration "win-*"
files {
  "./Core/include/Platforms/Windows/**",
  "./Core/src/Platforms/Windows/**",
  "./ThirdParty/mimalloc/**"
}

-- Includes
configuration "*"
includedirs {
  "./Core/include/",   
  "./ThirdParty/",
}

configuration "win-*"  
includedirs {
  "./Core/include/Platforms/",
  "./ThirdParty/mimalloc/"
}

-- Links
configuration "win-*"  
links {"mimalloc-static.lib"}
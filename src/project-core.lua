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
  "./include/Core/**",
  "./src/Core/**",
  "./ThirdParty/wyhash/*",
  "./ThirdParty/delegate/*",
}

configuration "win-*"
files {
  "./include/Core/Platforms/Windows/**",
  "./src/Core/Platforms/Windows/**",
  "./ThirdParty/mimalloc/**"
}

-- Includes
configuration "*"
includedirs {
  "./include/Core/",    
  "./ThirdParty/",
}

configuration "win-*"  
includedirs {
  "./include/Core/Platforms/",
  "./ThirdParty/mimalloc/"
}

-- Links
configuration "win-*"  
links {"mimalloc-static.lib"}
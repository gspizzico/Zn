-- Files
configuration "*"
files {
  "./Core/**",  
  "./ThirdParty/wyhash/*",
  "./ThirdParty/delegate/*",
  "./ThirdParty/tracy/TracyClient.cpp"
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
links {"mimalloc-static"}
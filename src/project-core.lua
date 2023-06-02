-- Files
configuration "*"
files {
  "./Runtime/*/Core/**",  
  "./ThirdParty/wyhash/*",
  "./ThirdParty/delegate/*",
  "./ThirdParty/tracy/TracyClient.cpp"
}

configuration "win-*"
files {
  "./Runtime/*/Core/Platforms/Windows/**",
  "./ThirdParty/mimalloc/**"
}

-- Includes
configuration "*"
includedirs {
  "./Runtime/Public/Core/",   
  "./ThirdParty/",
}

configuration "win-*"  
includedirs {
  "./Runtime/Public/Core/Platforms/",
  "./ThirdParty/mimalloc/"
}

-- Links
configuration "win-*"
links {"mimalloc-static"}
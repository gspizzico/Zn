-- Files
configuration "*"
files {
  "./Runtime/*/Core/**",  
  "./ThirdParty/wyhash/*",
  "./ThirdParty/delegate/*",
  "./ThirdParty/ankerl_map/*",
  "./ThirdParty/tracy/TracyClient.cpp",
  "./ThirdParty/glm/**"
}

-- Includes
configuration "*"
includedirs {
  "./Runtime/Public/",
  "./ThirdParty/",
}
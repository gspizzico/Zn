-- Files
configuration "*"
files {
  "./Runtime/*/Core/**",  
  "./ThirdParty/wyhash/*",
  "./ThirdParty/delegate/*",
  "./ThirdParty/tracy/TracyClient.cpp"
}

-- Includes
configuration "*"
includedirs {
  "./Runtime/Public/",
  "./ThirdParty/",
}
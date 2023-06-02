-- Files
configuration "win-*"
files {
  "./Platforms/Public/Windows/**",
  "./Platforms/Private/Windows/**",
  "./ThirdParty/mimalloc/**",
}

defines{
  "PLATFORM_WINDOWS=1",
}

-- Includes
includedirs {
  "./Platforms/Public/",
  "./ThirdParty/sdl/",
  "./ThirdParty/mimalloc/"
}

links {"SDL2", "mimalloc-static"}
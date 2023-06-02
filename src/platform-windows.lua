-- Files
configuration "win-*"
files {
  "./Runtime/*/Platform/Windows/**",
  "./ThirdParty/mimalloc/**",
}

defines{
  "PLATFORM_WINDOWS=1",
}

-- Includes
includedirs {
  "./Runtime/Public/Platform/",
  "./ThirdParty/sdl/",
  "./ThirdParty/mimalloc/"
}

links {"SDL2", "mimalloc-static"}
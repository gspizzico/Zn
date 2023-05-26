--dofile("debug.lua")

solution("ZnGenie")
  
  language "C++"
  platforms {
    "x64"
  }
  flags {
    "Cpp20",
    "NoExceptions",
    "NoRTTI",
    "Symbols"
  }

  configuration "*-debug"
  libdirs {
    "../libs/Debug/**"
  }
  defines {
    "_DEBUG",
    "ZN_DEBUG",
    "_CONSOLE"
  }  
  flags {
    "FullSymbols"
  }
  
  configuration "*-release"
  libdirs {
    "../libs/Release/**"
  }
  defines {
    "NDEBUG",
    "ZN_RELEASE",
    "_CONSOLE"
  }
  flags {
    "Optimize"
  }

  configuration "win-*"
  defines {
    "PLATFORM_WINDOWS",
    "NOMINMAX"
  }

  links {
    "kernel32",
    "user32",
    "gdi32",
    "rpcrt4",
  }

  configurations {
    "win-debug", 
    "win-release",
  }

  configuration "*-debug"
    targetsuffix "-debug"
  
  configuration "*-release"
    targetsuffix "-release"

  targetsubdir "./bin/"


project("core")

  solution "ZnGenie"
  location = os.getcwd()

  kind "StaticLib"

  configuration "*"

  pchheader "Corepch.h"
  pchsource "./Core/Corepch.cpp"

  files {
    "./Core/**",
    "./ThirdParty/wyhash/*",
    "./ThirdParty/delegate/*",
  }

  includedirs {
    "./Core/",    
    "./ThirdParty/",
  }

  configuration "win-*"

  links {"mimalloc-static.lib"}
  
  files {
    "./Core/Platforms/Windows/**",
    "./ThirdParty/mimalloc/**"
  }
  includedirs {
    "./Core/Platforms/",
    "./ThirdParty/mimalloc/"
  }
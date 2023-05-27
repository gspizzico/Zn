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

  configurations {
    "win-debug", 
    "win-release",
  }

  -- Defines

  configuration "*-debug"
  defines {
    "_DEBUG",
    "ZN_DEBUG",
    "_CONSOLE"
  } 
  
  configuration "*-release"
  defines {
    "NDEBUG",
    "ZN_RELEASE",
    "_CONSOLE"
  }

  -- Platform Defines

  configuration "win-*"
  defines {
    "PLATFORM_WINDOWS",
    "NOMINMAX",
    "_HAS_EXCEPTIONS=0",
  }

  -- Flags 
  configuration "*-debug"
  flags {
    "FullSymbols"
  }
  
  configuration "*-release"
  flags {
    "Optimize"
  }

  -- Libs

  configuration "*-debug"
  libdirs {
    "../libs/Debug/**"
  }
  
  configuration "*-release"
  libdirs {
    "../libs/Release/**"
  }

  -- Suffixes

  configuration "*-debug"
    targetsuffix "-debug"
  
  configuration "*-release"
    targetsuffix "-release"

  configuration "*"

  -- targetsubdir "./bin/"
  targetsubdir "./../Libs/Debug/"

  dofile("project-core.lua")
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

  objdir "../Intermediate/"

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
    "../Libs/Debug/"
  }
  
  configuration "*-release"
  libdirs {
    "../Libs/Release/"
  }

  -- Linker

  configuration "*"

  links {
    "kernel32",
    "user32",
    "gdi32",
    "rpcrt4",
  }

  -- Suffixes

  configuration "*-debug"
    targetsuffix "-debug"
  
  configuration "*-release"
    targetsuffix "-release"

  configuration "*"

  -- targetsubdir "./../Libs/Debug/" 
  targetsubdir "Binaries" -- to be changed to parent

  dofile("project-zn.lua")
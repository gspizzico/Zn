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
    "NOMINMAX",
    "_HAS_EXCEPTIONS=0",
  }

  configurations {
    "win-debug", 
    "win-release",
  }

  configuration "*-debug"
    targetsuffix "-debug"
  
  configuration "*-release"
    targetsuffix "-release"

  configuration "*"

  -- targetsubdir "./bin/"
  targetsubdir "./../Libs/Debug/"

-- project("math")
  
--   kind "StaticLib"
  
--   configuration "*"

--   files {
--     "./Math/**",
--     "./ThirdParty/glm/**.h",
--     "./ThirdParty/glm/**.cpp",
--     "./ThirdParty/glm/**.hpp",
--     "./ThirdParty/glm/**.inl",
--   }

--   includedirs {
--     "./Math/",
--     "./ThirdParty/glm/"
--   }

project("core")

  links {
    "kernel32",
    "user32",
    "gdi32",
    "rpcrt4",
  }

  solution "ZnGenie"
  location = os.getcwd()

  kind "StaticLib"

  configuration "*"

  pchheader "Core/Corepch.h"
  pchsource "src/Core/Corepch.cpp"

  files {
    "./include/Core/**",
    "./src/Core/**",
    "./ThirdParty/wyhash/*",
    "./ThirdParty/delegate/*",
  }

  includedirs {
    "./include/",    
    "./ThirdParty/",
  }

  configuration "win-*"

  links {"mimalloc-static.lib"}
  
  files {
    "./include/Core/Platforms/Windows/**",
    "./src/Core/Platforms/Windows/**",
    "./ThirdParty/mimalloc/**"
  }
  includedirs {
    "./include/Core/Platforms/",
    "./ThirdParty/mimalloc/"
  }
-- Files
configuration "*"
files {
  "./Engine/**"
}

vpaths{
  ["Headers/Engine/*"] = "Engine/include/**",
  ["Source/Engine/*"] = "Engine/src/**",
}

-- Includes
configuration "*"
includedirs {
  "./Engine/include/"
}
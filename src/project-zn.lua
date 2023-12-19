startproject("ZnEditor")

-- project("ZnCore")
-- kind "StaticLib"

-- dofile("platform-windows.lua")
-- dofile("project-core.lua")
-- dofile("project-application.lua")
-- dofile("project-engine.lua")

project("ZnEditor")
kind "ConsoleApp"

dofile("platform-windows.lua")
dofile("rhi-original.lua")
dofile("project-core.lua")
dofile("project-application.lua")
dofile("project-engine.lua")
dofile("project-editor.lua")

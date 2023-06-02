startproject("ZnEditor")

project("ZnCore")
kind "StaticLib"

dofile("project-core.lua")
dofile("project-application.lua")
dofile("project-engine.lua")

project("ZnEditor")
kind "ConsoleApp"

dofile("project-core.lua")
dofile("project-application.lua")
dofile("project-engine.lua")
dofile("project-editor.lua")

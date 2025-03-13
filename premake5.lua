-- Premake script for OmniScript++
workspace "OmniScript++"
    configurations { "Debug", "Release" }
    architecture "x64"

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Osengine"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    targetdir ("bin/" .. outputdir)
    objdir ("bin-int/" .. outputdir)

    files {
        "src/**.cpp"
        -- "src/main.cpp",
        -- "src/engine/*.cpp",
        -- "src/runtime/object.cpp"
    }

    includedirs {
        "include",
        "dependencies/llvm/include"
    }
    
    libdirs {
        "dependencies/llvm/lib"
        -- "C:/msys64/mingw64/lib"
    }

    links {
        "LLVM-19",
        "pthread"
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        flags { "MultiProcessorCompile" }

    filter "system:windows"
        defines { "PLATFORM_WINDOWS" }
        systemversion "latest"

    filter {}

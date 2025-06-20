-- Premake script for OmniScript++
workspace "OmniScript++"
    configurations { "Debug", "Release" }
    architecture "x64"

    filter "configurations:Debug"
        defines { "DEBUG" }
    
    filter "configurations:Release"
        defines { "NDEBUG" }
    
    filter {}
    
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


project "Osengine"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    targetdir ("bin/" .. outputdir)
    objdir ("bin-int/" .. outputdir)

    files {
        "src/**.cpp"
    }

    includedirs {
        "include",
        "dependencies/llvm/include"
    }
    
    libdirs {
        "dependencies/llvm/lib"
    }

    links {
        "LLVM-20",
        "pthread",
        "quadmath"
    }

    -- Disable stack probes for Windows
    filter { "system:windows", "toolset:msc*" }
        buildoptions { "/mno-stack-arg-probe" }

    filter { "system:windows", "toolset:not msc*" }
        buildoptions { "-mno-stack-arg-probe" }

    filter "system:windows"
        defines { "PLATFORM_WINDOWS" }
        systemversion "latest"

        -- Link against required MSVC runtime libs to fix __chkstk_ms
        links {
            -- "legacy_stdio_definitions",
            "ucrt",
            -- "vcruntime",
            "msvcrt"
        }

        postbuildcommands {
            "powershell -Command \"if (-not (Test-Path -Path 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}')) { New-Item -ItemType Directory -Force -Path 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}' }\"",
            "powershell -Command \"Copy-Item 'dependencies/llvm/bin/*.dll' -Destination 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}'\""
        }

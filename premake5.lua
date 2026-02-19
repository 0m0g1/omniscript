workspace "OmniScript++"
    configurations { "Debug", "Release" }
    architecture "x64"

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter { "configurations:Debug", "system:not windows", "toolset:gcc or clang" }
        buildoptions { "-fsanitize=address", "-fno-omit-frame-pointer" }
        linkoptions  { "-fsanitize=address" }

    filter { "configurations:Debug", "system:windows", "toolset:clang" }
        buildoptions { "-fsanitize=address", "-fno-omit-frame-pointer" }
        linkoptions  { "-fsanitize=address" }

    filter {}

project "Osengine"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++23"
    targetdir ("bin/" .. outputdir)
    objdir ("bin-int/" .. outputdir)

    files { "src/**.cpp" }

    includedirs {
        "include",
        "dependencies/llvm/include"
    }

    libdirs { "dependencies/llvm/lib" }

    links {
        "LLVM-20",
        "pthread",
        "quadmath"
    }

    filter { "system:windows", "toolset:msc*" }
        buildoptions { "/mno-stack-arg-probe" }

    filter { "system:windows", "toolset:not msc*" }
        buildoptions { "-mno-stack-arg-probe" }

    filter "system:windows"
        defines { "PLATFORM_WINDOWS" }
        systemversion "latest"

        links {
            "ucrt",
            "msvcrt"
        }

        postbuildcommands {
            "powershell -Command \"if (-not (Test-Path -Path 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}')) { New-Item -ItemType Directory -Force -Path 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}' }\"",
            "powershell -Command \"Copy-Item 'dependencies/llvm/bin/*.dll' -Destination 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}'\""
        }

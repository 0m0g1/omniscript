-- Change this one value when upgrading LLVM.
-- It must match the files in dependencies/llvm (libLLVM-<version>.dll.a / .dll)
llvm_version = "23"

-- MSYS2 environment to pull LLVM from (override with the MSYS2_ROOT env var).
-- Packages named mingw-w64-x86_64-* install into C:/msys64/mingw64 (legacy msvcrt.dll runtime).
-- Packages named mingw-w64-ucrt-x86_64-* install into C:/msys64/ucrt64 (UCRT runtime).
-- Use ucrt64: the exe and libLLVM must share ONE C runtime, otherwise memory
-- allocated inside LLVM and freed in the exe corrupts the heap.
-- Install with: pacman -S mingw-w64-ucrt-x86_64-llvm mingw-w64-ucrt-x86_64-gcc
msys_root = os.getenv("MSYS2_ROOT") or "C:/msys64/ucrt64"

-- Copies the needed LLVM files from MSYS2 into dependencies/llvm,
-- but only the pieces that are missing. Runs when you run premake5.
local function sync_llvm()
    if os.host() ~= "windows" then return end

    local dest = path.translate(_SCRIPT_DIR .. "/dependencies/llvm", "\\")
    local src  = path.translate(msys_root, "\\")

    local dllA = "libLLVM-" .. llvm_version .. ".dll.a"
    local dll  = "libLLVM-" .. llvm_version .. ".dll"

    -- If the cached copy came from a different MSYS2 environment (e.g. switching
    -- mingw64 -> ucrt64), wipe it so stale headers/libs/DLLs are not reused.
    local marker = dest .. "\\.msys_source"
    if os.isdir(src) and os.isdir(dest) then
        local f = io.open(marker, "r")
        local prev = f and f:read("*l") or nil
        if f then f:close() end
        if prev ~= src then
            print("LLVM source changed (" .. tostring(prev) .. " -> " .. src .. "), clearing dependencies/llvm ...")
            os.execute('rmdir /S /Q "' .. dest .. '"')
        end
    end

    local needInclude = not os.isdir(dest .. "\\include\\llvm")
                     or not os.isdir(dest .. "\\include\\llvm-c")
    local needLib = not os.isfile(dest .. "\\lib\\" .. dllA)
    local needBin = not os.isfile(dest .. "\\bin\\" .. dll)

    if not os.isdir(src) then
        -- Only complain if something is actually missing; otherwise the
        -- existing copies in dependencies/llvm are fine to use as-is.
        if needInclude or needLib or needBin then
            print("WARNING: MSYS2 not found at " .. src .. " - cannot copy LLVM files.")
        end
        return
    end

    if needInclude or needLib or needBin then
        print("Copying missing LLVM " .. llvm_version .. " files from " .. src .. " ...")
    end

    -- robocopy exit codes below 8 mean success
    local function robo(from, to, extra)
        os.mkdir(to)
        os.execute('robocopy "' .. from .. '" "' .. to .. '" ' .. extra .. " /NFL /NDL /NJH /NJS /NP >nul")
    end

    if needInclude then
        robo(src .. "\\include\\llvm",   dest .. "\\include\\llvm",   "/E")
        robo(src .. "\\include\\llvm-c", dest .. "\\include\\llvm-c", "/E")
    end

    if needLib then
        robo(src .. "\\lib", dest .. "\\lib", dllA)
    end

    if needBin then
        robo(src .. "\\bin", dest .. "\\bin", dll)
    end

    -- Always refresh the runtime DLLs so they match the LLVM build.
    -- robocopy skips files that are already identical, so this is cheap.
    robo(src .. "\\bin", dest .. "\\bin",
        "libzstd.dll zlib1.dll libxml2-*.dll libffi-*.dll libstdc++-6.dll" ..
        " libgcc_s_seh-1.dll libwinpthread-1.dll libiconv-2.dll liblzma-5.dll")

    if not os.isfile(dest .. "\\lib\\" .. dllA) or not os.isfile(dest .. "\\bin\\" .. dll) then
        print("WARNING: " .. dllA .. " / " .. dll .. " not found in MSYS2. " ..
              "Check 'pacman -Q mingw-w64-ucrt-x86_64-llvm' and llvm_version.")
    else
        os.mkdir(dest)
        local f = io.open(marker, "w")
        if f then f:write(src, "\n") f:close() end
    end
end

sync_llvm()

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
        "LLVM-" .. llvm_version,
        "pthread",
        "quadmath"
    }

    filter "system:windows"
        defines { "PLATFORM_WINDOWS" }
        systemversion "latest"
        buildoptions { "-mno-stack-arg-probe" }

        -- No explicit "ucrt"/"msvcrt" links: the ucrt64 toolchain already links
        -- UCRT, and naming both pulls two C runtimes into the exe.

        postbuildcommands {
            "powershell -Command \"if (-not (Test-Path -Path 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}')) { New-Item -ItemType Directory -Force -Path 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}' }\"",
            "powershell -Command \"Copy-Item 'dependencies/llvm/bin/*.dll' -Destination 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}' -Force\""
        }

    filter {}
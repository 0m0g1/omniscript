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

-- Copies shaderc_shared.dll (GLSL -> SPIR-V compiler used by Vulkan.os) from the installed
-- Vulkan SDK into dependencies/vk, but only if it is missing. The SDK installer sets the
-- VULKAN_SDK env var (e.g. C:\VulkanSDK\1.4.xxx.x); override the lookup with it if needed.
local function sync_vulkan()
    if os.host() ~= "windows" then return end

    local dest = path.translate(_SCRIPT_DIR .. "/dependencies/vk", "\\")
    local dll  = dest .. "\\shaderc_shared.dll"
    if os.isfile(dll) then return end

    local sdk = os.getenv("VULKAN_SDK")
    if not sdk or not os.isfile(sdk .. "\\Bin\\shaderc_shared.dll") then
        print("WARNING: shaderc_shared.dll not found. Install the Vulkan SDK (vulkan.lunarg.com) " ..
              "or copy shaderc_shared.dll into dependencies/vk. Vulkan shader compiling will not work without it.")
        return
    end

    print("Copying shaderc_shared.dll from " .. sdk .. " ...")
    os.mkdir(dest)
    os.copyfile(sdk .. "\\Bin\\shaderc_shared.dll", dll)
end

sync_llvm()
sync_vulkan()

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

    local std_src = path.translate(_SCRIPT_DIR .. "/standard", "\\")
    local dep_src = path.translate(_SCRIPT_DIR .. "/dependencies", "\\")

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
        -- buildoptions { "-mno-stack-arg-probe" }
        linkoptions { "-Wl,--stack,8388608" }

        -- No explicit "ucrt"/"msvcrt" links: the ucrt64 toolchain already links
        -- UCRT, and naming both pulls two C runtimes into the exe.

        postbuildcommands {
            "powershell -Command \"if (-not (Test-Path -Path 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}')) { New-Item -ItemType Directory -Force -Path 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}' }\"",
            "powershell -Command \"Copy-Item 'dependencies/llvm/bin/*.dll' -Destination 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}' -Force\"",
            -- shaderc_shared.dll must sit next to the exe (shaderc.os loads it by bare name)
            "powershell -Command \"if (Test-Path 'dependencies/vk/shaderc_shared.dll') { Copy-Item 'dependencies/vk/shaderc_shared.dll' -Destination 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}' -Force } else { Write-Warning 'shaderc_shared.dll missing in dependencies/vk' }\"",
            -- standard/ travels with the exe so findToolchainRoot() finds it at
            -- exe.parent_path()/standard wherever bin/ is moved. /MIR also deletes files
            -- removed from the source. robocopy exits 0-7 on success, 8+ on failure,
            -- so mask the low codes or every successful copy would fail the build.
            -- No "$" in the command: make would expand $LASTEXITCODE as its own variable.
            "powershell -NoProfile -Command \"robocopy '" .. std_src .. "' 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/standard' /MIR /NFL /NDL /NJH /NJS /NP; if ((Get-Variable LASTEXITCODE -ValueOnly) -ge 8) { exit 1 } else { exit 0 }\"",
            -- Runtime DLLs of dependencies/ (glfw, stb, libcurl, ...) keep their relative layout
            -- under bin/<cfg>/dependencies so extern paths like "dependencies/glfw/.../glfw3.dll"
            -- still resolve (relative to the toolchain root). Only *.dll are copied; headers,
            -- import libs and the (already flattened) llvm folder are skipped.
            "powershell -NoProfile -Command \"robocopy '" .. dep_src .. "' 'bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/dependencies' *.dll /S /XD llvm include /NFL /NDL /NJH /NJS /NP; if ((Get-Variable LASTEXITCODE -ValueOnly) -ge 8) { exit 1 } else { exit 0 }\""
        }

    filter {}
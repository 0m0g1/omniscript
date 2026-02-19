-- premake5.lua
workspace "OmniScript"
  architecture "x86_64"
  configurations { "Debug", "Release" }
  startproject "Osengine"
  cppdialect "C++20"
  toolset "clang"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- ----------------------------
-- Helpers
-- ----------------------------
local function trim(s)
  return (s or ""):gsub("^%s*(.-)%s*$", "%1")
end

local function split_words(s)
  local t = {}
  for w in string.gmatch(s or "", "%S+") do
    table.insert(t, w)
  end
  return t
end

-- Convert "-lLLVM-20 -lLLVMSupport" -> { "LLVM-20", "LLVMSupport" }
local function libs_from_flags(flag_string)
  local out = {}
  for _, w in ipairs(split_words(flag_string)) do
    local name = w:match("^%-l(.+)$")
    if name then table.insert(out, name) end
  end
  return out
end

-- Convert "-L/usr/lib/llvm-20/lib -L/other" -> { "/usr/lib/llvm-20/lib", "/other" }
local function libdirs_from_flags(flag_string)
  local out = {}
  for _, w in ipairs(split_words(flag_string)) do
    local dir = w:match("^%-L(.+)$")
    if dir then table.insert(out, dir) end
  end
  return out
end

-- ----------------------------
-- LLVM auto-detect
-- ----------------------------
local LLVM = { found = false }

local llvm_config = os.getenv("LLVM_CONFIG") or "llvm-config"
local llvm_dir    = os.getenv("LLVM_DIR")

do
  local inc = trim(os.outputof('"' .. llvm_config .. '" --includedir 2>/dev/null'))
  local lib = trim(os.outputof('"' .. llvm_config .. '" --libdir 2>/dev/null'))
  local libs = trim(os.outputof('"' .. llvm_config .. '" --libs core support 2>/dev/null'))
  local syslibs = trim(os.outputof('"' .. llvm_config .. '" --system-libs 2>/dev/null'))
  local ldflags = trim(os.outputof('"' .. llvm_config .. '" --ldflags 2>/dev/null'))

  if inc ~= "" and lib ~= "" then
    LLVM.found     = true
    LLVM.includedir= inc
    LLVM.libdir    = lib
    LLVM.libs_raw  = libs
    LLVM.syslibs_raw = syslibs
    LLVM.ldflags_raw = ldflags

    -- Parsed
    LLVM.libnames  = libs_from_flags((libs or "") .. " " .. (syslibs or ""))
    LLVM.libdirs   = libdirs_from_flags(ldflags)
  end
end

-- Fallback: LLVM_DIR (useful esp. Windows)
if (not LLVM.found) and llvm_dir and #llvm_dir > 0 then
  local inc = path.join(llvm_dir, "include")
  local lib = path.join(llvm_dir, "lib")
  if os.isdir(inc) and os.isdir(lib) then
    LLVM.found      = true
    LLVM.includedir = inc
    LLVM.libdir     = lib
    -- If you use this fallback, set LLVM_LIBS yourself (e.g. "LLVMCore LLVMSupport" or "-lLLVM-20")
    local env_libs = os.getenv("LLVM_LIBS") or ""
    LLVM.libnames  = libs_from_flags(env_libs)
    LLVM.libdirs   = {}
    LLVM.ldflags_raw = ""
  end
end

-- ----------------------------
-- Global settings
-- ----------------------------
filter "configurations:Debug"
  symbols "On"
  optimize "Off"
  defines { "OMNI_DEBUG" }

filter "configurations:Release"
  symbols "Off"
  optimize "Speed"
  defines { "OMNI_RELEASE" }

filter "system:linux"
  systemversion "latest"
  pic "On"
  buildoptions { "-fdiagnostics-color=always" }

filter {}
  
-- ----------------------------
-- OmniCore (Static Library)
-- ----------------------------
project "OmniCore"
  kind "StaticLib"
  language "C++"

  targetdir ("bin/" .. outputdir .. "/%{prj.name}")
  objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

  files {
    "engine/src/**.cpp",
    "engine/include/**.h",
    "engine/include/**.hpp"
  }

  -- Keep exe entrypoint out of the library
  removefiles { "engine/src/main.cpp" }

  includedirs { "engine/include" }

  filter "system:linux"
    links { "dl", "pthread" }
  filter {}

  if LLVM.found then
    defines { "OMNI_HAS_LLVM=1" }
    includedirs { LLVM.includedir }
    -- Note: linking LLVM here is optional; final link must happen in Osengine.
  else
    defines { "OMNI_HAS_LLVM=0" }
  end

-- ----------------------------
-- Osengine (Executable)
-- ----------------------------
project "Osengine"
  kind "ConsoleApp"
  language "C++"

  targetdir ("bin/" .. outputdir .. "/%{prj.name}")
  objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

  files { "engine/src/main.cpp" }
  includedirs { "engine/include" }

  links { "OmniCore" }

  filter "system:linux"
    links { "dl", "pthread" }
  filter {}

  -- IMPORTANT: LLVM must be linked at the final binary
  if LLVM.found then
    includedirs { LLVM.includedir }

    -- Prefer llvm-config's --libdir; also add any -L dirs found in ldflags
    libdirs { LLVM.libdir }
    if LLVM.libdirs and #LLVM.libdirs > 0 then
      libdirs (LLVM.libdirs)
    end

    -- This will include "LLVM-20" based on your llvm-config output: -lLLVM-20
    links (LLVM.libnames)

    -- Keep ldflags in case it contains useful linker args (rpath, etc.)
    if LLVM.ldflags_raw and LLVM.ldflags_raw ~= "" then
      linkoptions { LLVM.ldflags_raw }
    end
  end

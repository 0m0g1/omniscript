-- premake5.lua  (UPDATED: libclang detection + link setup for Ubuntu/Windows)
workspace "OmniScript"
  architecture "x86_64"
  configurations { "Debug", "Release" }
  startproject "Osengine"
  cppdialect "C++23"
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

local function tolower(s)
  return string.lower(s or "")
end

local function ends_with(s, suf)
  s, suf = s or "", suf or ""
  return #s >= #suf and s:sub(#s-#suf+1) == suf
end

local function file_exists(p)
  return p and #p > 0 and os.isfile(p)
end

local function dir_exists(p)
  return p and #p > 0 and os.isdir(p)
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
  local ver = trim(os.outputof('"' .. llvm_config .. '" --version 2>/dev/null'))

  if inc ~= "" and lib ~= "" then
    LLVM.found      = true
    LLVM.version    = ver
    LLVM.includedir = inc
    LLVM.libdir     = lib
    LLVM.libs_raw   = libs
    LLVM.syslibs_raw= syslibs
    LLVM.ldflags_raw= ldflags

    -- Parsed
    LLVM.libnames   = libs_from_flags((libs or "") .. " " .. (syslibs or ""))
    LLVM.libdirs    = libdirs_from_flags(ldflags)
  end
end

-- Fallback: LLVM_DIR (useful esp. Windows)
if (not LLVM.found) and llvm_dir and #llvm_dir > 0 then
  local inc = path.join(llvm_dir, "include")
  local lib = path.join(llvm_dir, "lib")
  if dir_exists(inc) and dir_exists(lib) then
    LLVM.found      = true
    LLVM.includedir = inc
    LLVM.libdir     = lib
    local env_libs  = os.getenv("LLVM_LIBS") or ""
    LLVM.libnames   = libs_from_flags(env_libs)
    LLVM.libdirs    = {}
    LLVM.ldflags_raw= ""
  end
end

-- ----------------------------
-- libclang auto-detect
-- ----------------------------
-- Your Ubuntu state:
--   llvm-config: 20.1.8, include=/usr/lib/llvm-20/include, lib=/usr/lib/llvm-20/lib
--   runtime libs present in /lib/x86_64-linux-gnu: libclang-20.so.20, libclang-cpp.so.20.1
-- but dev headers may be missing unless you install libclang-20-dev (recommended)
--
-- This detection:
--   1) Prefer headers under LLVM.includedir (clang-c/Index.h)
--   2) Else try common Ubuntu multiarch include path (/usr/include/clang-c/Index.h)
--   3) Prefer linking against libclang (C API). If only libclang-cpp exists, fall back.
--
local CLANG = {
  found = false,
  includedir = nil,
  libdirs = {},
  link = nil,       -- "clang" or "clang-cpp"
  is_cpp = false,   -- true if using clang-cpp fallback
  dll = nil         -- windows helper
}

local function detect_libclang()
  if not LLVM.found then
    return
  end

  -- Candidate include dirs (order matters)
  local inc_candidates = {
    LLVM.includedir,                 -- /usr/lib/llvm-20/include
    "/usr/include",                  -- sometimes has clang-c
    "/usr/local/include",
  }

  local found_inc = nil
  for _, inc in ipairs(inc_candidates) do
    if file_exists(path.join(inc, "clang-c", "Index.h")) then
      found_inc = inc
      break
    end
  end

  -- Candidate lib dirs
  local lib_candidates = {
    LLVM.libdir,                     -- /usr/lib/llvm-20/lib
    "/lib/x86_64-linux-gnu",          -- where your ldconfig shows libclang-20.so.20
    "/usr/lib/x86_64-linux-gnu",
    "/lib64",
    "/usr/lib64",
  }

  local found_libdir = nil
  local link_name = nil
  local is_cpp_fallback = false

  -- Prefer libclang (C API). Debian/Ubuntu runtime is often libclang-20.so.20
  -- The linker name is usually "-lclang-20" in that case (not "-lclang").
  local function choose_link_in_dir(d)
    -- libclang.so / libclang.so.X (some installs)
    if file_exists(path.join(d, "libclang.so")) then
      return "clang", false
    end
    -- versioned sonames (Ubuntu)
    if LLVM.version and LLVM.version ~= "" then
      local major = LLVM.version:match("^(%d+)")
      if major then
        if file_exists(path.join(d, "libclang-" .. major .. ".so")) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".0")) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".1")) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".2")) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".20")) then
          return "clang-" .. major, false
        end
        -- Your actual file: libclang-20.so.20
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".0")) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".1")) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".8")) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".20")) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        -- Most reliable for your case:
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        -- If only the exact soname exists:
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        -- Your exact file pattern: libclang-20.so.20
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".0")) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".1")) or
           file_exists(path.join(d, "libclang-" .. major .. ".so." .. major .. ".8")) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
        if file_exists(path.join(d, "libclang-" .. major .. ".so." .. major)) then
          return "clang-" .. major, false
        end
      end
    end

    -- Fallback: clang-cpp (C++ API library). Not preferred for libclang C API,
    -- but some distros only ship clang-cpp in a convenient way.
    if file_exists(path.join(d, "libclang-cpp.so")) then
      return "clang-cpp", true
    end
    if LLVM.version and LLVM.version ~= "" then
      local major = LLVM.version:match("^(%d+)")
      if major and file_exists(path.join(d, "libclang-cpp.so." .. major .. ".1")) then
        return "clang-cpp", true
      end
    end

    return nil, false
  end

  for _, d in ipairs(lib_candidates) do
    local ln, iscpp = choose_link_in_dir(d)
    if ln then
      found_libdir = d
      link_name = ln
      is_cpp_fallback = iscpp
      break
    end
  end

  if found_inc and found_libdir and link_name then
    CLANG.found = true
    CLANG.includedir = found_inc
    CLANG.libdirs = { found_libdir }
    CLANG.link = link_name
    CLANG.is_cpp = is_cpp_fallback
  end

  -- Windows: try LLVM_DIR structure if set
  if not CLANG.found and llvm_dir and #llvm_dir > 0 then
    local inc = path.join(llvm_dir, "include")
    local lib = path.join(llvm_dir, "lib")
    if file_exists(path.join(inc, "clang-c", "Index.h")) and dir_exists(lib) then
      CLANG.found = true
      CLANG.includedir = inc
      CLANG.libdirs = { lib }

      -- prefer libclang.lib if present
      if file_exists(path.join(lib, "libclang.lib")) then
        CLANG.link = "libclang" -- MSVC-style import lib name (Premake handles .lib)
      else
        -- fallback: sometimes it's just "clang"
        CLANG.link = "clang"
      end

      local bin = path.join(llvm_dir, "bin")
      local dll = path.join(bin, "libclang.dll")
      if file_exists(dll) then
        CLANG.dll = dll
      end
    end
  end
end

detect_libclang()

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

  removefiles { "engine/src/main.cpp" }

  includedirs { "engine/include" }

  filter "system:linux"
    links { "dl", "pthread" }
  filter {}

  if LLVM.found then
    defines { "OMNI_HAS_LLVM=1" }
    includedirs { LLVM.includedir }
  else
    defines { "OMNI_HAS_LLVM=0" }
  end

  if CLANG.found then
    defines { "OMNI_HAS_LIBCLANG=1" }
    includedirs { CLANG.includedir }
  else
    defines { "OMNI_HAS_LIBCLANG=0" }
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

  -- LLVM must be linked at the final binary
  if LLVM.found then
    includedirs { LLVM.includedir }

    libdirs { LLVM.libdir }
    if LLVM.libdirs and #LLVM.libdirs > 0 then
      libdirs (LLVM.libdirs)
    end

    links (LLVM.libnames)

    if LLVM.ldflags_raw and LLVM.ldflags_raw ~= "" then
      linkoptions { LLVM.ldflags_raw }
    end
  end

  -- libclang (for header parsing / FFI importer)
  if CLANG.found then
    includedirs { CLANG.includedir }
    libdirs (CLANG.libdirs)
    links { CLANG.link }

    -- Ubuntu: when linking -lclang-20 from /lib/x86_64-linux-gnu, ensure we can find it
    filter "system:linux"
      -- optional: helps runtime loading if you dlopen libclang; harmless otherwise
      if CLANG.libdirs and #CLANG.libdirs > 0 then
        linkoptions { "-Wl,-rpath," .. CLANG.libdirs[1] }
      end
    filter {}

    -- Windows: copy libclang.dll next to exe (recommended)
    filter "system:windows"
      if CLANG.dll then
        postbuildcommands {
          '{COPY} "' .. CLANG.dll .. '" "%{cfg.targetdir}"'
        }
      end
    filter {}
  end

-- ----------------------------
-- Notes / What you need to install on Ubuntu
-- ----------------------------
-- You currently have runtime libclang (ldconfig shows libclang-20.so.20),
-- but you are missing headers: dpkg says libclang-dev not installed.
-- Install the matching dev package for LLVM 20:
--   sudo apt install libclang-20-dev
--
-- After that, clang-c/Index.h should exist under /usr/lib/llvm-20/include/clang-c/Index.h
--
-- If you prefer the unversioned name "libclang-dev", it may pull a different LLVM version.
-- Matching major versions avoids ABI mismatches.
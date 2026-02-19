# build.ps1
# Usage:
#   pwsh ./build.ps1
#   pwsh ./build.ps1 -Config release
# Optional:
#   $env:LLVM_CONFIG="C:\path\to\llvm-config.exe" (or /usr/bin/llvm-config on linux)
#   $env:CC="clang"
#   $env:CXX="clang++"

param(
  [ValidateSet("debug","release")]
  [string]$Config = "debug"
)

$ErrorActionPreference = "Stop"

function Find-Exe($names) {
  foreach ($n in $names) {
    $cmd = Get-Command $n -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Path }
  }
  return $null
}

# Tools
$premake = Find-Exe @("premake5")
if (-not $premake) { throw "premake5 not found on PATH." }

$make = Find-Exe @("make","gmake")
if (-not $make) { throw "make/gmake not found on PATH. (On Windows, install MSYS2/WSL and ensure make is on PATH.)" }

$clang = Find-Exe @("clang")
$clangxx = Find-Exe @("clang++")
if (-not $clang -or -not $clangxx) { throw "clang/clang++ not found on PATH." }

# Force clang toolchain for make
if (-not $env:CC)  { $env:CC  = "clang" }
if (-not $env:CXX) { $env:CXX = "clang++" }

# Auto-detect llvm-config unless user already set LLVM_CONFIG
if (-not $env:LLVM_CONFIG -or $env:LLVM_CONFIG.Trim().Length -eq 0) {
  $llvmConfig = Find-Exe @("llvm-config","llvm-config-18","llvm-config-17","llvm-config-16","llvm-config-15","llvm-config-14")
  if ($llvmConfig) {
    $env:LLVM_CONFIG = $llvmConfig
  } else {
    Write-Host "Note: llvm-config not found. LLVM backend will NOT be enabled (expected)." -ForegroundColor Yellow
  }
}

Write-Host "premake5: $premake"
Write-Host "make:     $make"
Write-Host "CC:       $env:CC"
Write-Host "CXX:      $env:CXX"
if ($env:LLVM_CONFIG) { Write-Host "LLVM_CONFIG: $env:LLVM_CONFIG" }

& cd ..

# Generate makefiles (premake gmake; gmake2 is deprecated)
& premake5 gmake

# Build
& $make "config=$Config"

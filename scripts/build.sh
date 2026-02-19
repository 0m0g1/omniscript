#!/usr/bin/env bash
set -euo pipefail

# Choose premake action (premake5 renamed gmake2 -> gmake)
PMAKE_ACTION="${PMAKE_ACTION:-gmake}"

# Prefer clang if installed (common for your setup)
export CC="${CC:-clang}"
export CXX="${CXX:-clang++}"

# Auto-detect llvm-config unless user already set LLVM_CONFIG
if [[ -z "${LLVM_CONFIG:-}" ]]; then
  if command -v llvm-config >/dev/null 2>&1; then
    export LLVM_CONFIG="$(command -v llvm-config)"
  elif command -v llvm-config-18 >/dev/null 2>&1; then
    export LLVM_CONFIG="$(command -v llvm-config-18)"
  elif command -v llvm-config-17 >/dev/null 2>&1; then
    export LLVM_CONFIG="$(command -v llvm-config-17)"
  elif command -v llvm-config-16 >/dev/null 2>&1; then
    export LLVM_CONFIG="$(command -v llvm-config-16)"
  elif command -v llvm-config-15 >/dev/null 2>&1; then
    export LLVM_CONFIG="$(command -v llvm-config-15)"
  elif command -v llvm-config-14 >/dev/null 2>&1; then
    export LLVM_CONFIG="$(command -v llvm-config-14)"
  else
    echo "Note: llvm-config not found on PATH. LLVM backend will be disabled (if your premake is optional-aware)." >&2
  fi
fi

# Detect OS (for messaging only)
UNAME="$(uname -s || true)"
if [[ "$UNAME" == MINGW* || "$UNAME" == MSYS* || "$UNAME" == CYGWIN* ]]; then
  echo "Windows shell detected. This script assumes make + a POSIX shell."
  echo "Use one of:"
  echo "  - MSYS2 + mingw-w64 + make"
  echo "  - Git Bash + make toolchain"
  echo "  - WSL (recommended) for this exact workflow"
  echo "Or generate Visual Studio projects with: premake5 vs2022"
  exit 1
fi

echo "Using premake action: $PMAKE_ACTION"
echo "CC=$CC CXX=$CXX"
if [[ -n "${LLVM_CONFIG:-}" ]]; then
  echo "LLVM_CONFIG=$LLVM_CONFIG"
fi

cd ..

premake5 "$PMAKE_ACTION"
make "config=${CONFIG:-debug}"

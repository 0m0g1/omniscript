# Build Instructions

How to build the OmniScript compiler (`Osengine`) from source, and a reference for its command-line options.

> ⚠️ Early development. Build steps and flags may change.

## Requirements

- Windows 10+, x86_64 (primary platform)
- ~4 GB RAM and ~500 MB disk for a full build
- `make.exe` and `premake5.exe` (bundled in `scripts/`)
- LLVM headers and libraries in `dependencies/llvm/include` and `dependencies/llvm/lib` (bundled)

> **TODO (maintainer):** document the LLVM version and the C++ compiler used to build.

## Build

### 1. Generate makefiles

```bash
./scripts/premake/premake5.exe gmake2
```

Re-run whenever `premake5.lua` changes or a C++ source file is added.

### 2. Clean (optional)

```bash
make clean
```

Only needed after moving C++ files or making structural changes.

### 3. Compile

```bash
make config=debug      # symbols, assertions, verbose errors
make config=release    # optimized
```

Use `make -j4 config=release` to build in parallel.

### Output

```
bin/{config}-{system}-{arch}/Osengine.exe
```

Examples: `bin/Debug-windows-x86_64/`, `bin/Release-windows-x86_64/`. The premake script copies the required LLVM DLLs into this folder.

## Running

```bash
Osengine.exe [options] file.os
```

```bash
# JIT
Osengine.exe examples/types.os --execute

# JIT with debug logs
Osengine.exe examples/types.os --execute --debug

# AOT: standalone executable
Osengine.exe examples/types.os --make -o types.exe
```

## Command-line reference

Flags are listed as documented in [Compiler](Compiler.md). Run `Osengine.exe --help` for the authoritative list in your build.

### Execution and output

| Flag | Description |
|---|---|
| `--execute` | Run in JIT mode |
| `--make` | Compile in AOT mode |
| `--entry <function>` | Set the entry function (also written `--entry=name`) |
| `--output`, `-o <file>` | Output path |
| `--dry` | Dry compilation |
| `--keep-obj` | Keep intermediate object files |

### Inputs and modules

| Flag | Description |
|---|---|
| `--main-source <file>` | Primary source file |
| `--source <file>` | Additional source file (repeatable) |
| `--enable-modules <path>` | Enable module support from a path |
| `-` as file path | Read source from stdin |

### Targets

| Flag | Description |
|---|---|
| `--target-os <os>` | `linux`, `windows`/`win32`, `macos`/`darwin`, `freebsd`, `android`, `ios`, `wasm`/`webassembly`, `auto` |
| `--target-arch <arch>` | e.g. `x86_64`, `arm64` |
| `--target-triple <triple>` | e.g. `x86_64-linux-gnu` |
| `--list-targets` | List available targets |
| `--show-host-info` | Print host architecture and OS, then exit |

JIT requires the host OS to match the target. AOT can cross-compile, but platform-specific symbols (for example `CreateThread` on Linux) will fail.

### Optimization

| Flag | Description |
|---|---|
| `--enable-lto` | Link-time optimization |
| `--enable-pgo` | Profile-guided optimization |
| `--enable-vectorization` | Vectorization |
| `--enable-inlining` | Function inlining |
| `--enable-tail-calls` | Tail-call optimization |
| `--fast-math` | Fast-math optimizations |
| `--enable-pic` | Position-independent code |

`-O3` is the default for both JIT and AOT.

### Emit options

| Flag | Output |
|---|---|
| `--emit-object` | Object file |
| `--emit-assembly` | Assembly (`.s`) |
| `--emit-ir` | LLVM IR (`.ll`) |
| `--emit-bitcode` | LLVM bitcode (`.bc`) |
| `--emit-staticlib` / `--emit-sharedlib` | Static / shared library |
| `--emit-wasm` | WebAssembly (`.wasm`) |

### Safety and sanitizers

| Flag | Description |
|---|---|
| `--safety <level>` | `standard`, `paranoid` |
| `--enable-stack-protection` | Stack protection |
| `--enable-cfi` | Control-flow integrity |
| `--enable-asan` / `--enable-msan` / `--enable-tsan` / `--enable-ubsan` | Sanitizers |

### Memory

| Flag | Description |
|---|---|
| `--gc <strategy>` | e.g. `refcounting`, `marksweep` |
| `--enable-parallel-gc` / `--enable-concurrent-gc` | GC modes |
| `--heap-size <size>` / `--stack-size <size>` | e.g. `64MB`, `8MB` |

### Diagnostics and build speed

| Flag | Description |
|---|---|
| `--debug`, `-d` | Detailed logs and performance stats |
| `--verbose` | Parse tree and token stream (with `--debug`) |
| `--log-asm` / `--log-final-code` | Log assembly / final IR |
| `--show-metadata` | Show metadata during compilation |
| `--error-log <file>` | Write errors to a file |
| `--max-errors <n>` | Stop after n errors |
| `--incremental`, `--cache-dir <dir>`, `--parallel-jobs <n>` | Incremental builds |
| `--profiler <type>` | e.g. `perf`, `gprof` |
| `--version`, `--help` | Info |

## AOT linking

For `--make`, the compiler generates linker arguments from the library paths in your `extern` blocks, then tries linkers in order: `clang++`, `g++`, and on Windows the MSVC `link`. It adds default libraries (`user32`, `gdi32`, `shell32`, `kernel32`, `ntdll` on Windows; `-lm -ldl -lpthread` elsewhere), copies only the non-system DLLs your program needs, and can write a symbol table of globals, functions, and aliases. See [FFI](ffi.md).

## Platform notes

- **Windows:** Visual C++ runtime libraries may be required. Antivirus may flag freshly built compilers.
- **Linux/macOS:** not yet supported by the build scripts. Expect to need `gcc`/`clang`, `make`, and a system LLVM.

## Troubleshooting

| Error | Solution |
|---|---|
| `LLVM headers not found in dependencies/llvm/include` | Populate `dependencies/llvm/`. |
| `Access denied` running `premake5.exe` | Check antivirus, or run from an elevated prompt. |
| `'make' is not recognized` | Use the bundled `make.exe` or install MinGW-w64. |
| Missing LLVM DLLs at runtime | Re-run the build or copy the DLLs beside `Osengine.exe`. |
| `Linking failed. Available linkers: ...` | Install `clang++` or `g++` (or MSVC) and check the last error shown. |

## Development workflow

```bash
./scripts/premake/premake5.exe gmake2     # when project structure changed
make config=debug
./bin/Debug-windows-x86_64/Osengine.exe examples/types.os --execute --debug
make config=release                        # when ready
```

Full CI-style build:

```bash
make clean
./scripts/premake/premake5.exe gmake2
make config=debug
make config=release
```

## Custom configurations

Edit `premake5.lua`:

```lua
configurations { "Debug", "Release", "Profile" }

filter "configurations:Profile"
    defines { "PROFILE_BUILD" }
    optimize "Speed"
    symbols "On"
```

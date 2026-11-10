# Getting Started with OmniScript

This guide takes you from a fresh clone to running and compiling your first program.

> ⚠️ OmniScript is in early development. Windows x86_64 is the primary tested platform.

## Prerequisites

| Requirement | Notes |
|---|---|
| Windows 10+ (x86_64) | Primary platform. Linux/macOS are untested. |
| Build tools | `make.exe` and `premake5.exe` ship in the repo's `scripts/` folder. Nothing to install. |
| LLVM | Headers and libraries live in `dependencies/llvm/include` and `dependencies/llvm/lib`. |
| C++ compiler | Needed to build the compiler itself (see [Build Instructions](buildinstructions.md)). |

> **TODO (maintainer):** state the exact LLVM version and the C++ compiler/toolchain used, and link a prebuilt `Osengine.exe` release so users can skip the build.

## 1. Get the source

```bash
git clone https://github.com/0m0g1/omniscript.git
cd omniscript
```

## 2. Build the compiler

```bash
./scripts/premake/premake5.exe gmake2
make config=release
```

Use `make config=debug` for a debug build. Full details, including when to regenerate makefiles, are in [Build Instructions](buildinstructions.md).

The binary is written to `bin/{config}-{system}-{arch}/`, for example `bin/Release-windows-x86_64/Osengine.exe`.

## 3. Hello, world

Create `hello.os`:

```os
extern "C" fn printf(fmt: char*, ...) => int;

printf("Hello, OmniScript!\n");
```

Run it with the JIT:

```bash
./bin/Release-windows-x86_64/Osengine.exe hello.os --execute
```

Expected output:

```
Hello, OmniScript!
```

Every statement ends with a semicolon.

## 4. Compile to an executable

```bash
./bin/Release-windows-x86_64/Osengine.exe hello.os --make -o hello.exe
./hello.exe
```

| Goal | Command |
|---|---|
| Run immediately (JIT) | `Osengine.exe file.os --execute` |
| Compile to a standalone executable (AOT) | `Osengine.exe file.os --make -o out.exe` |
| Run with a custom entry function | `Osengine.exe file.os --entry=myStart` |
| Verbose compiler logs | add `--debug` |
| Cross-compile | add `--target-os linux` (AOT only) |

The full flag list is in [Build Instructions](buildinstructions.md#command-line-reference).

## 5. A slightly bigger program

```os
extern "C" fn printf(fmt: char*, ...) => int;

struct Point {
    x: f32 = 0.0;
    y: f32 = 0.0;

    lengthSquared() => f32 {
        return this.x * this.x + this.y * this.y;
    }
}

function main() => i32 {
    let p = Point{ x: 3.0, y: 4.0 };
    printf("length^2 = %f\n", p.lengthSquared() as f64);
    return 0;
}
```

`printf` is variadic C, so floats are passed as `f64`: cast with `as f64`.

## Execution models

1. **Top-down:** no `main`; the file runs from top to bottom.
2. **`main`:** top-level code runs first, then `main`. Accepted signatures: `main()`, `main(argc: int)`, `main(argc: int, argv: char**)`. Returning `void` is also used in the repo's examples.
3. **Custom entry:** any function, chosen with `--entry`.

## Multiple files and libraries

- Add files with `--source other.os` and name the primary file with `--main-source main.os`.
- Repo examples pull in code with `include "path/file.os";`.
- Link C libraries by path in an `extern` block. See [External Bindings](externalbindings.md) and [FFI](ffi.md). In AOT mode the compiler generates linker arguments and copies the DLLs your program needs.

## Editor support

1. Clone the [OmniScript Language Server](https://github.com/0m0g1/omniscript-language-server).
2. In VS Code run **Developer: Install Extension from Location**.
3. Select the cloned folder.

You get syntax highlighting, completion, and error detection.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `premake5.exe` not found | Run from the repo root and check `scripts/premake/premake5.exe` exists. |
| `make` not recognized | Use the bundled `make.exe` from `scripts/`, or install MinGW-w64. |
| Engine exits immediately or DLL errors | Make sure the LLVM DLLs were copied next to `Osengine.exe`. Re-run the build. |
| LLVM headers not found | Check `dependencies/llvm/include` and `dependencies/llvm/lib` are populated. |
| Library not found at JIT time | In JIT mode, `extern` library paths are used to verify symbols. Fix the path or put the DLL beside the working directory. |
| Linker failed (AOT) | The compiler tries `clang++`, then `g++`, then MSVC `link`. Install at least one and check the error log. |

## Next steps

[Syntax](syntax.md) · [Data Types](datatypes.md) · [Operations](operations.md) · [Control Flow](controlflow.md) · [Functions](functions.md) · [Classes and Structs](classesandstructs.md) · [Optionals](optionals.md) · [External Bindings](externalbindings.md) · [FFI](ffi.md)

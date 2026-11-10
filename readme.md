# OmniScript

A systems programming language with an LLVM backend. It reads like a scripting language, gives C-level control, and calls C libraries directly. Run it instantly with the JIT, or compile it to a standalone executable.

> ⚠️ **Early development.** Not ready for general use. Syntax and behavior will change without notice. Windows x86_64 is the primary tested platform.

OmniScript was originally and is still sometimes called **OS** in the docs while the final name is settled. Source files use the `.os` extension and the compiler binary is `Osengine`.

## Example

```os
extern "C" fn printf(fmt: char*, ...) => int;

struct Point {
    x: f32 = 0.0;
    y: f32 = 0.0;
}

function main() => i32 {
    let p = Point{ x: 3.0, y: 4.0 };
    printf("(%f, %f)\n", p.x as f64, p.y as f64);
    return 0;
}
```

```bash
Osengine.exe hello.os --execute                    # run with the JIT
Osengine.exe hello.os --make -o hello.exe          # compile to an executable
```

## Features

- **JIT and AOT** through LLVM, with `-O3` by default and no runtime
- **Three execution models:** top-down scripts, `main`, or any function via `--entry`
- **Direct C interop:** declare C functions with `extern`, link `.dll`, `.so`, `.a` or `.lib` by path; the AOT build generates linker arguments and copies the DLLs you need
- **Nullable types** (`T?`) with mandatory null checks; FFI pointer returns are nullable automatically
- **Structs and classes** with methods, default fields, and struct literals
- **Functions** with default and named parameters, overloading, generics, and lambdas
- **Four string kinds:** regular, template, raw, and character literals
- **Cross-compilation** with `--target-os` and `--target-triple`

## Quick start

```bash
git clone https://github.com/0m0g1/omniscript.git
cd omniscript

./scripts/premake/premake5.exe gmake2
make config=release

./bin/Release-windows-x86_64/Osengine.exe examples/types.os --execute
```

`make.exe` and `premake5.exe` are bundled in `scripts/`, and LLVM headers and libraries live in `dependencies/llvm/`. The output folder follows `bin/{config}-{system}-{arch}`. See [Getting Started](docs/gettingstarted.md) and [Build Instructions](docs/buildinstructions.md) for details, flags, and troubleshooting.

## Documentation

| | |
|---|---|
| [Introduction](docs/intro.md) | Execution models, JIT vs AOT, FFI overview |
| [Getting Started](docs/gettingstarted.md) | Install, build, first program |
| [Build Instructions](docs/buildinstructions.md) | Building the compiler, CLI reference |
| [Syntax](docs/syntax.md) | Comments, literals, keywords, statements |
| [Data Types](docs/datatypes.md) | Integers, floats, arrays, pointers, structs, enums |
| [Operations](docs/operations.md) | Operators, casts, typing rules |
| [Control Flow](docs/controlflow.md) | Conditionals and loops |
| [Functions](docs/functions.md) | Parameters, overloading, generics, lambdas |
| [Classes and Structs](docs/classesandstructs.md) | Methods, constructors, access control |
| [Strings](docs/strings.md) | Regular, template, raw strings, chars |
| [Optionals](docs/optionals.md) | Nullable types |
| [External Bindings](docs/externalbindings.md) · [FFI](docs/ffi.md) | Calling C libraries |
| [Compiler](docs/Compiler.md) · [SymbolTable](docs/SymbolTable.md) | Compiler internals |

## Editor support

Install the [OmniScript Language Server](https://github.com/0m0g1/omniscript-language-server) for syntax highlighting, completion, and error detection. In VS Code, run **Developer: Install Extension from Location** and select the cloned folder.

## Status

OmniScript is a solo project. Expect missing features, rough edges, and breaking changes. Windows is the only platform tested regularly.

## Feedback

Bug reports, ideas, and discussion are welcome. Please open an issue on GitHub.

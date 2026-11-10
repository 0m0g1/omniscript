# OS Programming Language

Welcome to **OS** (to be renamed), a powerful and flexible systems programming language designed for performance and interoperability. OS combines the low-level control of C/C++ with a modern, concise syntax, making it ideal for building high-performance applications. With support for both Just-In-Time (JIT) and Ahead-Of-Time (AOT) compilation via LLVM, and a robust Foreign Function Interface (FFI), OS enables seamless integration with existing C libraries and system APIs. This introductory guide showcases OS's basic syntax, execution models, compilation options, and library integration through simple examples.

## Hello World Example

OS supports three execution models: **top-down**, **main-function-based**, and **custom entry point**. Programs can be run using the JIT compiler for immediate execution or compiled into standalone executables for distribution. Below are examples demonstrating these approaches with a "Hello, World!" program.

### Top-Down Style
In the top-down style, code is executed sequentially from the top of the file, similar to scripting languages like Python or JavaScript.

```os
extern "C" fn printf(fmt: char*, ...) => int;

printf("Hello, OS!\n");
```

### Main Function Style
In the main function style, a `main` function serves as the entry point, and any top-level code is executed at the start of `main`. This is similar to C/C++.

```os
extern "C" fn printf(fmt: char*, ...) => int;

function main() => i32 {
    printf("Hello, OS!\n");
    return 0;
}
```

### Custom Entry Point Style
In the custom entry point style, any function can be designated as the entry point using the `--entry=functionName` compiler argument. The function can have any name and return type.

```os
extern "C" fn printf(fmt: char*, ...) => int;

function customStart() => void {
    printf("Hello, OS! (Custom Entry Point)\n");
}
```

Compile and run with:
```bash
./path/to/Osengine.exe program.os --entry=customStart
```

## Running OS Programs

OS programs can be executed in two primary ways, leveraging its LLVM-based compiler:

1. **JIT Execution**: Run programs directly using the JIT compiler for immediate execution, ideal for development and testing. For example:
   ```bash
   ./path/to/Osengine.exe docs/examples/opengl/shaders/starlikeglad.os
   ```
   This command executes the `starlikeglad.os` script using the JIT compiler, targeting the native architecture with `-O3` optimization.

2. **AOT Compilation**: Generate standalone executables for distribution using the `--make` flag. For example:
   ```bash
   ./path/to/Osengine.exe docs/examples/opengl/shaders/starlikeglad.os --make -o myappname.exe
   ```
   This command compiles `starlikeglad.os` into a standalone executable named `myappname.exe`, optimized for portability like C++ binaries.

## Library Integration

OS's powerful Foreign Function Interface (FFI) allows it to use almost any C-based library that provides static (`.a`, `.lib`, etc) or dynamic (`.dll`, `.so`, `.dll`, `.wa`) library files. This makes OS highly interoperable with existing ecosystems, enabling developers to leverage libraries written for C/C++ and other languages that expose C-compatible interfaces. In the future, OS plans to support libraries from other languages if necessary, further expanding its interoperability.

### Example: Using SDL2
The following example demonstrates how to use the SDL2 library to create a window and handle basic events. SDL2 is a popular cross-platform library for graphics and input handling, commonly used in game development.

```os
extern "C" {
    fn printf(fmt: char*, ...) => int;
}

extern "dependencies/SDL2/lib/SDL2.dll", "dependencies/SDL2/lib/libSDL2.a" {
    fn SDL_Init(flags: uint) => int;
    fn SDL_CreateWindow(title: char*, x: int, y: int, w: int, h: int, flags: uint) => void*;
    fn SDL_PollEvent(event: void*) => int;
    fn SDL_DestroyWindow(window: void*) => void;
    fn SDL_Quit() => void;
}

const SDL_INIT_VIDEO: uint = 0x00000020;
const SDL_WINDOW_SHOWN: uint = 0x00000004;

function main() => i32 {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed\n");
        return 1;
    }

    let window = SDL_CreateWindow("OS SDL2 Example", 100, 100, 800, 600, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        printf("SDL_CreateWindow failed\n");
        SDL_Quit();
        return 1;
    }

    let running: int = 1;
    let event: void*;
    while (running) {
        while (SDL_PollEvent(&event)) {
            // Handle events (e.g., close window)
            // Note: Simplified for brevity; real applications would check event types
            running = 0;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("SDL2 window closed\n");
    return 0;
}
```

Run with:
```bash
./path/to/Osengine.exe sdl_example.os
```

Or compile to an executable:
```bash
./path/to/Osengine.exe sdl_example.os --make -o sdl_example.exe
```

This example shows how OS can link to SDL2's dynamic (`.dll`) and static (`.a`) libraries, initialize a window, and handle events, demonstrating its ability to integrate with widely used C libraries.

## Explanation

OS is designed to be flexible and lightweight, offering developers three execution models and versatile compilation options:

1. **Top-Down Execution**: Code runs sequentially from the top of the file, ideal for quick scripts or prototyping. This model is intuitive for developers familiar with Python or JavaScript.
2. **Main Function Execution**: If a `main` function is defined, it becomes the program's entry point, and any top-level code is executed at the start of `main`. This model aligns with C/C++ conventions and is suited for structured systems programming.
3. **Custom Entry Point Execution**: If a custom entry point with any name and return type is defined and specified with the `--entry=functionName` compiler argument, it will be executed. This provides maximum flexibility for defining program entry points, accommodating diverse use cases.

### Key Features
- **Foreign Function Interface (FFI)**: OS's FFI is a cornerstone of its design, enabling integration with virtually any C-based library that provides static (`.a`) or dynamic (`.dll`, `.so`) files. For example, libraries like SDL2, GLFW, or OpenGL can be used seamlessly by declaring their functions with `extern "path/to/lib"`. The FFI syntax is concise and platform-aware: [more about the ffi](./ffi.md).

  ```os
  extern "C" fn printf(fmt: char*, ...) => int;
  ```
  On Windows, this implicitly links to `msvcrt.dll` (e.g., `extern "C:/Windows/System32/msvcrt.dll"`). On Linux and macOS, it resolves to `libc.so` or `libSystem.dylib`. Similarly, libraries like SDL2 can be linked by specifying their paths:
  ```os
  extern "dependencies/SDL2/lib/SDL2.dll", "dependencies/SDL2/lib/libSDL2.a" { ... }
  ```

- **LLVM Backend**: OS uses LLVM for both JIT and AOT compilation, with `-O3` optimization enabled by default. JIT targets the native architecture for maximum performance, while AOT generates portable binaries, similar to C++.

- **Lightweight Design**: OS prioritizes simplicity and performance, so its freestanding by default and has no runtime. 

- **StandardLibrary**: OS will have a large standard library but it can leverages the FFI to access system and third-party libraries, since its still in early development.

### Why Choose OS?
OS is designed for developers who need low-level control without the verbosity of C++. Its clean syntax, combined with LLVM's optimizations, a robust FFI, and flexible execution models, makes it an excellent choice for systems programming, graphics, and cross-platform development. The ability to use almost any C-based library, run programs via JIT, or create standalone executables ensures OS can handle a wide range of use cases, from quick scripts to complex applications.

## Next Steps

To learn more about OmniScript, explore the following topics:

- [**FFI Details**](./ffi.md): How to link and use external libraries (e.g., GLFW, OpenGL, SDL2)
- [**Syntax**](./syntax.md): An introduction to OS's syntax; 
- [**Compiler**](./Compiler.md): Compiler.
- [**SymbolTable**](./SymbolTable.md): SymbolTable. 
- [**Datatypes**](./datatypes.md): Datatypes in OS. 
- [**The 4 Strings**](./strings.md): strings in OS. 
- [**Optionals**](./optionals.md): Optional/NullableTypes in OS. 
- [**Operations**](./operations.md): Operations in OS. 
- [**Control Flow**](./operations.md): Control flow operations in OS. 
- [**Functions**](./functions.md): An introduction to Functions in OS. 
- [**Classes and Structs**](./classesandstructs.md): An introduction to Classes and structs.
- [**Threading**](./threading.md): Using OmniScript's low-level threading APIs for concurrent programming  
- [**Examples Scripts**](): Examples
- [**Graphics Programming**](): Creating real-time graphics with OmniScript and libraries like OpenGL or SDL2

Stay tuned for more documentation as OS evolves!
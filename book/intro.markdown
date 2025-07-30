# Crafting the OS Language with LLVM: JIT and AOT Compilation

The OS language is a modern, systems-oriented programming language designed with a focus on performance, safety, and expressiveness. This article explores the process of building a Just-In-Time (JIT) and Ahead-Of-Time (AOT) compiler for OS using LLVM, a powerful and flexible compiler infrastructure. We'll discuss the language's syntax, its unique features, and the steps to implement its compilation pipeline. The OS language examples provided include vector operations, sprite-based scene management, dynamic arrays, and a threaded OpenGL starfield visualization, showcasing its capabilities.

## Overview of the OS Language

OS is designed to be a high-performance language with modern features like smart pointers (`unique_ptr`, `shared_ptr`, `weak_ptr`), capability-based method invocation, operator overloading, and seamless integration with external libraries like GLFW and OpenGL. Its syntax is clean and expressive, supporting structs, constructors, destructors, and dynamic polymorphism through capability methods (denoted by `can`). The language also supports threading and foreign function interfaces (FFI) for interoperability with C libraries.

### Key Features of OS
- **Smart Pointers**: Automatic memory management with `unique_ptr`, `shared_ptr`, and `weak_ptr` for safe resource handling.
- **Capability Methods**: Methods prefixed with `can` allow for dynamic dispatch and flexible behavior (e.g., `Sprite can move()`).
- **Operator Overloading**: Intuitive syntax for operations like `+` and `[]` (e.g., `Vec2 can +`, `DynArray can []`).
- **FFI Support**: Integration with C libraries like GLFW and GLAD for graphics and system calls.
- **Threading**: Support for multithreading via bindings to system APIs like `CreateThread`.
- **Flexible Data Structures**: Generic `DynArray<T>` for dynamic arrays with smart pointer-based memory management.

The provided OS code includes examples like `Vec2` and `Vec3` for vector math, `Sprite` and `Scene` for game-like functionality, `DynArray<T>` for generic collections, and a threaded OpenGL starfield visualization, which we’ll use to guide the LLVM-based compiler design.

## Why LLVM?

LLVM is an ideal choice for compiling OS due to its modular architecture, robust optimization passes, and support for both JIT and AOT compilation. LLVM’s intermediate representation (IR) is a portable, high-level assembly language that abstracts away machine-specific details, making it easier to target multiple platforms. Additionally, LLVM provides:
- **Frontend Flexibility**: OS’s custom syntax can be parsed and converted to LLVM IR.
- **Optimization**: LLVM’s optimization passes (e.g., inlining, loop unrolling) ensure high-performance code.
- **Backend Support**: Generate native code for various architectures (x86, ARM, etc.).
- **JIT and AOT**: Support for both dynamic JIT compilation and static AOT compilation.

## Designing the OS Compiler

To build a compiler for OS using LLVM, we need to implement a frontend, an IR generator, and a backend. The compiler must handle OS’s unique features like smart pointers, capability methods, and FFI. Below, we outline the steps for both JIT and AOT compilation.

### 1. Parsing and Abstract Syntax Tree (AST)

The first step is to parse OS source code into an AST. OS’s syntax is C-like with additional constructs like `can` for capabilities and smart pointer declarations. We can use a parser generator like ANTLR or a hand-written recursive descent parser to process the syntax.

#### Example Syntax Parsing
For the `Vec2` struct:
```os
struct Vec2 {
    x: float;
    y: float;

    Vec2 can + (other: Vec2) => Vec2 {
        return Vec2 { x: this.x + other.x, y: this.y + other.y };
    }
}
```
The parser would:
- Recognize `struct` as a type definition.
- Parse fields (`x: float`, `y: float`).
- Identify the `can +` capability as a method with operator overloading.
- Build an AST node for the method body, capturing the return expression and arithmetic operations.

The AST nodes would include:
- `StructDecl`: For `Vec2`, `Vec3`, `Sprite`, etc.
- `FieldDecl`: For fields like `x`, `y`, `position`.
- `MethodDecl`: For capability methods like `can +`, `can move`.
- `Expr`: For expressions like `this.x + other.x`.

### 2. Semantic Analysis

Semantic analysis ensures type safety, resolves symbols, and validates capability usage. For example:
- Verify that `this.x + other.x` in `Vec2 can +` uses compatible types (`float`).
- Check that smart pointers (`unique_ptr`, `shared_ptr`) are used correctly (e.g., `move` semantics for `unique_ptr`).
- Resolve FFI declarations (e.g., `glfwInit`) to their corresponding C functions.

For the `Scene` struct’s `add_sprite` method, the compiler must ensure that the `unique_ptr<Sprite[]>` is properly resized and that `move` operations maintain ownership semantics.

### 3. LLVM IR Generation

Once the AST is validated, we generate LLVM IR. LLVM’s C++ API (`llvm::IRBuilder`) is used to construct IR for OS constructs. Below, we outline how key features are translated.

#### Structs and Fields
For `struct Vec2`:
```cpp
// LLVM IR for Vec2
%struct.Vec2 = type { float, float }
```
- Create an LLVM `StructType` for `Vec2` with two `float` fields.
- For methods like `Vec2 can +`, generate a function that takes a `Vec2*` (for `this`) and a `Vec2` parameter, returning a new `Vec2`.

#### Smart Pointers
Smart pointers require special handling:
- **unique_ptr**: Represent as a pointer with ownership semantics. Use LLVM’s `malloc` and `free` for allocation/deallocation, ensuring `move` operations nullify the source pointer.
- **shared_ptr**: Implement reference counting. Each `shared_ptr` assignment increments a counter, and deletion decrements it, freeing memory when the count reaches zero.
- **weak_ptr**: Store a pointer to the reference count without incrementing it, with a `lock` method to create a `shared_ptr` if the resource is still alive.

For example, in `DynArray<T>`’s constructor:
```os
this.first = unique_array<T>(this.capacity);
```
Generate IR to:
- Call `malloc` to allocate an array of type `T` with size `this.capacity`.
- Store the pointer in `this.first`.
- Ensure automatic cleanup in the destructor by generating a `free` call.

#### Capability Methods
Capability methods (`can`) are translated as virtual functions or function pointers, depending on whether dynamic dispatch is needed. For `Sprite can move`:
```cpp
; LLVM IR for Sprite::move
define void @Sprite_move(%struct.Sprite* %this) {
    %position = getelementptr inbounds %struct.Sprite, %struct.Sprite* %this, i32 0, i32 0
    %velocity = getelementptr inbounds %struct.Sprite, %struct.Sprite* %this, i32 0, i32 1
    ; Load and add position + velocity
    ...
}
```
- Use `getelementptr` to access `position` and `velocity`.
- Call the `+` operator for `Vec2` to compute the new position.

#### FFI and External Libraries
For FFI declarations like `glfwInit`:
```os
extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll" {
    fn glfwInit() => int;
}
```
- Declare an LLVM function with external linkage:
```cpp
declare i32 @glfwInit()
```
- Link against the specified library (`glfw3.dll`) during AOT compilation or dynamically load it for JIT.

#### Threading
For the threaded starfield example:
```os
g_renderThread = CreateThread(nullptr, 0, renderThreadFunction, nullptr, 0, &threadId);
```
- Map `CreateThread` to an LLVM external function call.
- Ensure thread-safe access to globals like `g_shouldExit` using atomic operations (`atomicrmw` in LLVM IR).

### 4. JIT Compilation

For JIT compilation, we use LLVM’s `ExecutionEngine` to execute OS code dynamically. The process involves:
1. **Parse and Generate IR**: Convert the OS code (e.g., the starfield example) to LLVM IR.
2. **Optimize IR**: Apply LLVM optimization passes (e.g., `-O3`, inlining, vectorization).
3. **Create Execution Engine**: Use `MCJIT` or `OrcJIT` to compile IR to machine code in memory.
4. **Execute**: Call the `main` function or other entry points.

For the starfield example, the JIT compiler would:
- Load GLFW and GLAD dynamically using `dlopen` (or equivalent).
- Execute `renderThreadFunction` in a separate thread, rendering the colorful starfield.
- Handle dynamic updates to uniforms like `u_time` and `u_resolution`.

The OS compiler command for JIT mode is:
```
./path/to/Osengine.exe docs/examples/opengl/shaders/starlikeglad.os
```

### 5. AOT Compilation

For AOT compilation, we generate a standalone executable:
1. **Generate IR**: Same as JIT, but target a specific architecture (e.g., x86_64).
2. **Optimize IR**: Apply aggressive optimizations for performance.
3. **Generate Object Code**: Use LLVM’s backend to produce object code.
4. **Link**: Use a system linker (e.g., `ld`) to link against libraries like `glfw3.dll` and `glad_gl.dll`.
5. **Output Executable**: Produce `app.exe`.

The OS compiler command for AOT mode is:
```
./path/to/Osengine.exe docs/examples/opengl/shaders/starlikeglad.os --make -o app.exe
```

### 6. Linking External Libraries

OS’s FFI system specifies library paths explicitly:
```os
extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll" {
    fn glfwInit() => int;
}
```
- For AOT, the linker flags are generated automatically based on the provided paths (e.g., `-lglfw3`).
- For JIT, dynamically load libraries using `dlopen` and resolve symbols with `dlsym`.

### 7. Challenges and Solutions

- **Smart Pointers**: Implementing reference counting for `shared_ptr` requires careful IR generation to avoid leaks. Use LLVM’s `atomicrmw` for thread-safe reference counting.
- **Capability Methods**: Dynamic dispatch for `can` methods can be implemented using function pointers or a virtual table, depending on performance needs.
- **Threading**: Ensure thread safety for globals like `g_window` and `g_shouldExit` using LLVM’s atomic instructions.
- **FFI**: Validate library paths and ensure compatibility with the target platform (e.g., Windows for `kernel32.dll`).
- **Error Handling**: For shader compilation in the starfield example, propagate errors (e.g., `glGetShaderInfoLog`) to the OS runtime.

### 8. Example: Compiling the Starfield

The starfield example demonstrates OS’s capabilities:
- **Graphics**: Uses GLFW and GLAD for OpenGL rendering.
- **Threading**: Separates rendering (`renderThreadFunction`) from event handling (`main`).
- **Shaders**: Defines vertex and fragment shaders for a colorful starfield with nebula effects.

The compiler must:
- Generate IR for the shader compilation functions (`compileShader`, `createShaderProgram`).
- Handle the fullscreen quad rendering (`glDrawArrays`).
- Manage thread creation and synchronization (`CreateThread`, `WaitForSingleObject`).

### 9. Future Improvements

- **Better Optimization**: Leverage LLVM’s advanced passes like loop vectorization for `DynArray` operations.
- **Cross-Platform Support**: Extend the backend to target ARM, macOS, and Linux.
- **Debugging**: Integrate LLVM’s debug metadata for better debugging support.
- **Standard Library**: Develop a standard library for OS with common utilities (e.g., math, I/O).

## Conclusion

Building a JIT and AOT compiler for OS using LLVM is a complex but rewarding task. By leveraging LLVM’s infrastructure, we can translate OS’s expressive syntax—smart pointers, capability methods, FFI, and threading—into efficient machine code. The provided examples, like the threaded starfield visualization, demonstrate OS’s potential for high-performance applications. As OS evolves, the compiler can be extended to support new features, optimize performance, and target diverse platforms.
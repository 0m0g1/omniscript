# OS Programming Language Documentation

⚠️ **Early Development Notice**  
This project is in **early development** and not ready for general use. APIs and behavior are subject to change without notice.

## Overview

**OS** (formerly OmniScript) is a modern systems programming language that combines the performance of compiled languages with the flexibility of just-in-time compilation. It supports both ahead-of-time (AOT) and just-in-time (JIT) compilation modes.

## Key Features

- **Dual Compilation Modes**: Both AOT and JIT compilation support
- **Flexible Execution**: Top-down execution or main function entry points
- **Modern Syntax**: Clean, expressive syntax with type safety
- **Memory Management**: Manual memory management with pointer and reference support
- **Interoperability**: Seamless C library integration via `extern` declarations
- **Generics**: Template-like generic programming support
- **Object-Oriented**: Classes and structs with methods and constructors

## Language Characteristics

### Compilation Modes
- **AOT (Ahead-of-Time)**: Compile to native machine code before execution
- **JIT (Just-in-Time)**: Compile during runtime for maximum flexibility

### Execution Models
- **Top-down**: Execute code from top to bottom without a main function
- **Main function**: Traditional entry point with `main()` function

### Supported Main Function Signatures
```os
function main() => i32 {
    return 0;
}

function main(argc: int) => i32 {
    return 0;
}

function main(argc: int, argv: char**) => i32 {
    return 0;
}
```

## Getting Started

### Installation
Refer to the [Build Instructions](build-instructions.md) for setting up the OS compiler and runtime.

### Your First Program
```os
// Simple top-down execution
let greeting: char* = "Hello, OS!";
printf("%s\n", greeting);
```

### With Main Function
```os
extern "C" {
    fn printf(fmt: char*, ...) => int;
}

function main() => i32 {
    printf("Hello, OS!\n");
    return 0;
}
```

## Documentation Structure

- [Language Basics](language-basics.md) - Core language concepts
- [Type System](type-system.md) - Data types and type annotations
- [Functions](functions.md) - Function definitions, parameters, and generics
- [Classes and Structs](classes-structs.md) - Object-oriented programming
- [Memory Management](memory-management.md) - Pointers, references, and allocation
- [External Bindings](external-bindings.md) - C library integration
- [Control Flow](control-flow.md) - Loops, conditionals, and branching
- [Examples](examples.md) - Complete code examples
- [Build Instructions](build-instructions.md) - Compilation and toolchain setup

## Community and Feedback

This language is in active development. Feedback, ideas, and discussion are very welcome!

- **GitHub Repository**: [OS Language](https://github.com/0m0g1/omniscript)
- **Language Server**: [OmniScript Language Server](https://github.com/0m0g1/omniscript-language-server)
- **Temporary Documentation**: [Current docs](https://github.com/0m0g1/omniscript/blob/main/docs/temp.md)

## Syntax Highlighting

To enable syntax highlighting in VSCode:

1. Download the [OmniScript Language Server](https://github.com/0m0g1/omniscript-language-server)
2. Open VSCode
3. Run: `Developer: Install Extension from Location`
4. Select the downloaded folder

---

*This documentation is for OS language version in development. Features and syntax may change.*
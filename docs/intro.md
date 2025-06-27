# OS Programming Language

⚠️ **Early Development Notice**  
This project is in **early development** and not ready for general use. APIs and behavior are subject to change without notice.

**OS** (formerly OmniScript) is a modern systems programming language designed for performance and safety. It combines the control of C++ with modern language features and a clean syntax.

## Key Features

- **Strong static typing** with type inference
- **Memory safety** with explicit memory management
- **Zero-cost abstractions**
- **C interoperability** through extern declarations
- **Object-oriented programming** with classes and structs
- **Generic programming** with templates
- **Cross-platform** compilation targeting multiple architectures

## Quick Start

### Prerequisites

- No external downloads required
- All necessary tools (`make.exe` and `premake5.exe`) are included in the `scripts` folder

### Build Instructions

1. **Generate Makefiles:**
   ```bash
   ./scripts/premake/premake5.exe gmake2
   ```

2. **Clean Build (if needed):**
   ```bash
   make clean
   ```

3. **Compile:**
   ```bash
   # Debug mode
   make config=debug
   
   # Release mode
   make config=release
   ```

### Running Programs

```bash
# Debug mode
.\bin\Debug-windows-x86_64\Osengine.exe .\examples\types.os --execute --debug

# Release mode
.\bin\Release-windows-x86_64\Osengine.exe .\examples\types.os --execute
```

> **Note:** Output folder format: `bin/{config}-{system}-{arch}`

## Documentation

- [Getting Started](docs/getting-started.md)
- [Language Syntax](docs/syntax.md)
- [Data Types](docs/types.md)
- [Functions](docs/functions.md)
- [Control Flow](docs/control-flow.md)
- [Object-Oriented Programming](docs/oop.md)
- [Memory Management](docs/memory.md)
- [External Function Interface](docs/ffi.md)
- [Build System](docs/build-system.md)
- [Examples](docs/examples.md)

## IDE Support

Install the [OmniScript Language Server](https://github.com/0m0g1/omniscript-language-server) for syntax highlighting and IntelliSense in VSCode.

In VSCode, run: `Developer: Install Extension from Location` and select the downloaded folder.

## Hello World Example

```os
extern "C" {
    fn printf(fmt: char*, ...) => int;
}

function main() => i32 {
    printf("Hello, OS!\n");
    return 0;
}
```

## Contributing

Feedback, ideas, and discussion are very welcome! This language is in active development, and community input helps shape its future.

## Links

- [Temporary Documentation](https://github.com/0m0g1/omniscript/blob/main/docs/temp.md)
- [Language Server](https://github.com/0m0g1/omniscript-language-server)
- [Main Repository](https://github.com/0m0g1/omniscript)

## License

[License information to be added]
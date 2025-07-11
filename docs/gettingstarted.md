# Getting Started with OS

This guide will help you set up and start programming with the OS language.

## Prerequisites

The OS language build system is self-contained:
- **No external downloads required**
- All necessary tools (`make.exe` and `premake5.exe`) are included in the `scripts` folder
- Tools can be used and installed directly from the project directory

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/0m0g1/omniscript.git
   cd omniscript
   ```

## Build Process

### 1. Generate Makefiles

Run this when the Lua project script changes or when a new C++ file is added:

```bash
./scripts/premake/premake5.exe gmake2
```

### 2. Clean Build (Optional)

Run only if any C++ file is moved to a different location:

```bash
make clean
```

### 3. Compile

Choose your build configuration:

**Debug Mode:**
```bash
make config=debug
```

**Release Mode:**
```bash
make config=release
```

## Running Programs

After compiling, you can run OS programs using the engine:

**Debug Mode:**
```bash
.\bin\Debug-windows-x86_64\Osengine.exe .\examples\types.os --execute --debug
```

**Release Mode:**
```bash
.\bin\Release-windows-x86_64\Osengine.exe .\examples\types.os --execute
```

### Output Directory Structure

The output folder depends on your system and architecture:
- Format: `bin/{config}-{system}-{arch}`
- Example: `Debug-windows-x86_64` on a 64-bit Windows system

## IDE Setup

### VSCode Support

1. Install the [OmniScript Language Server](https://github.com/0m0g1/omniscript-language-server)
2. Download the language server extension
3. In VSCode, run: `Developer: Install Extension from Location`
4. Select the downloaded folder to complete installation

This provides:
- Syntax highlighting
- IntelliSense
- Error detection
- Code completion

## Your First Program

Create a file called `hello.os`:

```os
extern "C" fn printf(fmt: char*, ...) => int;

printf("Hello, OS!\n")
```

Run it:
```bash
.\bin\Release-windows-x86_64\Osengine.exe hello.os --execute
```

## Project Structure

A typical OS project looks like this:

```
project/
├── src/
│   ├── main.os           # Main source file
│   └── modules/          # Additional modules
├── examples/
│   └── types.os          # Example programs
├── scripts/
│   └── premake/
│       └── premake5.exe  # Build tool
├── dependencies/
│   └── llvm/             # LLVM backend
│       ├── include/
│       └── lib/
└── bin/                  # Compiled output
    └── {config}-{system}-{arch}/
```

## Build System Notes

### Windows-Specific
- The `premake5.lua` script automatically copies required LLVM `.dll` files to the build output folder
- Ensure LLVM headers and libraries are correctly placed in `dependencies/llvm/include` and `dependencies/llvm/lib`

### Cross-Platform
OS supports compilation for multiple platforms and architectures through the LLVM backend.

## Next Steps

Now that you have OS set up, explore these topics:

1. [Language Syntax](syntax.md) - Learn the basic syntax
2. [Data Types](types.md) - Understand OS's type system
3. [Functions](functions.md) - Write reusable code
4. [Examples](examples.md) - See practical programs

## Troubleshooting

### Common Issues

**Build fails with "premake5.exe not found":**
- Ensure you're running the command from the project root directory
- Check that `scripts/premake/premake5.exe` exists

**Engine fails to run:**
- Check that the binary was compiled successfully
- Verify the path to the binary matches your system architecture
- Ensure LLVM DLLs are in the output directory (Windows)

**Missing LLVM dependencies:**
- Verify `dependencies/llvm/include` and `dependencies/llvm/lib` contain the required files
- Re-run the build process after fixing dependencies

### Getting Help

- Check the [examples](examples.md) for working code samples
- Review the [temporary documentation](https://github.com/0m0g1/omniscript/blob/main/docs/temp.md)
- Open an issue on the GitHub repository for bugs or questions
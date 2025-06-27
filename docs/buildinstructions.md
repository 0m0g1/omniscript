# Build Instructions

This guide covers how to build the OS language compiler and runtime from source code.

⚠️ **Early Development Notice**  
This project is in **early development** and not ready for general use. APIs and behavior are subject to change without notice.

## Prerequisites

The OS language build system is designed to be self-contained with minimal external dependencies.

### Required Tools (Included)
- **No external downloads required**
- All necessary tools are included in the `scripts` folder:
  - `make.exe` - Build automation tool
  - `premake5.exe` - Project file generator

### System Requirements
- **Windows**: Windows 10 or later (primary development platform)
- **Architecture**: x86_64 (64-bit)
- **Memory**: At least 4GB RAM recommended for compilation
- **Disk Space**: ~500MB for full build with dependencies

### Dependencies (Included)
- **LLVM**: Headers and libraries included in `dependencies/llvm/`
  - Headers: `dependencies/llvm/include`
  - Libraries: `dependencies/llvm/lib`
- **Additional libraries** as needed for specific features

## Build Process

### Step 1: Generate Makefiles

Generate the build system files using Premake5. Run this command whenever:
- The Lua project script (`premake5.lua`) is changed
- New C++ source files are added to the project
- Build configuration is modified

```bash
./scripts/premake/premake5.exe gmake2
```

This command creates Makefiles for the GNU Make build system.

### Step 2: Clean Build (Optional)

Run a clean build only when:
- C++ files have been moved to different locations
- Major structural changes have been made
- You want to ensure a completely fresh build

```bash
make clean
```

### Step 3: Compile the Project

#### Debug Mode Compilation
For development and debugging purposes:

```bash
make config=debug
```

Debug builds include:
- Debug symbols for debugging
- Runtime checks and assertions
- Verbose error messages
- Slower execution but better error reporting

#### Release Mode Compilation
For production and performance testing:

```bash
make config=release
```

Release builds include:
- Optimized code for better performance
- Minimal debug information
- Stripped binaries for smaller size
- Maximum execution speed

## Running the Engine

After successful compilation, the OS engine executable will be located in the build output directory.

### Output Directory Structure

The build output follows this pattern:
```
bin/{config}-{system}-{arch}/
```

Examples:
- **Windows 64-bit Debug**: `bin/Debug-windows-x86_64/`
- **Windows 64-bit Release**: `bin/Release-windows-x86_64/`
- **Linux 64-bit Debug**: `bin/Debug-linux-x86_64/`

### Running Examples

#### Debug Mode
```bash
.\bin\Debug-windows-x86_64\Osengine.exe .\examples\types.os --execute --debug
```

Debug mode features:
- Detailed execution tracing
- Runtime type checking
- Memory allocation tracking
- Verbose error messages

#### Release Mode
```bash
.\bin\Release-windows-x86_64\Osengine.exe .\examples\types.os --execute
```

Release mode features:
- Optimized execution
- Minimal runtime overhead
- Production-ready performance

### Command Line Options

The OS engine supports various command-line options:

```bash
Osengine.exe [script_file] [options]
```

#### Available Options
- `--execute`: Execute the script immediately
- `--debug`: Enable debug mode with verbose output
- `--compile`: Compile to native code (AOT mode)
- `--jit`: Use just-in-time compilation
- `--help`: Show help information
- `--version`: Show version information

#### Examples
```bash
# Execute with JIT compilation
Osengine.exe script.os --execute --jit

# Compile to native executable
Osengine.exe script.os --compile --output=myprogram.exe

# Debug execution with tracing
Osengine.exe script.os --execute --debug --trace
```

## Project Structure

```
OS/
├── bin/                    # Build output directory
│   ├── Debug-windows-x86_64/
│   └── Release-windows-x86_64/
├── dependencies/           # External dependencies
│   └── llvm/
│       ├── include/        # LLVM headers
│       └── lib/           # LLVM libraries
├── examples/              # Example OS scripts
│   ├── types.os
│   ├── graphics.os
│   └── ...
├── scripts/               # Build tools
│   └── premake/
│       └── premake5.exe
├── src/                   # Source code
│   ├── compiler/
│   ├── runtime/
│   └── ...
├── premake5.lua          # Build configuration
└── Makefile              # Generated makefile
```

## Platform-Specific Notes

### Windows
- The `premake5.lua` script automatically copies required LLVM `.dll` files to the build output folder
- Visual Studio runtime libraries may be required for execution
- Windows Defender might flag the compiler during development

### Linux/Unix (Future Support)
- Additional dependencies may be required: `gcc`, `make`, `libc6-dev`
- LLVM libraries may need to be installed separately
- Use appropriate package manager for dependencies

### macOS (Future Support)
- Xcode command line tools required
- Homebrew recommended for dependency management
- LLVM may need to be installed via `brew install llvm`

## Troubleshooting

### Common Build Issues

#### LLVM Not Found
```
Error: LLVM headers not found in dependencies/llvm/include
```
**Solution**: Ensure LLVM headers and libraries are correctly placed in the `dependencies/llvm/` directory.

#### Premake5 Fails
```
Error: Access denied when running premake5.exe
```
**Solution**: Run command prompt as Administrator or check antivirus settings.

#### Make Command Not Found
```
'make' is not recognized as an internal or external command
```
**Solution**: Use the included `make.exe` from the scripts folder or install MinGW-w64.

#### Missing DLL Errors
```
Error: Cannot find LLVM DLL files
```
**Solution**: Ensure the build script has copied DLL files to the output directory, or copy them manually.

### Debug Build Issues

#### Slow Performance
- This is expected in debug builds
- Use release builds for performance testing
- Debug builds include extensive runtime checks

#### Memory Usage
- Debug builds use more memory for tracking
- Consider increasing system RAM or using release builds

### Release Build Issues

#### Missing Debug Information
- Release builds strip debug symbols
- Use debug builds for development and debugging
- Consider creating separate debug symbol files

## Development Workflow

### Recommended Development Process

1. **Make changes** to source code
2. **Run premake5** if project structure changed:
   ```bash
   ./scripts/premake/premake5.exe gmake2
   ```
3. **Build in debug mode** for testing:
   ```bash
   make config=debug
   ```
4. **Test with examples**:
   ```bash
   .\bin\Debug-windows-x86_64\Osengine.exe .\examples\types.os --execute --debug
   ```
5. **Build release version** when ready:
   ```bash
   make config=release
   ```

### Continuous Integration

For automated building:

```bash
# Full clean build
make clean
./scripts/premake/premake5.exe gmake2
make config=debug
make config=release

# Run tests
.\bin\Debug-windows-x86_64\Osengine.exe .\examples\test_suite.os --execute
```

## Advanced Build Options

### Custom Build Configurations

Modify `premake5.lua` to add custom build configurations:

```lua
configurations { "Debug", "Release", "Profile" }

filter "configurations:Profile"
    defines { "PROFILE_BUILD" }
    optimize "Speed"
    symbols "On"
```

### Compiler Flags

Add custom compiler flags in `premake5.lua`:

```lua
filter "system:windows"
    buildoptions { "/W4", "/WX" }  -- High warning level, warnings as errors

filter "system:linux"
    buildoptions { "-Wall", "-Wextra", "-Werror" }
```

## Performance Optimization

### Build Performance
- Use `make -j4` for parallel compilation (4 cores)
- Enable compiler caching if available
- Use SSD storage for faster I/O

### Runtime Performance
- Always use release builds for benchmarking
- Profile with appropriate tools
- Consider Link-Time Optimization (LTO) for final builds

---

*Build instructions are subject to change as the project evolves.*
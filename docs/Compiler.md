# Omniscript Compiler Documentation

## Overview

Omniscript is a high-performance, modular compiler designed for compiling and executing code in a flexible, target-agnostic manner. It supports both Just-In-Time (JIT) and Ahead-Of-Time (AOT) compilation modes, with features like parallel compilation, incremental caching, profiling, and robust error handling. The compiler is built with C++ and leverages LLVM for backend code generation, targeting multiple architectures (x86_64, ARM64, etc.) and operating systems (Linux, Windows, macOS, WebAssembly, etc.).

This documentation covers the Omniscript compiler's architecture, usage, and implementation details, focusing on the core components (`Compiler`, `Engine`, `Application`) and their features.

## Table of Contents

1. [Architecture](#architecture)
   - [Compiler](#compiler)
   - [Engine](#engine)
   - [Application](#application)
2. [Features](#features)
   - [Core Features](#core-features)
   - [Enhanced Features](#enhanced-features)
3. [Usage](#usage)
   - [Command-Line Interface](#command-line-interface)
   - [Configuration Options](#configuration-options)
   - [Examples](#examples)
4. [Implementation Details](#implementation-details)
   - [Compiler Implementation](#compiler-implementation)
   - [Engine Implementation](#engine-implementation)
   - [Application Implementation](#application-implementation)
   - [Utility Components](#utility-components)
5. [Building and Running](#building-and-running)
6. [Error Handling and Debugging](#error-handling-and-debugging)
7. [Performance and Profiling](#performance-and-profiling)
8. [Future Improvements](#future-improvements)

## Architecture

The Omniscript compiler is structured around three main classes: `Compiler`, `Engine`, and `Application`. Each class has a specific role, ensuring modularity and maintainability.

### Compiler

The `Compiler` class (`Compiler.h`, `Compiler.cpp`) is responsible for the core compilation process, transforming Omniscript source code into executable machine code or intermediate representations.

- **Responsibilities**:
  - Parses and compiles statements in JIT, AOT, or hybrid modes.
  - Supports parallel compilation of multiple source files or statement chunks.
  - Manages backend initialization (LLVM JIT and AOT backends).
  - Tracks compilation statistics (parse time, codegen time, link time, memory usage).
  - Handles incremental compilation with caching.
- **Key Methods**:
  - `compile`: Compiles statements sequentially, supporting JIT, AOT, or hybrid modes.
  - `compileParallel`: Distributes compilation across multiple threads for performance.
  - `validateTargetConfiguration`: Validates target architecture and OS settings.
  - `saveCache` / `loadCache`: Manages incremental compilation cache.
- **Thread Safety**: Uses `std::mutex` for state management and `std::atomic` for `busy_` and `cancelled_` flags, ensuring safe concurrent access.
- **Hybrid Mode**: Combines JIT and AOT compilation, executing code immediately via JIT while generating persistent artifacts (e.g., executables, libraries) via AOT. Useful for debugging JIT execution while producing AOT output for deployment.

### Engine

The `Engine` class (`Engine.h`, `Engine.cpp`) serves as the orchestrator, handling argument parsing, configuration, and execution flow.

- **Responsibilities**:
  - Parses command-line arguments into a `Config` object.
  - Reads and processes source code from files or stdin.
  - Coordinates compilation via the `Compiler` class or direct JIT execution.
  - Manages target configuration (architecture, OS, output format).
- **Key Methods**:
  - `parseArguments`: Processes command-line arguments into a `Config` object.
  - `run`: Executes the compilation or JIT execution pipeline.
  - `readSourceCode`: Reads source code from files or stdin.
  - `setDefaultOutputPath`: Determines output file extensions based on format (e.g., `.s`, `.ll`, `.wasm`).
- **File Path Handling**: Supports stdin input (`filePath == "-"`) and issues warnings when multiple file paths are provided without using `--source`, defaulting to processing all files.

### Application

The `Application` class (`Application.h`, `Application.cpp`) provides the entry point for the Omniscript compiler, managing global state and execution.

- **Responsibilities**:
  - Initializes console, signal handlers, and memory limits.
  - Runs the compilation pipeline via the `Engine` class.
  - Logs errors and profiler results.
- **Key Methods**:
  - `run`: Executes the application, returning an exit code.
  - `initializeGlobalState`: Sets up console and error collector.
  - `setupSignalHandlers`: Configures handlers for `SIGINT`, `SIGTERM`, and Linux-specific `SIGUSR1`, `SIGUSR2`.
  - `setupMemoryLimits`: Sets platform-specific memory (8GB) and stack (16MB) limits.

## Features

### Core Features

- **JIT and AOT Compilation**: Supports Just-In-Time (JIT) execution for immediate results and Ahead-Of-Time (AOT) compilation for persistent binaries.
- **Cross-Platform Support**: Targets multiple architectures (x86_64, ARM64, x86_32, ARM32, RISCV64, WASM32, WASM64) and operating systems (Linux, Windows, macOS, FreeBSD, Android, iOS, WebAssembly).
- **Command-Line Interface**: Extensive CLI options for configuration, debugging, and output control.
- **Error Handling**: Centralized error collection with severity levels (Info, Warning, Error, Fatal) and optional file logging.
- **Performance Monitoring**: Tracks compilation time, memory usage, and optimization metrics.

### Enhanced Features

- **Multi-File Compilation**: Supports multiple source files via `sourcePaths`, with `mainSourceFile` specifying the primary entry point. When multiple files are provided without `--source`, a warning is issued, and all files are processed.
- **Parallel Compilation**: Leverages `parallelJobs` to distribute compilation tasks across threads, using `std::async` for efficiency.
- **Incremental Caching**: Stores compilation artifacts in `cacheDirectory` to speed up subsequent builds.
- **Profiling Support**: Integrates profiling tools (e.g., `perf`, `gprof`) with output to `profiler.outputPath`.
- **Module System**: Enables modular code organization with `modules.enableModules` and `modulePaths`.
- **Hybrid Mode**: Runs both JIT and AOT backends simultaneously, allowing immediate execution and persistent artifact generation. Ideal for scenarios requiring both rapid prototyping and deployable binaries.
- **Advanced Error Handling**: Configurable error logging (`errorHandling.logToFile`, `errorHandling.errorLogPath`, `errorHandling.maxErrorCount`) for robust diagnostics.

## Usage

### Command-Line Interface

Run the Omniscript compiler using the command:

```bash
./omniscript [options] <file>
```

Use `-` as the file path to read from stdin. When multiple files are provided without `--source`, a warning is issued, but all files are processed.

### Configuration Options

| Option | Description | Example |
| --- | --- | --- |
| `--main-source <file>` | Specify the main source file | `--main-source main.osc` |
| `--source <file>` | Add additional source files | `--source lib.osc` |
| `--enable-modules <path>` | Enable module support with path | `--enable-modules ./modules` |
| `--incremental` | Enable incremental compilation | `--incremental` |
| `--cache-dir <dir>` | Set cache directory | `--cache-dir ./cache` |
| `--parallel-jobs <n>` | Set number of parallel jobs | `--parallel-jobs 4` |
| `--profiler <type>` | Enable profiling (e.g., `perf`, `gprof`) | `--profiler perf` |
| `--error-log <file>` | Log errors to a file | `--error-log errors.log` |
| `--max-errors <n>` | Set maximum error count | `--max-errors 10` |
| `--debug`, `-d` | Enable debug mode (shows detailed logs, performance stats) | `--debug` |
| `--execute` | Run in JIT mode | `--execute` |
| `--make` | Compile in AOT mode | `--make` |
| `--dry` | Perform dry compilation | `--dry` |
| `--entry <function>` | Set entry function | `--entry main` |
| `--output`, `-o <file>` | Set output file path | `-o output.bin` |
| `--keep-obj` | Keep intermediate object files | `--keep-obj` |
| `--log-asm` | Log generated assembly | `--log-asm` |
| `--log-final-code` | Log final IR code | `--log-final-code` |
| `--show-metadata` | Show metadata during compilation | `--show-metadata` |
| `--verbose` | Show parse tree and token stream in debug mode | `--verbose` |
| `--target-arch <arch>` | Target architecture (e.g., `x86_64`, `arm64`) | `--target-arch x86_64` |
| `--target-os <os>` | Target OS (e.g., `linux`, `windows`) | `--target-os linux` |
| `--target-triple <triple>` | Set target triple | `--target-triple x86_64-linux-gnu` |
| `--list-targets` | List available targets | `--list-targets` |
| `--show-host-info` | Display host system information (architecture, OS) and exit | `--show-host-info` |
| `--gc <strategy>` | Garbage collection strategy (e.g., `refcounting`, `marksweep`) | `--gc refcounting` |
| `--safety <level>` | Safety level (e.g., `standard`, `paranoid`) | `--safety standard` |
| `--enable-lto` | Enable Link Time Optimization | `--enable-lto` |
| `--enable-pgo` | Enable Profile Guided Optimization | `--enable-pgo` |
| `--enable-vectorization` | Enable vectorization | `--enable-vectorization` |
| `--enable-inlining` | Enable function inlining | `--enable-inlining` |
| `--enable-tail-calls` | Enable tail call optimization | `--enable-tail-calls` |
| `--fast-math` | Enable fast math optimizations | `--fast-math` |
| `--emit-staticlib` | Emit static library | `--emit-staticlib` |
| `--emit-sharedlib` | Emit shared library | `--emit-sharedlib` |
| `--emit-assembly` | Emit assembly code (`.s`) | `--emit-assembly` |
| `--emit-ir` | Emit LLVM IR (`.ll`) | `--emit-ir` |
| `--emit-object` | Emit object file | `--emit-object` |
| `--emit-bitcode` | Emit LLVM bitcode (`.bc`) | `--emit-bitcode` |
| `--emit-wasm` | Emit WebAssembly (`.wasm`) | `--emit-wasm` |
| `--enable-parallel-gc` | Enable parallel garbage collection | `--enable-parallel-gc` |
| `--enable-concurrent-gc` | Enable concurrent garbage collection | `--enable-concurrent-gc` |
| `--heap-size <size>` | Set heap size (e.g., `64MB`) | `--heap-size 64MB` |
| `--stack-size <size>` | Set stack size (e.g., `8MB`) | `--stack-size 8MB` |
| `--enable-stack-protection` | Enable stack protection | `--enable-stack-protection` |
| `--enable-cfi` | Enable Control Flow Integrity | `--enable-cfi` |
| `--enable-asan` | Enable AddressSanitizer | `--enable-asan` |
| `--enable-msan` | Enable MemorySanitizer | `--enable-msan` |
| `--enable-tsan` | Enable ThreadSanitizer | `--enable-tsan` |
| `--enable-ubsan` | Enable UndefinedBehaviorSanitizer | `--enable-ubsan` |
| `--enable-pic` | Enable Position Independent Code | `--enable-pic` |
| `--version` | Display version | `--version` |
| `--help` | Display help | `--help` |

### Examples

1. **Compile a single file in AOT mode**:
   ```bash
   ./omniscript --make --output program.bin main.osc
   ```

2. **Run in JIT mode with debugging**:
   ```bash
   ./omniscript --execute --debug main.osc
   ```

3. **Compile multiple files with parallel jobs**:
   ```bash
   ./omniscript --main-source main.osc --source lib1.osc --source lib2.osc --parallel-jobs 4 --make
   ```

4. **Enable profiling and error logging**:
   ```bash
   ./omniscript --profiler perf --error-log errors.log --max-errors 5 main.osc
   ```

5. **Read from stdin**:
   ```bash
   cat main.osc | ./omniscript --execute -
   ```

6. **Run in hybrid mode**:
   ```bash
   ./omniscript --execute --make --main-source main.osc --output program.bin
   ```

## Implementation Details

### Compiler Implementation

- **Files**: `Compiler.h`, `Compiler.cpp`
- **Key Features**:
  - Uses `LLVMJITBackend` and `LLVMAOTBackend` for code generation.
  - Supports parallel compilation via `std::async`, splitting statements or files across threads.
  - Implements RAII cleanup with `std::unique_ptr` for resource management.
  - Tracks memory usage with platform-specific functions (`mallinfo` on Linux, `GetProcessMemoryInfo` on Windows, `task_info` on macOS).
  - Provides detailed progress updates (e.g., "Initializing", "Parsing complete") via callbacks.
- **Performance Optimizations**:
  - Caches target configuration validation results in `validationCache_`.
  - Uses `std::mutex` and `std::atomic` for thread safety.
  - Reserves memory for `validationCache_` (64 entries) to reduce reallocations.

### Engine Implementation

- **Files**: `Engine.h`, `Engine.cpp`
- **Key Features**:
  - Parses command-line arguments into a `Config` struct, supporting over 30 options.
  - Reads source code from files or stdin, handling large files efficiently.
  - Coordinates compilation via `Compiler` or direct JIT execution with `LLVMJITBackend`.
  - Auto-configures target settings and validates configurations.
  - Sets default output paths based on input file and output format (e.g., `.s`, `.ll`, `.wasm`).
- **Restored Features**:
  - Extensive argument parsing for flags like `--verbose`, `--keep-obj`, `--log-asm`.
  - Console integration with `console.log`, `console.warn`, and `console.error`.
  - Support for stdin input and warnings for multiple file paths.

### Application Implementation

- **Files**: `Application.h`, `Application.cpp`
- **Key Features**:
  - Serves as the entry point, initializing global state (console, error collector).
  - Sets up signal handlers (`SIGINT`, `SIGTERM`, `SIGUSR1`, `SIGUSR2` on Linux).
  - Configures platform-specific memory limits (8GB memory, 16MB stack on Linux).
  - Logs errors to file if specified and displays profiler results.
- **Restored Features**:
  - Detailed performance statistics in debug mode (total time, parse time, compile time, memory usage).
  - Comprehensive signal handling for graceful shutdown.

### Utility Components

- **Namespaces**: `detail`, `perf`, `error` (assumed in `omniscript_pch.h`)
- **Key Utilities**:
  - `detail::StringInterner`: Interns strings for memory efficiency using a thread-safe `std::shared_mutex`.
  - `perf::ScopedTimer`: Measures execution time for profiling critical sections.
  - `perf::MemoryTracker`: Tracks memory usage with platform-specific implementations (e.g., `mallinfo` on Linux).
  - `error::ErrorCollector`: Collects errors with severity, message, context, timestamp, and thread ID.
  - `detail::fast_string_equal`: Performs fast string comparison using SIMD (SSE2) when available, falling back to standard comparison if SSE2 is not supported.

## Building and Running

1. **Dependencies**:
   - C++20 compiler (e.g., GCC, Clang)
   - LLVM libraries for JIT and AOT backends
   - Standard libraries for threading, filesystem, and platform-specific APIs

2. **Build Instructions**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

3. **Run Instructions**:
   ```bash
   ./omniscript [options] <file>
   ```

## Error Handling and Debugging

- **Error Collection**: Errors are stored in `error::globalErrorCollector` with severity levels (Info, Warning, Error, Fatal).
- **Logging**: Errors can be logged to a file using `--error-log <file>` and limited with `--max-errors <n>`.
- **Debug Mode**: Enable with `--debug` or `-d` to display detailed logs, performance statistics, and configuration details. Use `--verbose` to show parse tree and token stream, and `--show-metadata` to display compilation metadata.
- **Console Output**: Uses `console.log`, `console.warn`, and `console.error` for user feedback.

## Performance and Profiling

- **Profiling Tools**: Supports `perf` and `gprof` via `--profiler <type>`.
- **Metrics**: Tracks parse time, codegen time, link time, and memory usage.
- **Parallel Compilation**: Configurable with `--parallel-jobs <n>` to utilize multiple CPU cores.
- **Incremental Compilation**: Enabled with `--incremental` and `--cache-dir <dir>` to reduce rebuild times.

## Future Improvements

- **Language Syntax Documentation**: Add detailed syntax and semantics for the Omniscript language.
- **Advanced Optimizations**: Implement more aggressive optimizations (e.g., loop unrolling, advanced vectorization).
- **Module System Enhancements**: Support for dynamic module loading and dependency resolution.
- **Extended Profiling**: Integrate more profiling tools and detailed performance reports.
- **Cross-Compilation**: Improve support for cross-compilation in JIT mode.
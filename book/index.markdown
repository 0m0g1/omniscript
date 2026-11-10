# OmniScript++ Compiler Reference

## Overview
The *OmniScript++ Compiler Reference* is the definitive guide for building professional-grade compilers, using the OmniScript++ (OS) compiler as a central case study. Unlike introductory texts such as *Crafting Interpreters*, which focus on toy languages, this book targets experienced compiler developers. It covers advanced topics including parsing, semantic analysis, code generation with LLVM IR, optimization techniques, thread-safe error handling, memory optimization, and performance profiling. Each component of the OS compiler is documented in detail, providing practical insights into real-world compiler design.

The OS compiler, hosted at [https://github.com/0m0g1/omniscript](https://github.com/0m0g1/omniscript), is a work-in-progress programming language designed for robust, high-performance compilation. This book uses its components—such as `FileSpan` for source location tracking, `Console` for diagnostics, and `Core` for foundational utilities—to illustrate best practices in compiler architecture. Each component is presented with its source code, detailed explanations, and usage examples tied to OS language features like `Vec2`, `Sprite`, or the `starfield` example.

## Component Reference
Below is an alphabetical list of documented components in the OmniScript++ compiler. Each entry links to a dedicated Markdown file detailing the component’s purpose, implementation, and role in the compilation process.

| Component | Description | Link |
|-----------|-------------|------|
| Compiler | Orchestrates compilation, integrating parsing, code generation, and linking for JIT, AOT, and hybrid modes. | [Compiler](Compiler.md) |
| Console   | Logging and diagnostics for error reporting. | [Console](Console.md) |
| Core | Foundational utilities for string interning, performance profiling, and error handling. | [Core](Core.md) |
| Engine | Parses command-line arguments and orchestrates compilation for JIT, AOT, and hybrid modes. | [Engine](Engine.md) |
| EngineConfigs | Configures compilation modes, optimizations, runtime, and target-specific settings. | [EngineConfigs](EngineConfigs.md) |
| FileSpan  | Tracks source code locations for precise diagnostics. | [FileSpan](FileSpan.md) |
| TargetInfo | Manages target architectures, operating systems, and triples for cross-compilation. | [TargetInfo](TargetInfo.md) |

## Accessing the Source Code
The complete OmniScript++ compiler codebase is available at [https://github.com/0m0g1/omniscript](https://github.com/0m0g1/omniscript). Refer to the repository for the latest updates, build instructions, and example scripts. The repository’s `premake5.lua` script and included tools (`make.exe`, `premake5.exe`) simplify building the compiler on supported platforms.

## Contributing to the Documentation
As the OS compiler is in early development, feedback on both the compiler and this reference is welcome. Submit issues or pull requests to the GitHub repository to suggest improvements or report errors.
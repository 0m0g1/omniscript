# FileSpan: Source Code Position and Span Tracking for OmniScript++

The `FileSpan` module in the OmniScript++ (OS) compiler provides utilities for tracking source code positions and spans, enabling precise error reporting and debugging. This document covers both the declarations (from `FileSpan.h`) and definitions (from `core.cpp`) of the `filePosition` and `FileSpan` structures, as well as related global variables and functions. These components are essential for associating compiler diagnostics (e.g., syntax errors in OS code like `Vec2 can +`) with specific locations in source files, supporting professional-grade compiler development.

## Purpose
- **Source Position Tracking**: The `filePosition` structure records a single point in the source code (line, column, file path), used for pinpointing errors or warnings.
- **Source Span Management**: The `FileSpan` structure tracks a range of source code (from start to end positions), ideal for reporting issues that span multiple lines or columns, such as a malformed `Sprite` struct definition.
- **Global Span Management**: The `currentSpan` global variable and associated functions (`setSpan`, `getSpan`, `setSpanFromPosition`) allow the compiler to maintain and query the current source location during parsing, semantic analysis, or code generation.
- **Thread Synchronization**: The `allThreadsDone` flag supports multi-threaded compilation scenarios, such as the threaded starfield example in OS, ensuring proper cleanup after all threads complete.

## FileSpan.h Declarations

The `FileSpan.h` header declares the `filePosition` and `FileSpan` structures, along with global variables and functions for span management. Below is the complete header content:

```cpp
#pragma once

#include <string>

namespace Omniscript {

// Position structure
struct filePosition {
    int line = -1;
    int col = -1;
    std::string filePath;

    filePosition() = default;

    filePosition(int l, int c, const std::string& path)
        : line(l), col(c), filePath(path) {}

    std::string toString() const;
};

// Span structure for tracking ranges
struct FileSpan {
    filePosition start;
    filePosition end;

    FileSpan(int sLine = -1, int sCol = -1, int eLine = -1, int eCol = -1, const std::string& path = "")
        : start(sLine, sCol, path), end(eLine, eCol, path) {}

    bool isValid() const;
    void merge(const FileSpan& other);
    std::string toString() const;

private:
    static bool isBefore(const filePosition& a, const filePosition& b);
};

// Global span tracking
extern FileSpan currentSpan;

void setSpan(const filePosition& start, const filePosition& end);
void setSpan(const FileSpan& span);
void setSpan(int startLine, int startCol, int endLine, int endCol, const std::string& path);
FileSpan getSpan();
void setSpanFromPosition(int line, int column, const std::string& path);

extern bool allThreadsDone;

}  // namespace Omniscript
```

### Declaration Details
- **filePosition**:
  - **Fields**: `line` (int, default -1), `col` (int, default -1), `filePath` (std::string).
  - **Constructors**:
    - Default constructor initializes `line` and `col` to -1, `filePath` to empty.
    - Parameterized constructor sets `line`, `col`, and `filePath`.
  - **Method**: `toString()` returns a string representation (e.g., `file.os:10:5`).
- **FileSpan**:
  - **Fields**: `start` and `end` of type `filePosition`.
  - **Constructor**: Initializes `start` and `end` with provided line, column, and path values.
  - **Methods**:
    - `isValid()`: Checks if the span is valid (both `start` and `end` lines are non-negative).
    - `merge(const FileSpan&)`: Merges another span into this one, extending to cover both.
    - `toString()`: Returns a string representation (e.g., `file.os:10:5 - file.os:12:3`).
    - `isBefore(const filePosition&, const filePosition&)`: Static helper to compare positions.
- **Global Variables**:
  - `currentSpan`: A global `FileSpan` tracking the current source location.
  - `allThreadsDone`: A boolean flag indicating whether all compilation threads have finished.
- **Global Functions**:
  - `setSpan(filePosition, filePosition)`: Sets `currentSpan` with start and end positions.
  - `setSpan(FileSpan)`: Sets `currentSpan` to a given span.
  - `setSpan(int, int, int, int, std::string)`: Sets `currentSpan` with start/end line, column, and path.
  - `getSpan()`: Returns the current span.
  - `setSpanFromPosition(int, int, std::string)`: Sets `currentSpan` to a single position (start = end).

## core.cpp Definitions

The `core.cpp` file provides the implementation of the methods declared in `FileSpan.h`. Below is the relevant portion of `core.cpp`:

```cpp
#include <omniscript/FileSpan.h>

namespace Omniscript {
// filePosition method definitions
FileSpan currentSpan;

std::string filePosition::toString() const {
    return filePath + ":" + std::to_string(line) + ":" + std::to_string(col);
}

// FileSpan method definitions
bool FileSpan::isValid() const {
    return start.line >= 0 && end.line >= 0;
}

void FileSpan::merge(const FileSpan& other) {
    if (!other.isValid()) return;
    if (!isValid()) {
        *this = other;
        return;
    }

    if (isBefore(other.start, start)) start = other.start;
    if (isBefore(end, other.end)) end = other.end;
}

std::string FileSpan::toString() const {
    return start.toString() + " - " + end.toString();
}

bool FileSpan::isBefore(const filePosition& a, const filePosition& b) {
    if (a.filePath != b.filePath) return false; // Or compare lexicographically if needed
    if (a.line < b.line) return true;
    if (a.line == b.line && a.col < b.col) return true;
    return false;
}

// Function definitions
void setSpan(const filePosition& start, const filePosition& end) {
    currentSpan.start = start;
    currentSpan.end = end;
}

void setSpan(const FileSpan& span) {
    currentSpan = span;
}

void setSpan(int startLine, int startCol, int endLine, int endCol, const std::string& path) {
    currentSpan.start.line = startLine;
    currentSpan.start.col = startCol;
    currentSpan.start.filePath = path;
    
    currentSpan.end.line = endLine;
    currentSpan.end.col = endCol;
    currentSpan.end.filePath = path;
}

FileSpan getSpan() {
    return currentSpan;
}

void setSpanFromPosition(int line, int column, const std::string& path) {
    filePosition pos;
    pos.line = line;
    pos.col = column;
    pos.filePath = path;
    currentSpan.start = pos;
    currentSpan.end = pos;
}

} // namespace Omniscript
```

### Definition Details
- **filePosition::toString()**:
  - Concatenates `filePath`, `line`, and `col` into a string (e.g., `file.os:10:5`).
- **FileSpan::isValid()**:
  - Returns true if both `start.line` and `end.line` are non-negative, indicating a valid span.
- **FileSpan::merge(const FileSpan&)**:
  - If the other span is invalid, does nothing.
  - If the current span is invalid, copies the other span.
  - Otherwise, updates `start` to the earlier position and `end` to the later position using `isBefore`.
- **FileSpan::toString()**:
  - Returns a string combining `start.toString()` and `end.toString()` (e.g., `file.os:10:5 - file.os:12:3`).
- **FileSpan::isBefore(const filePosition&, const filePosition&)**:
  - Compares two positions in the same file:
    - Returns false if file paths differ.
    - Returns true if `a.line < b.line` or if lines are equal and `a.col < b.col`.
- **setSpan(filePosition, filePosition)**:
  - Assigns the provided `start` and `end` positions to `currentSpan`.
- **setSpan(FileSpan)**:
  - Copies the provided span to `currentSpan`.
- **setSpan(int, int, int, int, std::string)**:
  - Sets `currentSpan`’s `start` and `end` fields with the provided line, column, and path values.
- **getSpan()**:
  - Returns a copy of `currentSpan`.
- **setSpanFromPosition(int, int, std::string)**:
  - Creates a `filePosition` with the given line, column, and path, and sets both `currentSpan.start` and `currentSpan.end` to this position.

## Usage in OS Compiler
- **Error Reporting**: The `Console` class uses `FileSpan` to report errors with precise source locations. For example, a syntax error in an OS `Vec2 can +` method would include a `FileSpan` indicating the problematic line and column.
- **Parser Integration**: During tokenization, the parser updates `currentSpan` to reflect the current token’s position, ensuring accurate diagnostics for constructs like `Sprite` or `DynArray<T>`.
- **Multi-Threading**: The `allThreadsDone` flag is used in multi-threaded compilation scenarios (e.g., JIT compilation of the starfield example) to synchronize thread completion before cleanup.
- **Debugging**: `toString()` methods provide human-readable output for debugging, integrated with `Console`’s `showSourceContext` to display source code snippets.

## Example Usage
Consider an OS source file `example.os` with a syntax error:
```os
struct Vec2 {
    x: float;
    y: float;

    Vec2 can + (other: Vec2) => Vec2 {  // Error: missing semicolon
        return Vec2 { x: this.x + other.x, y: this.y + other.y }
    }
}
```
The parser might detect the missing semicolon and set the span:
```cpp
Omniscript::setSpan(5, 1, 5, 1, "example.os"); // Point to the end of the return statement
console.reportError(Omniscript::Console::SYNTAX_ERROR, "Missing semicolon", Omniscript::getSpan());
```
Output:
```
syntax error: Missing semicolon
  --> example.os:5:1
    5 |        return Vec2 { x: this.x + other.x, y: this.y + other.y }
      |        ^
```

## Integration with Project
- **File Placement**: Save `FileSpan.h` in `include/omniscript/FileSpan.h` and `core.cpp` in `src/core.cpp` to align with the `premake5.lua` project structure.
- **Build**: The `premake5.lua` script includes `src/**.cpp` and `include/` in the build, so no modifications are needed.
- **Dependencies**: Requires `<string>` from the C++ standard library, included in `FileSpan.h`.

## Adding to the Index
To include this document in your documentation index (`index.md`):
```markdown
| FileSpan: Source Code Position and Span Tracking | Documents the filePosition and FileSpan structures for tracking source code locations in the OS compiler, used for precise error reporting. | [FileSpan](FileSpan.md) |
```

## Conclusion
The `FileSpan` module is a cornerstone of the OS compiler’s diagnostic system, providing robust source code position and span tracking. By combining `filePosition` and `FileSpan` with global span management, the compiler can deliver precise, context-rich error messages, enhancing developer experience. The implementations in `core.cpp` are efficient and thread-safe, supporting the professional-grade requirements of the OmniScript++ project.
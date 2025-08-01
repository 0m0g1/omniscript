# FileSpan

## Purpose
The `FileSpan` component is a critical utility in the OmniScript++ (OS) compiler for tracking source code locations. It enables precise diagnostic reporting by associating errors, warnings, and other messages with specific ranges of source code (e.g., lines and columns in a file). By maintaining start and end positions, `FileSpan` supports detailed error messages, such as those used in syntax or semantic errors, and facilitates context display (e.g., showing offending source lines with caret indicators). In professional compiler design, accurate source tracking is essential for user-friendly diagnostics, and `FileSpan` exemplifies this practice in the OS compiler.

## Declarations
Below is the header file for `FileSpan`, defining the `filePosition` and `FileSpan` structures, along with global functions for managing a current span.

```cpp
#pragma once

#include <string>

namespace Omniscript {

// Position structure
struct filePosition {
    size_t line = -1;
    size_t col = -1;
    std::string filePath;

    filePosition() = default;

    filePosition(size_t l, size_t c, const std::string& path)
        : line(l), col(c), filePath(path) {}

    std::string toString() const;
};

// Span structure for tracking ranges
struct FileSpan {
    filePosition start;
    filePosition end;

    FileSpan(size_t sLine = -1, size_t sCol = -1, size_t eLine = -1, size_t eCol = -1, const std::string& path = "")
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
void setSpan(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& path);
FileSpan getSpan();
void setSpanFromPosition(size_t line, size_t column, const std::string& path);

extern bool allThreadsDone;

}  // namespace Omniscript
```

### Explanation
- **`filePosition`**: Represents a single point in the source code, with `line`, `col`, and `filePath`. The default constructor initializes invalid positions (`-1`), while the parameterized constructor sets specific values. The `toString()` method formats the position as `file:line:col`.
- **`FileSpan`**: Encapsulates a range of source code using `start` and `end` `filePosition` objects. The constructor allows initialization with line, column, and file path. Key methods include:
  - `isValid()`: Checks if the span has valid line numbers (≥ 0).
  - `merge()`: Combines two spans by selecting the earliest start and latest end positions.
  - `toString()`: Formats the span as `start - end`.
  - `isBefore()`: A private static method to compare positions, ensuring correct merging logic.
- **Global Functions**: Functions like `setSpan()` and `getSpan()` manage a global `currentSpan`, enabling components like the parser to track the current source location during compilation. `setSpanFromPosition()` sets a single-point span.
- **`allThreadsDone`**: An external boolean, likely used for thread synchronization, but not directly related to `FileSpan`’s core functionality.

## Definitions
Below is the implementation file for `FileSpan`, defining the methods declared in the header.

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

void setSpan(size_t startLine, size_t startCol, size_t endLine, size_t endCol, const std::string& path) {
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

void setSpanFromPosition(size_t line, size_t column, const std::string& path) {
    filePosition pos;
    pos.line = line;
    pos.col = column;
    pos.filePath = path;
    currentSpan.start = pos;
    currentSpan.end = pos;
}

}
```

### Explanation
- **`filePosition::toString()`**: Concatenates file path, line, and column into a string (e.g., `main.os:10:5`).
- **`FileSpan::isValid()`**: Ensures both start and end lines are non-negative, indicating a valid span.
- **`FileSpan::merge()`**: Updates the span to encompass another valid span, using `isBefore()` to compare positions. If the current span is invalid, it adopts the other span.
- **`FileSpan::toString()`**: Combines start and end position strings (e.g., `main.os:10:5 - main.os:10:15`).
- **`FileSpan::isBefore()`**: Compares two positions, returning `false` for different files (a simplification, as noted in the code) or ordering by line and column.
- **Global Functions**: Implementations of `setSpan()` variants assign values to `currentSpan`, while `getSpan()` retrieves it. `setSpanFromPosition()` creates a single-point span for cases like token start positions.

## Usage in OS Compiler
The `FileSpan` component is used throughout the OS compiler to track source locations during parsing, semantic analysis, and code generation. For example, consider an OS script (`examples/types.os`) with a syntax error:

```os
let x: Vec2 = 42; // Type mismatch
```

The parser records the span of the expression `42` using `setSpan(1, 14, 1, 16, "types.os")`. When the type checker detects a mismatch (expecting `Vec2`, got `int`), it calls `Console::reportError()` with `getSpan()`, producing output like:

```
type error: Expected Vec2, got int
  --> types.os:1:14-16
    1 | let x: Vec2 = 42;
                  ^^
```

This precise location tracking enhances developer experience by pinpointing errors, a hallmark of professional compilers.

## Development Notes
The `FileSpan` component was designed early in the OS compiler’s development to establish a robust foundation for diagnostics. Key design decisions include:
- **Separate `filePosition` Structure**: Allows fine-grained position tracking and reuse in other components.
- **Global `currentSpan`**: Simplifies span management across parsing phases but assumes single-threaded parsing (note the `allThreadsDone` variable, suggesting future thread-safety considerations).
- **Merge Functionality**: Supports combining spans for constructs spanning multiple tokens (e.g., expressions or statements).
Challenges included defining `isBefore()` for cross-file comparisons, resolved by returning `false` for different files, with a comment noting potential lexicographical ordering. This component was implemented before the parser to ensure all subsequent components could rely on location tracking.

## Dependencies
- **Standard Library**: Uses `<string>` for file paths and string formatting.
- **No Other OS Components**: `FileSpan` is a foundational utility, independent of other compiler components, though it is heavily used by `Console` for diagnostic reporting.

## Source Code
- Header: [https://github.com/0m0g1/omniscript/blob/main/include/omniscript/FileSpan.h](https://github.com/0m0g1/omniscript/blob/main/include/omniscript/FileSpan.h)
- Implementation: [https://github.com/0m0g1/omniscript/blob/main/src/FileSpan.cpp](https://github.com/0m0g1/omniscript/blob/main/src/FileSpan.cpp)

## Integration with Project
- **File Placement**:
  - Header: `include/omniscript/FileSpan.h`
  - Implementation: `src/FileSpan.cpp`
- **Build System**: The `premake5.lua` script includes `FileSpan.cpp` in the source file list for compilation. The header is placed in the include directory, accessible to other components like `Console`. No special build flags are required, as `FileSpan` uses standard C++ features.
- **Compatibility**: Works with the OS compiler’s build system, which generates Makefiles via `premake5 gmake2` and supports Debug/Release modes.

## Adding to the Index
Add the following entry to `index.md` under the Component Reference table:

```markdown
| FileSpan | Tracks source code locations for precise diagnostics. | [FileSpan](FileSpan.md) |
```
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

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

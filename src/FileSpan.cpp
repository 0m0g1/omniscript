// core.cpp
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

}
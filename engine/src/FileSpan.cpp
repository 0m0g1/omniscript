// core.cpp
#include <omniscript/FileSpan.h>

#include <string>

namespace Omniscript {


// ---------------- FilePosition ----------------

std::string FilePosition::toString() const {
    const std::string path = filePath.empty() ? "?" : filePath;
    if (!isValid()) return path + ":?:?";
    return path + ":" + std::to_string(line) + ":" + std::to_string(col);
}

// ---------------- FileSpan ----------------

bool FileSpan::isBefore(const FilePosition& a, const FilePosition& b) noexcept {
    if (a.filePath != b.filePath) return false;
    if (a.line < b.line) return true;
    if (a.line > b.line) return false;
    return a.col < b.col;
}

void FileSpan::merge(const FileSpan& other) {
    if (!other.isValid()) return;

    if (!isValid()) {
        *this = other;
        return;
    }

    if (start.filePath != other.start.filePath) {
        return; // don't merge across files
    }

    if (isBefore(other.start, start)) start = other.start;
    if (isBefore(end, other.end))     end = other.end;
}

std::string FileSpan::toString() const {
    if (!isValid()) return "<invalid span>";
    return start.toString() + " - " + end.toString();
}

} // namespace Omniscript

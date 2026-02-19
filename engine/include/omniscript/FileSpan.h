#pragma once

#include <cstddef>
#include <string>

namespace Omniscript {

struct FilePosition {
    std::size_t line = 0;   // 1-based recommended, 0 = invalid/unknown
    std::size_t col  = 0;   // 1-based recommended, 0 = invalid/unknown
    std::string filePath;

    FilePosition() = default;

    FilePosition(std::size_t l, std::size_t c, std::string path = {})
        : line(l), col(c), filePath(std::move(path)) {}

    bool isValid() const noexcept { return line != 0 && col != 0; }

    std::string toString() const;
};

struct FileSpan {
    FilePosition start;
    FilePosition end;

    FileSpan() = default;

    FileSpan(FilePosition s, FilePosition e)
        : start(std::move(s)), end(std::move(e)) {}

    FileSpan(std::size_t sLine, std::size_t sCol,
             std::size_t eLine, std::size_t eCol,
             std::string path = {})
        : start(sLine, sCol, path), end(eLine, eCol, std::move(path)) {}

    bool isValid() const noexcept { return start.isValid() && end.isValid(); }

    // Merge spans in the same file; if file differs, you can decide to reject
    void merge(const FileSpan& other);

    std::string toString() const;

private:
    static bool isBefore(const FilePosition& a, const FilePosition& b) noexcept;
};

} // namespace Omniscript

#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <omniscript/FileSpan.h>

namespace Omniscript {

struct Diagnostic {
    FileSpan span{};
    std::string message;
};

class Diagnostics {
public:
    void error(const FileSpan& s, std::string msg) {
        m_errors.push_back({s, std::move(msg)});
    }

    bool hasErrors() const { return !m_errors.empty(); }

    void print(std::ostream& os) const {
        for (auto& e : m_errors) {
            os << "error: " << e.message;
            if (!e.span.start.filePath.empty()) {
                os << " (" << e.span.start.filePath << ":" << e.span.start.line << ":" << e.span.start.col << ")";
            }
            os << "\n";
        }
    }

private:
    std::vector<Diagnostic> m_errors;
};

} // namespace Omniscript
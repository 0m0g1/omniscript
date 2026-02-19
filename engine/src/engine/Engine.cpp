#include <omniscript/Engine.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Omniscript {

Engine::Engine(int argc, char** argv)
  : m_argc(argc), m_argv(argv) {}

std::string Engine::readSourceFile(const std::string& file_path) const {
    std::ifstream in(file_path); // text mode
    if (!in) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int Engine::run() {
    try {
        if (m_argc < 2 || !m_argv || !m_argv[1]) {
            std::cerr << "Usage: " << (m_argv && m_argv[0] ? m_argv[0] : "omniscript")
                      << " <source-file>\n";
            return 2;
        }

        const std::string source = readSourceFile(m_argv[1]);
        std::cout << source << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

} // namespace Omniscript

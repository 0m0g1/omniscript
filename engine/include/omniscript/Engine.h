#pragma once
#include <string>

namespace Omniscript {

class Engine {
public:
    Engine(int argc, char** argv);
    int run();

private:
    std::string readSourceFile(const std::string& file_path) const;
    int m_argc;
    char** m_argv;
};

} // namespace Omniscript

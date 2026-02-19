#pragma once

namespace Omniscript {

class Engine {
public:
    Engine(int argc, char** argv);
    int run();

private:
    int m_argc;
    char** m_argv;
};

} // namespace Omniscript

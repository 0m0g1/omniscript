#pragma once
#include <omniscript/Engine.h>

namespace Omniscript {

class Application {
public:
    Application(int argc, char** argv);
    int run();

private:
    int m_argc;
    char** m_argv;
    Engine m_engine;
};

} // namespace Omniscript

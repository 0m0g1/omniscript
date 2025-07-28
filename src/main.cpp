#include <omniscript/Application.h>
#include <omniscript/omniscript_pch.h>
#include <iostream>

int main(int argc, char* argv[]) noexcept {
    Omniscript::Application app(argc, argv);
    return app.run();
}
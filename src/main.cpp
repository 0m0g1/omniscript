#include <omniscript/Application.h>
#include <omniscript/utils.h>

int main(int argc, char* argv[]) noexcept {
    Omniscript::setToolchainRoot(Omniscript::findToolchainRoot(argv[0]));
    Omniscript::Application app(argc, argv);
    return app.run();
}
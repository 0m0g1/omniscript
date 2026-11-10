// SHOWCASE 2: a window from a C library with no CMake, no Makefile, no SDK.
// std/vulkan.os carries the GLFW declarations (generated from the C header).
//
// Run now:        Osengine.exe docs/examples/showcase/02_window.os --execute
// Ship it:        Osengine.exe docs/examples/showcase/02_window.os --make -o window.exe
// Expected: an 800x600 window that closes when you press the close button.
include "std/vulkan.os";

if (!glfwInit()) { return 1; }

glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
let w = glfwCreateWindow(800, 600, "Hello from OmniScript", nullptr, nullptr);
if (w == nullptr) {
    glfwTerminate();
    return 1;
}

while (!glfwWindowShouldClose(w)) {
    glfwPollEvents();
}

glfwDestroyWindow(w);
glfwTerminate();

// standard/1/Window.os
// Tier 5: window + GL context via GLFW. Call Window.init() once, then open(), loop with shouldClose/poll/swap.
module G = import "standard/1/c/glfw.os";

let GLFW_CLIENT_API: int = 0x22001;
let GLFW_NO_API: int = 0;
let GLFW_CONTEXT_VERSION_MAJOR: int = 0x22002;
let GLFW_CONTEXT_VERSION_MINOR: int = 0x22003;
let GLFW_OPENGL_PROFILE: int = 0x22008;
let GLFW_OPENGL_CORE_PROFILE: int = 0x32001;
let GLFW_RESIZABLE: int = 0x20003;

function init() => bool { return G.glfwInit() != 0; }
function terminate() => void { G.glfwTerminate(); }
function pollEvents() => void { G.glfwPollEvents(); }
function time() => double { return G.glfwGetTime(); }

struct Window {
    h: void* = nullptr;

    // opengl: true = GL 3.3 core context; false = no API (use for Vulkan)
    open(w: int, h: int, title: char*, opengl: bool) => bool {
        if (opengl) {
            G.glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            G.glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            G.glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        } else G.glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        this.h = G.glfwCreateWindow(w, h, title, nullptr, nullptr);
        if (this.h == nullptr) return false;
        if (opengl) G.glfwMakeContextCurrent(this.h);
        return true;
    }
    close() => void { if (this.h != nullptr) { G.glfwDestroyWindow(this.h); this.h = nullptr; } }
    shouldClose() => bool { return G.glfwWindowShouldClose(this.h) != 0; }
    requestClose() => void { G.glfwSetWindowShouldClose(this.h, 1); }
    swap() => void { G.glfwSwapBuffers(this.h); }
    vsync(on: bool) => void { if (on) G.glfwSwapInterval(1); else G.glfwSwapInterval(0); }
    setTitle(t: char*) => void { G.glfwSetWindowTitle(this.h, t); }
    setSize(w: int, h: int) => void { G.glfwSetWindowSize(this.h, w, h); }
    size(w: int*, h: int*) => void { G.glfwGetWindowSize(this.h, w, h); }
    framebufferSize(w: int*, h: int*) => void { G.glfwGetFramebufferSize(this.h, w, h); }
}

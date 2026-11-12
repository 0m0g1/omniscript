// standard/1/Window.os
// Tier 5: window + GL context via GLFW. Call Window.init() once, then open() (or openGL/openVulkan),
// loop with shouldClose/poll/swap. Input: keyDown/mouseDown/cursor. All earlier calls are unchanged.
module G = import "standard/1/c/glfw.os";

let GLFW_CLIENT_API: int = 0x22001;
let GLFW_NO_API: int = 0;
let GLFW_CONTEXT_VERSION_MAJOR: int = 0x22002;
let GLFW_CONTEXT_VERSION_MINOR: int = 0x22003;
let GLFW_OPENGL_PROFILE: int = 0x22008;
let GLFW_OPENGL_CORE_PROFILE: int = 0x32001;
let GLFW_RESIZABLE: int = 0x20003;
let GLFW_VISIBLE: int = 0x20004;
let GLFW_DECORATED: int = 0x20005;

// keys (GLFW key codes) and mouse buttons
let KEY_SPACE: int = 32;
let KEY_0: int = 48;
let KEY_A: int = 65;
let KEY_D: int = 68;
let KEY_E: int = 69;
let KEY_Q: int = 81;
let KEY_R: int = 82;
let KEY_S: int = 83;
let KEY_W: int = 87;
let KEY_ESCAPE: int = 256;
let KEY_ENTER: int = 257;
let KEY_TAB: int = 258;
let KEY_BACKSPACE: int = 259;
let KEY_RIGHT: int = 262;
let KEY_LEFT: int = 263;
let KEY_DOWN: int = 264;
let KEY_UP: int = 265;
let KEY_LEFT_SHIFT: int = 340;
let KEY_LEFT_CONTROL: int = 341;
let MOUSE_LEFT: int = 0;
let MOUSE_RIGHT: int = 1;
let MOUSE_MIDDLE: int = 2;

function init() => bool { return G.glfwInit() != 0; }
function terminate() => void { G.glfwTerminate(); }
function pollEvents() => void { G.glfwPollEvents(); }
function time() => double { return G.glfwGetTime(); }

// Call before open(). Applies to windows created afterwards.
function setResizable(on: bool) => void {
    if (on) G.glfwWindowHint(GLFW_RESIZABLE, 1); else G.glfwWindowHint(GLFW_RESIZABLE, 0);
}

struct Window {
    h: void* = nullptr;
    shareWith: void* = nullptr;      // GLFWwindow* whose GL objects (textures, buffers) this window shares; set by openGLShared


    // opengl: true = GL 3.3 core context; false = no API (use for Vulkan)
    open(w: int, h: int, title: char*, opengl: bool) => bool {
        if (opengl) {
            G.glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            G.glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            G.glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        } else G.glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        this.h = G.glfwCreateWindow(w, h, title, nullptr, this.shareWith);
        if (this.h == nullptr) return false;
        if (opengl) G.glfwMakeContextCurrent(this.h);
        return true;
    }
    // GL 3.3 core window with the context current. Then call OpenGL.load().
    openGL(w: int, h: int, title: char*) => bool { return this.open(w, h, title, true); }
    // GL window sharing textures/buffers with `other` (VAOs/FBOs are never shared by GL).
    openGLShared(w: int, h: int, title: char*, other: Window*) => bool {
        this.shareWith = other.h;
        return this.open(w, h, title, true);
    }
    // No-API window for Vulkan: Vulkan.init(&win, name) or Vulkan.initEx(&win, name, true).
    openVulkan(w: int, h: int, title: char*) => bool { return this.open(w, h, title, false); }

    close() => void { if (this.h != nullptr) { G.glfwDestroyWindow(this.h); this.h = nullptr; } }
    isOpen() => bool { return this.h != nullptr; }
    shouldClose() => bool { return G.glfwWindowShouldClose(this.h) != 0; }
    requestClose() => void { G.glfwSetWindowShouldClose(this.h, 1); }
    makeCurrent() => void { G.glfwMakeContextCurrent(this.h); }
    swap() => void { G.glfwSwapBuffers(this.h); }
    vsync(on: bool) => void { if (on) G.glfwSwapInterval(1); else G.glfwSwapInterval(0); }
    setTitle(t: char*) => void { G.glfwSetWindowTitle(this.h, t); }
    setSize(w: int, h: int) => void { G.glfwSetWindowSize(this.h, w, h); }
    size(w: int*, h: int*) => void { G.glfwGetWindowSize(this.h, w, h); }
    framebufferSize(w: int*, h: int*) => void { G.glfwGetFramebufferSize(this.h, w, h); }

    // true while minimised (framebuffer 0x0): skip rendering this frame
    isMinimized() => bool {
        let w: int = 0; let hh: int = 0;
        G.glfwGetFramebufferSize(this.h, &w, &hh);
        return w == 0 || hh == 0;
    }
    // width / height of the framebuffer (1.0 when height is 0)
    aspect() => float {
        let w: int = 0; let hh: int = 0;
        G.glfwGetFramebufferSize(this.h, &w, &hh);
        if (hh == 0) return 1.0;
        return (w as float) / (hh as float);
    }

    // input: poll after pollEvents()
    keyDown(key: int) => bool { return G.glfwGetKey(this.h, key) != 0; }
    mouseDown(button: int) => bool { return G.glfwGetMouseButton(this.h, button) != 0; }
    cursor(x: double*, y: double*) => void { G.glfwGetCursorPos(this.h, x, y); }

    // GL function loader for OpenGL.load()-style code
    procAddress(name: char*) => void* { return G.glfwGetProcAddress(name); }
}

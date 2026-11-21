// standard/1/c/glfw.os  (windowing + input)
extern "dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll" {
    fn glfwInit() => int;
    fn glfwTerminate() => void;
    fn glfwWindowHint(h: int, v: int) => void;
    fn glfwCreateWindow(w: int, h: int, title: char*, mon: void*, share: void*) => void*;
    fn glfwDestroyWindow(w: void*) => void;
    fn glfwMakeContextCurrent(w: void*) => void;
    fn glfwWindowShouldClose(w: void*) => int;
    fn glfwSetWindowShouldClose(w: void*, v: int) => void;
    fn glfwSwapBuffers(w: void*) => void;
    fn glfwSwapInterval(n: int) => void;
    fn glfwPollEvents() => void;
    fn glfwGetTime() => double;
    fn glfwSetWindowTitle(w: void*, t: char*) => void;
    fn glfwSetWindowSize(w: void*, a: int, b: int) => void;
    fn glfwSetWindowIcon(w: void*, count: int, images: void*) => void;   // images: GLFWimage* {int width; int height; u8* pixels (RGBA8)}
    fn glfwGetWindowSize(w: void*, a: int*, b: int*) => void;
    fn glfwGetFramebufferSize(w: void*, a: int*, b: int*) => void;
    fn glfwGetKey(w: void*, key: int) => int;
    fn glfwGetMouseButton(w: void*, b: int) => int;
    fn glfwGetCursorPos(w: void*, x: double*, y: double*) => void;
    fn glfwGetProcAddress(name: char*) => void*;
    fn glfwCreateStandardCursor(shape: int) => void*;       // 0x36001 arrow, 0x36002 I-beam, 0x36003 crosshair, 0x36004 hand, 0x36005 h-resize, 0x36006 v-resize
    fn glfwSetCursor(w: void*, c: void*) => void;           // c = nullptr restores the default arrow
    fn glfwDestroyCursor(c: void*) => void;
    fn glfwSetScrollCallback(w: void*, cb: void*) => void;  // cb: function(w: void*, dx: double, dy: double) => void, passed as `fn as void*`
    fn glfwVulkanSupported() => int;
    fn glfwGetRequiredInstanceExtensions(n: u32*) => char**;
    fn glfwCreateWindowSurface(inst: void*, w: void*, alloc: void*, surf: u64*) => int;
}

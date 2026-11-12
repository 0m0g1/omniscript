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
    fn glfwGetWindowSize(w: void*, a: int*, b: int*) => void;
    fn glfwGetFramebufferSize(w: void*, a: int*, b: int*) => void;
    fn glfwGetKey(w: void*, key: int) => int;
    fn glfwGetMouseButton(w: void*, b: int) => int;
    fn glfwGetCursorPos(w: void*, x: double*, y: double*) => void;
    fn glfwGetProcAddress(name: char*) => void*;
    fn glfwVulkanSupported() => int;
    fn glfwGetRequiredInstanceExtensions(n: u32*) => char**;
    fn glfwCreateWindowSurface(inst: void*, w: void*, alloc: void*, surf: u64*) => int;
}

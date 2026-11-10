// standard/1/vulkan.os
// Vulkan + GLFW bindings for Omniscript. Use with:  include "std/vulkan.os";   (std/ resolves to standard/1/)
// Top level on purpose (no `module` wrapper) so names stay unprefixed after `import *`.
// Needs vulkan_min.h next to this file (standard/1/). Runtime: a GPU driver providing vulkan-1.dll.
// module Vk {

// Structs, functions and VK_* constants generated from the header; vulkan-1.dll is found by name via the normal DLL search path.
extern "dependencies/vk/vulkan_full.h", "vulkan-1.dll" { }
extern "dependencies/vk/vulkan_ext.h", "vkext.dll" { }

// GLFW (window + surface only, GLFW loads vulkan-1.dll itself)
extern 
"dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/glfw3.dll", 
"dependencies/glfw/glfw-3.4/bin/lib-mingw-w64/libglfw3.a" {
    fn glfwInit() => int;
    fn glfwCreateWindow(width: int, height: int, title: char*, monitor: void*, share: void*) => void*;
    fn glfwWindowShouldClose(window: void*) => int;
    fn glfwPollEvents() => void;
    fn glfwWaitEvents() => void;
    fn glfwTerminate() => void;
    fn glfwDestroyWindow(window: void*) => void;
    fn glfwWindowHint(hint: int, value: int) => void;
    fn glfwSetErrorCallback(callback: void*) => void*;
    fn glfwGetTime() => double;
    fn glfwGetFramebufferSize(window: void*, width: int*, height: int*) => void;
    fn glfwVulkanSupported() => int;
    fn glfwGetRequiredInstanceExtensions(count: uint*) => char**;
    fn glfwCreateWindowSurface(instance: void*, window: void*, allocator: void*, surface: void**) => int;
}

// GLFW constants
const GLFW_CLIENT_API: int = 0x00022001;
const GLFW_NO_API: int = 0;

// Not expressible as a plain header #define
const VK_SUBPASS_EXTERNAL: uint = 0xFFFFFFFF;
const U64_MAX: u64 = 0xFFFFFFFFFFFFFFFF;

// }

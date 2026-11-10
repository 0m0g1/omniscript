extern "C" fn printf(fmt: char*, ...) => int;
include "std/vulkan.os";

function main() => void {
    printf("GLFW_NO_API=%d VK_SUCCESS=%d\n", GLFW_NO_API, VK_SUCCESS);
}
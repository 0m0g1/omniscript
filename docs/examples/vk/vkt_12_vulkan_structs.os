extern "vulkan_min.h", "vulkan-1.dll" { }
extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

function main() => void {
    printf("[12] start\n"); fflush(nullptr);
    let caps: VkSurfaceCapabilitiesKHR;
    caps.minImageExtent.width = 5;
    let sci: VkSwapchainCreateInfoKHR;
    sci.imageExtent.width = 6;
    let stages: VkPipelineShaderStageCreateInfo[2];
    stages[0].module_ = nullptr;
    stages[1].module_ = nullptr;
    stages[1].pName = "main";
    printf("[12] %u %u\n", caps.minImageExtent.width, sci.imageExtent.width); fflush(nullptr);
}

// standard/1/Vulkan.os
// Tier 6: Vulkan bring-up: instance, surface (GLFW), device, queues, swapchain + image views, sync, command pool.
// Structs assume C layout (natural alignment). sType values from vulkan_core.h.
// TODO: render pass, pipeline, framebuffers, draw loop (bindings exist in c/vulkan.os).
module V = import "standard/1/c/vulkan.os";
module G = import "standard/1/c/glfw.os";
module W = import "standard/1/Window.os";
module L = import "standard/1/List.os";
module CMem = import "standard/1/c/mem.os";

let VK_SUCCESS: int = 0;
let VK_STRUCTURE_TYPE_APPLICATION_INFO: u32 = 0;
let VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO: u32 = 1;
let VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO: u32 = 2;
let VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO: u32 = 3;
let VK_STRUCTURE_TYPE_FENCE_CREATE_INFO: u32 = 8;
let VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO: u32 = 9;
let VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO: u32 = 15;
let VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO: u32 = 39;
let VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR: u32 = 1000001000;
let VK_FORMAT_B8G8R8A8_SRGB: u32 = 50;
let VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: u32 = 0;
let VK_PRESENT_MODE_FIFO_KHR: u32 = 2;
let VK_PRESENT_MODE_MAILBOX_KHR: u32 = 1;
let VK_QUEUE_GRAPHICS_BIT: u32 = 1;
let VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT: u32 = 16;
let VK_FENCE_CREATE_SIGNALED_BIT: u32 = 1;
let VK_API_VERSION_1_1: u32 = 4198400;

struct VkApplicationInfo {
    sType: u32; pNext: void*; pApplicationName: char*; applicationVersion: u32;
    pEngineName: char*; engineVersion: u32; apiVersion: u32;
}
struct VkInstanceCreateInfo {
    sType: u32; pNext: void*; flags: u32; pApplicationInfo: VkApplicationInfo*;
    enabledLayerCount: u32; ppEnabledLayerNames: char**;
    enabledExtensionCount: u32; ppEnabledExtensionNames: char**;
}
struct VkDeviceQueueCreateInfo {
    sType: u32; pNext: void*; flags: u32; queueFamilyIndex: u32; queueCount: u32; pQueuePriorities: float*;
}
struct VkDeviceCreateInfo {
    sType: u32; pNext: void*; flags: u32; queueCreateInfoCount: u32; pQueueCreateInfos: VkDeviceQueueCreateInfo*;
    enabledLayerCount: u32; ppEnabledLayerNames: char**;
    enabledExtensionCount: u32; ppEnabledExtensionNames: char**; pEnabledFeatures: void*;
}
struct VkQueueFamilyProperties { queueFlags: u32; queueCount: u32; timestampValidBits: u32; minW: u32; minH: u32; minD: u32; }
struct VkExtent2D { width: u32; height: u32; }
struct VkSurfaceCapabilitiesKHR {
    minImageCount: u32; maxImageCount: u32; currentExtent: VkExtent2D; minImageExtent: VkExtent2D; maxImageExtent: VkExtent2D;
    maxImageArrayLayers: u32; supportedTransforms: u32; currentTransform: u32; supportedCompositeAlpha: u32; supportedUsageFlags: u32;
}
struct VkSurfaceFormatKHR { format: u32; colorSpace: u32; }
struct VkSwapchainCreateInfoKHR {
    sType: u32; pNext: void*; flags: u32; surface: u64; minImageCount: u32; imageFormat: u32; imageColorSpace: u32;
    imageExtent: VkExtent2D; imageArrayLayers: u32; imageUsage: u32; imageSharingMode: u32;
    queueFamilyIndexCount: u32; pQueueFamilyIndices: u32*; preTransform: u32; compositeAlpha: u32;
    presentMode: u32; clipped: u32; oldSwapchain: u64;
}
struct VkComponentMapping { r: u32; g: u32; b: u32; a: u32; }
struct VkImageSubresourceRange { aspectMask: u32; baseMipLevel: u32; levelCount: u32; baseArrayLayer: u32; layerCount: u32; }
struct VkImageViewCreateInfo {
    sType: u32; pNext: void*; flags: u32; image: u64; viewType: u32; format: u32;
    components: VkComponentMapping; subresourceRange: VkImageSubresourceRange;
}
struct VkSemaphoreCreateInfo { sType: u32; pNext: void*; flags: u32; }
struct VkFenceCreateInfo { sType: u32; pNext: void*; flags: u32; }
struct VkCommandPoolCreateInfo { sType: u32; pNext: void*; flags: u32; queueFamilyIndex: u32; }

struct Vulkan {
    instance: void* = nullptr;
    surface: u64 = 0;
    phys: void* = nullptr;
    device: void* = nullptr;
    queue: void* = nullptr;
    queueFamily: u32 = 0;
    swapchain: u64 = 0;
    format: u32 = 0;
    extent: VkExtent2D;
    images: L.List<u64>;
    views: L.List<u64>;
    imageAvailable: u64 = 0;
    renderFinished: u64 = 0;
    inFlight: u64 = 0;
    pool: u64 = 0;

    // Window must be opened with opengl=false. Returns false on first failure.
    init(win: W.Window*, appName: char*) => bool {
        if (G.glfwVulkanSupported() == 0) return false;
        // instance
        let app: VkApplicationInfo;
        CMem.memset(&app as void*, 0, sizeof(VkApplicationInfo));
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; app.pApplicationName = appName; app.pEngineName = "omniscript"; app.apiVersion = VK_API_VERSION_1_1;
        let extCount: u32 = 0;
        let exts: char** = G.glfwGetRequiredInstanceExtensions(&extCount);
        let ici: VkInstanceCreateInfo;
        CMem.memset(&ici as void*, 0, sizeof(VkInstanceCreateInfo));
        ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; ici.pApplicationInfo = &app;
        ici.enabledExtensionCount = extCount; ici.ppEnabledExtensionNames = exts;
        if (V.vkCreateInstance(&ici as void*, nullptr, &this.instance) != VK_SUCCESS) return false;
        // surface
        if (G.glfwCreateWindowSurface(this.instance, win.h, nullptr, &this.surface) != VK_SUCCESS) return false;
        if (!this.pickDevice()) return false;
        if (!this.createDevice()) return false;
        if (!this.createSwapchain(win)) return false;
        return this.createSync();
    }

    pickDevice() => bool {
        let n: u32 = 0;
        V.vkEnumeratePhysicalDevices(this.instance, &n, nullptr);
        if (n == 0) return false;
        let devs = CMem.malloc((n as u64) * 8) as void**;
        V.vkEnumeratePhysicalDevices(this.instance, &n, devs);
        let found: bool = false;
        let i: u32 = 0;
        while (i < n && !found) {
            let qn: u32 = 0;
            V.vkGetPhysicalDeviceQueueFamilyProperties(devs[i], &qn, nullptr);
            let props = CMem.malloc((qn as u64) * sizeof(VkQueueFamilyProperties)) as VkQueueFamilyProperties*;
            V.vkGetPhysicalDeviceQueueFamilyProperties(devs[i], &qn, props as void*);
            let q: u32 = 0;
            while (q < qn && !found) {
                let present: u32 = 0;
                V.vkGetPhysicalDeviceSurfaceSupportKHR(devs[i], q, this.surface, &present);
                if ((props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && present != 0) { this.phys = devs[i]; this.queueFamily = q; found = true; }
                q = q + 1;
            }
            CMem.free(props as void*);
            i = i + 1;
        }
        CMem.free(devs as void*);
        return found;
    }

    createDevice() => bool {
        let prio: float = 1.0;
        let qci: VkDeviceQueueCreateInfo;
        CMem.memset(&qci as void*, 0, sizeof(VkDeviceQueueCreateInfo));
        qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; qci.queueFamilyIndex = this.queueFamily; qci.queueCount = 1; qci.pQueuePriorities = &prio;
        let ext: char* = "VK_KHR_swapchain";
        let dci: VkDeviceCreateInfo;
        CMem.memset(&dci as void*, 0, sizeof(VkDeviceCreateInfo));
        dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; dci.queueCreateInfoCount = 1; dci.pQueueCreateInfos = &qci;
        dci.enabledExtensionCount = 1; dci.ppEnabledExtensionNames = &ext;
        if (V.vkCreateDevice(this.phys, &dci as void*, nullptr, &this.device) != VK_SUCCESS) return false;
        V.vkGetDeviceQueue(this.device, this.queueFamily, 0, &this.queue);
        return true;
    }

    createSwapchain(win: W.Window*) => bool {
        let caps: VkSurfaceCapabilitiesKHR;
        V.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this.phys, this.surface, &caps as void*);
        // format
        let fmtCount: u32 = 0;
        V.vkGetPhysicalDeviceSurfaceFormatsKHR(this.phys, this.surface, &fmtCount, nullptr);
        let fmts = CMem.malloc((fmtCount as u64) * sizeof(VkSurfaceFormatKHR)) as VkSurfaceFormatKHR*;
        V.vkGetPhysicalDeviceSurfaceFormatsKHR(this.phys, this.surface, &fmtCount, fmts as void*);
        let chosen: VkSurfaceFormatKHR = fmts[0];
        let i: u32 = 0;
        while (i < fmtCount) { if (fmts[i].format == VK_FORMAT_B8G8R8A8_SRGB && fmts[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) chosen = fmts[i]; i = i + 1; }
        CMem.free(fmts as void*);
        this.format = chosen.format;
        // extent
        this.extent = caps.currentExtent;
        if (this.extent.width == 0xFFFFFFFF) {
            let w: int = 0; let h: int = 0;
            win.framebufferSize(&w, &h);
            this.extent.width = w as u32; this.extent.height = h as u32;
        }
        let count: u32 = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && count > caps.maxImageCount) count = caps.maxImageCount;
        let sci: VkSwapchainCreateInfoKHR;
        CMem.memset(&sci as void*, 0, sizeof(VkSwapchainCreateInfoKHR));
        sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; sci.surface = this.surface; sci.minImageCount = count;
        sci.imageFormat = chosen.format; sci.imageColorSpace = chosen.colorSpace; sci.imageExtent = this.extent;
        sci.imageArrayLayers = 1; sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; sci.imageSharingMode = 0;
        sci.preTransform = caps.currentTransform; sci.compositeAlpha = 1; sci.presentMode = VK_PRESENT_MODE_FIFO_KHR; sci.clipped = 1;
        if (V.vkCreateSwapchainKHR(this.device, &sci as void*, nullptr, &this.swapchain) != VK_SUCCESS) return false;
        // images + views
        let n: u32 = 0;
        V.vkGetSwapchainImagesKHR(this.device, this.swapchain, &n, nullptr);
        this.images.reserve(n as u64);
        V.vkGetSwapchainImagesKHR(this.device, this.swapchain, &n, this.images.data);
        this.images.len = n as u64;
        let k: u32 = 0;
        while (k < n) {
            let vci: VkImageViewCreateInfo;
            CMem.memset(&vci as void*, 0, sizeof(VkImageViewCreateInfo));
            vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; vci.image = this.images.data[k]; vci.viewType = 1; vci.format = this.format;
            vci.subresourceRange.aspectMask = 1; vci.subresourceRange.levelCount = 1; vci.subresourceRange.layerCount = 1;
            let view: u64 = 0;
            if (V.vkCreateImageView(this.device, &vci as void*, nullptr, &view) != VK_SUCCESS) return false;
            this.views.push(view);
            k = k + 1;
        }
        return true;
    }

    createSync() => bool {
        let sci: VkSemaphoreCreateInfo; CMem.memset(&sci as void*, 0, sizeof(VkSemaphoreCreateInfo));
        sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        let fci: VkFenceCreateInfo; CMem.memset(&fci as void*, 0, sizeof(VkFenceCreateInfo));
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO; fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        let pci: VkCommandPoolCreateInfo; CMem.memset(&pci as void*, 0, sizeof(VkCommandPoolCreateInfo));
        pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; pci.flags = 2; pci.queueFamilyIndex = this.queueFamily;
        if (V.vkCreateSemaphore(this.device, &sci as void*, nullptr, &this.imageAvailable) != VK_SUCCESS) return false;
        if (V.vkCreateSemaphore(this.device, &sci as void*, nullptr, &this.renderFinished) != VK_SUCCESS) return false;
        if (V.vkCreateFence(this.device, &fci as void*, nullptr, &this.inFlight) != VK_SUCCESS) return false;
        return V.vkCreateCommandPool(this.device, &pci as void*, nullptr, &this.pool) == VK_SUCCESS;
    }

    waitIdle() => void { if (this.device != nullptr) V.vkDeviceWaitIdle(this.device); }

    free() => void {
        if (this.device == nullptr) return;
        V.vkDeviceWaitIdle(this.device);
        let i: u64 = 0;
        while (i < this.views.len) { V.vkDestroyImageView(this.device, this.views.data[i], nullptr); i = i + 1; }
        this.views.free(); this.images.free();
        if (this.pool != 0) V.vkDestroyCommandPool(this.device, this.pool, nullptr);
        if (this.inFlight != 0) V.vkDestroyFence(this.device, this.inFlight, nullptr);
        if (this.renderFinished != 0) V.vkDestroySemaphore(this.device, this.renderFinished, nullptr);
        if (this.imageAvailable != 0) V.vkDestroySemaphore(this.device, this.imageAvailable, nullptr);
        if (this.swapchain != 0) V.vkDestroySwapchainKHR(this.device, this.swapchain, nullptr);
        V.vkDestroyDevice(this.device, nullptr);
        if (this.surface != 0) V.vkDestroySurfaceKHR(this.instance, this.surface, nullptr);
        V.vkDestroyInstance(this.instance, nullptr);
        this.device = nullptr;
    }
}

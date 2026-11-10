/*
Vulkan version of starlikeglad.os (single threaded, FIFO/vsync present).
TRACE BUILD: every step prints "[T] ..." and flushes, so the last [T] line shows where it stops.

Files that must sit next to this one:
  standard/1/vulkan.os + dependencies/vk/vulkan_full.h   Vulkan/GLFW declarations (header importer, no SDK needed)
  starlikeglad_vk.vert.spv     precompiled shaders (sources: starlikeglad_vk.vert / .frag)
  starlikeglad_vk.frag.spv
Runtime requirement: a GPU driver that provides vulkan-1.dll (every normal Windows install has it).
Shaders are loaded relative to the working directory, which must be the project root
(the folder containing docs/ and standard/); the paths below point into docs/examples/vk/.

Compile with:
./path/to/Osengine.exe docs/examples/vk/starlikeglad_vk.os // For JIT mode
./path/to/Osengine.exe docs/examples/vk/starlikeglad_vk.os --make -o app.exe // For AOT mode
*/
extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn fopen(path: char*, mode: char*) => void*;
extern "C" fn fclose(f: void*) => int;
extern "C" fn fseek(f: void*, offset: int, origin: int) => int;
extern "C" fn ftell(f: void*) => int;
extern "C" fn fread(buf: void*, size: u64, count: u64, f: void*) => u64;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

// Vulkan + GLFW declarations, constants and the header import live in the std module
include "std/vulkan.os";

// Push constants, must match the layout in the fragment shader (12 bytes)
struct PushConstants {
    resX: float = 0.0;
    resY: float = 0.0;
    time: float = 0.0;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
let g_fileSize: u64 = 0;
let g_frame: int = 0;

// Trace: print + flush so output survives a crash
function trace(s: char*) => void {
    printf("[T] %s\n", s);
    fflush(nullptr);
}

// Only traces during the first few frames, to avoid flooding the console
function traceFrame(s: char*) => void {
    if (g_frame <= 3) {
        printf("[T] frame %d: %s\n", g_frame, s);
        fflush(nullptr);
    }
}

function loadFile(path: char*) => void* {
    let f = fopen(path, "rb");
    if (f == nullptr) {
        printf("Failed to open file: %s\n", path);
        fflush(nullptr);
        return nullptr;
    }
    fseek(f, 0, 2);
    let n = ftell(f);
    fseek(f, 0, 0);
    let buf = malloc(n as u64);
    fread(buf, 1, n as u64, f);
    fclose(f);
    g_fileSize = n as u64;
    return buf;
}

function vkOk(result: int, what: char*) => int {
    if (result != VK_SUCCESS) {
        printf("Vulkan error %d in %s\n", result, what);
        fflush(nullptr);
        return 0;
    }
    return 1;
}

function glfwErrorCallback(error: int, description: char*) => void {
    printf("GLFW Error %d: %s\n", error, description);
    fflush(nullptr);
}

// ---------------------------------------------------------------------------
// Application
// ---------------------------------------------------------------------------
struct VulkanApp {
    window: void* = nullptr;
    instance: void* = nullptr;
    surface: void* = nullptr;
    physicalDevice: void* = nullptr;
    device: void* = nullptr;
    queue: void* = nullptr;
    queueFamily: uint = 0;

    swapchain: void* = nullptr;
    imageCount: uint = 0;
    images: void** = nullptr;
    views: void** = nullptr;
    framebuffers: void** = nullptr;
    renderFinished: void** = nullptr;
    extentW: uint = 0;
    extentH: uint = 0;

    renderPass: void* = nullptr;
    pipelineLayout: void* = nullptr;
    pipeline: void* = nullptr;
    commandPool: void* = nullptr;
    commandBuffer: void* = nullptr;
    imageAvailable: void* = nullptr;
    inFlight: void* = nullptr;

    createInstance() => int {
        trace("createInstance: glfwVulkanSupported");
        if (!glfwVulkanSupported()) {
            printf("GLFW: Vulkan not supported (is vulkan-1.dll installed?)\n");
            return 0;
        }

        trace("createInstance: glfwGetRequiredInstanceExtensions");
        let extCount: uint = 0;
        let exts = glfwGetRequiredInstanceExtensions(&extCount);

        trace("createInstance: fill structs");
        let app: VkApplicationInfo;
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = "Vulkan Star Field";
        app.pEngineName = "Osengine";
        // VK_API_VERSION_1_0 is a function-like macro in the real header, so it is not generated
        app.apiVersion = 0x00400000;

        let ci: VkInstanceCreateInfo;
        ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pApplicationInfo = &app;
        ci.enabledExtensionCount = extCount;
        ci.ppEnabledExtensionNames = exts;

        trace("createInstance: vkCreateInstance (&this.instance)");
        if (!vkOk(vkCreateInstance(&ci, nullptr, &this.instance), "vkCreateInstance")) return 0;
        printf("Created Vulkan instance\n");
        return 1;
    }

    createSurface() => int {
        trace("createSurface: glfwCreateWindowSurface (&this.surface)");
        if (!vkOk(glfwCreateWindowSurface(this.instance, this.window, nullptr, &this.surface), "glfwCreateWindowSurface")) return 0;
        printf("Created surface\n");
        return 1;
    }

    pickDevice() => int {
        let count: uint = 1;
        trace("pickDevice: vkEnumeratePhysicalDevices (&this.physicalDevice)");
        let r = vkEnumeratePhysicalDevices(this.instance, &count, &this.physicalDevice);
        if (r != VK_SUCCESS && r != VK_INCOMPLETE) { // INCOMPLETE: more than one GPU, we take the first
            printf("vkEnumeratePhysicalDevices failed: %d\n", r);
            return 0;
        }
        if (count == 0 || this.physicalDevice == nullptr) {
            printf("No Vulkan capable GPU found\n");
            return 0;
        }

        // Find a queue family that supports graphics AND presenting
        trace("pickDevice: queue family count");
        let famCount: uint = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(this.physicalDevice, &famCount, nullptr);
        printf("[T] famCount = %u\n", famCount); fflush(nullptr);

        trace("pickDevice: malloc props");
        let props = malloc((famCount * 24) as u64) as VkQueueFamilyProperties*;
        vkGetPhysicalDeviceQueueFamilyProperties(this.physicalDevice, &famCount, props);

        trace("pickDevice: loop over families");
        let found: int = 0;
        let i: uint = 0;
        while (i < famCount && !found) {
            let supported: uint = 0;
            vkGetPhysicalDeviceSurfaceSupportKHR(this.physicalDevice, i, this.surface, &supported);
            printf("[T] family %u: flags=%u present=%u\n", i, props[i].queueFlags, supported); fflush(nullptr);
            if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && supported != 0) {
                this.queueFamily = i;
                found = 1;
            }
            i = i + 1;
        }
        free(props as void*);

        if (!found) {
            printf("No graphics+present queue family\n");
            return 0;
        }
        printf("Using queue family %d\n", this.queueFamily);
        return 1;
    }

    createDevice() => int {
        trace("createDevice: fill structs");
        let priority: float = 1.0;
        let qci: VkDeviceQueueCreateInfo;
        qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = this.queueFamily;
        qci.queueCount = 1;
        qci.pQueuePriorities = &priority;

        let swapchainExt: char* = "VK_KHR_swapchain";

        let dci: VkDeviceCreateInfo;
        dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dci.queueCreateInfoCount = 1;
        dci.pQueueCreateInfos = &qci;
        dci.enabledExtensionCount = 1;
        dci.ppEnabledExtensionNames = &swapchainExt;

        trace("createDevice: vkCreateDevice");
        if (!vkOk(vkCreateDevice(this.physicalDevice, &dci, nullptr, &this.device), "vkCreateDevice")) return 0;
        trace("createDevice: vkGetDeviceQueue");
        vkGetDeviceQueue(this.device, this.queueFamily, 0, &this.queue);
        printf("Created logical device\n");
        return 1;
    }

    createSwapchain() => int {
        trace("createSwapchain: surface caps");
        let caps: VkSurfaceCapabilitiesKHR;
        if (!vkOk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this.physicalDevice, this.surface, &caps), "surface caps")) return 0;
        printf("[T] caps: minImages=%u maxImages=%u min=%ux%u max=%ux%u\n", caps.minImageCount, caps.maxImageCount, caps.minImageExtent.width, caps.minImageExtent.height, caps.maxImageExtent.width, caps.maxImageExtent.height); fflush(nullptr);

        trace("createSwapchain: framebuffer size");
        let w: int;
        let h: int;
        glfwGetFramebufferSize(this.window, &w, &h);
        printf("[T] framebuffer %d x %d\n", w, h); fflush(nullptr);
        if (w <= 0 || h <= 0) return 2; // minimized

        this.extentW = w as uint;
        this.extentH = h as uint;
        if (this.extentW < caps.minImageExtent.width) this.extentW = caps.minImageExtent.width;
        if (this.extentH < caps.minImageExtent.height) this.extentH = caps.minImageExtent.height;
        if (this.extentW > caps.maxImageExtent.width) this.extentW = caps.maxImageExtent.width;
        if (this.extentH > caps.maxImageExtent.height) this.extentH = caps.maxImageExtent.height;

        let minImages = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && minImages > caps.maxImageCount) minImages = caps.maxImageCount;

        trace("createSwapchain: fill swapchain info");
        let sci: VkSwapchainCreateInfoKHR;
        sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        sci.surface = this.surface;
        sci.minImageCount = minImages;
        sci.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        sci.imageExtent.width = this.extentW;
        sci.imageExtent.height = this.extentH;
        sci.imageArrayLayers = 1;
        sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        sci.preTransform = caps.currentTransform;
        sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        sci.clipped = VK_TRUE;

        trace("createSwapchain: vkCreateSwapchainKHR");
        if (!vkOk(vkCreateSwapchainKHR(this.device, &sci, nullptr, &this.swapchain), "vkCreateSwapchainKHR")) return 0;

        trace("createSwapchain: get images");
        vkGetSwapchainImagesKHR(this.device, this.swapchain, &this.imageCount, nullptr);
        printf("[T] imageCount = %u\n", this.imageCount); fflush(nullptr);
        this.images = malloc((this.imageCount * 8) as u64) as void**;
        this.views = malloc((this.imageCount * 8) as u64) as void**;
        this.framebuffers = malloc((this.imageCount * 8) as u64) as void**;
        this.renderFinished = malloc((this.imageCount * 8) as u64) as void**;
        vkGetSwapchainImagesKHR(this.device, this.swapchain, &this.imageCount, this.images);

        // Workaround: a `return` inside a `while` body is typed 'Void' by the compiler, so failures
        // inside the loop set `ok` and the return happens after the loop.
        let ok: int = 1;
        let i: uint = 0;
        while (i < this.imageCount && ok != 0) {
            printf("[T] createSwapchain: image %u view\n", i); fflush(nullptr);
            let vci: VkImageViewCreateInfo;
            vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vci.image = this.images[i];
            vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vci.format = VK_FORMAT_B8G8R8A8_UNORM;
            vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vci.subresourceRange.levelCount = 1;
            vci.subresourceRange.layerCount = 1;
            if (!vkOk(vkCreateImageView(this.device, &vci, nullptr, &this.views[i]), "vkCreateImageView")) ok = 0;

            if (ok != 0) {
                printf("[T] createSwapchain: image %u framebuffer\n", i); fflush(nullptr);
                let fci: VkFramebufferCreateInfo;
                fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fci.renderPass = this.renderPass;
                fci.attachmentCount = 1;
                fci.pAttachments = &this.views[i];
                fci.width = this.extentW;
                fci.height = this.extentH;
                fci.layers = 1;
                if (!vkOk(vkCreateFramebuffer(this.device, &fci, nullptr, &this.framebuffers[i]), "vkCreateFramebuffer")) ok = 0;
            }

            if (ok != 0) {
                printf("[T] createSwapchain: image %u semaphore\n", i); fflush(nullptr);
                let semci: VkSemaphoreCreateInfo;
                semci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                if (!vkOk(vkCreateSemaphore(this.device, &semci, nullptr, &this.renderFinished[i]), "renderFinished semaphore")) ok = 0;
            }

            i = i + 1;
        }
        if (ok == 0) return 0;
        trace("createSwapchain: done");
        return 1;
    }

    destroySwapchain() => void {
        if (this.swapchain == nullptr) return;
        let i: uint = 0;
        while (i < this.imageCount) {
            vkDestroySemaphore(this.device, this.renderFinished[i], nullptr);
            vkDestroyFramebuffer(this.device, this.framebuffers[i], nullptr);
            vkDestroyImageView(this.device, this.views[i], nullptr);
            i = i + 1;
        }
        free(this.images as void*);
        free(this.views as void*);
        free(this.framebuffers as void*);
        free(this.renderFinished as void*);
        vkDestroySwapchainKHR(this.device, this.swapchain, nullptr);
        this.swapchain = nullptr;
        this.imageCount = 0;
    }

    recreateSwapchain() => int {
        trace("recreateSwapchain");
        vkDeviceWaitIdle(this.device);
        this.destroySwapchain();
        return this.createSwapchain();
    }

    createRenderPass() => int {
        trace("createRenderPass: fill structs");
        let att: VkAttachmentDescription;
        att.format = VK_FORMAT_B8G8R8A8_UNORM;
        att.samples = VK_SAMPLE_COUNT_1_BIT;
        att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        let colorRef: VkAttachmentReference;
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        let sub: VkSubpassDescription;
        sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount = 1;
        sub.pColorAttachments = &colorRef;

        let dep: VkSubpassDependency;
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcAccessMask = 0;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        let rpci: VkRenderPassCreateInfo;
        rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpci.attachmentCount = 1;
        rpci.pAttachments = &att;
        rpci.subpassCount = 1;
        rpci.pSubpasses = &sub;
        rpci.dependencyCount = 1;
        rpci.pDependencies = &dep;

        trace("createRenderPass: vkCreateRenderPass");
        return vkOk(vkCreateRenderPass(this.device, &rpci, nullptr, &this.renderPass), "vkCreateRenderPass");
    }

    createShaderModule(path: char*, outModule: void**) => int {
        printf("[T] createShaderModule: loading %s\n", path); fflush(nullptr);
        let code = loadFile(path);
        if (code == nullptr) return 0;
        printf("[T] createShaderModule: loaded %u bytes\n", g_fileSize as uint); fflush(nullptr);
        let smci: VkShaderModuleCreateInfo;
        smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smci.codeSize = g_fileSize;
        smci.pCode = code;
        let ok = vkOk(vkCreateShaderModule(this.device, &smci, nullptr, outModule), "vkCreateShaderModule");
        free(code);
        return ok;
    }

    createPipeline() => int {
        let vert: void*;
        let frag: void*;
        trace("createPipeline: vertex shader module");
        if (!this.createShaderModule("docs/examples/vk/starlikeglad_vk.vert.spv", &vert)) return 0;
        trace("createPipeline: fragment shader module");
        if (!this.createShaderModule("docs/examples/vk/starlikeglad_vk.frag.spv", &frag)) return 0;

        // The importer renames the keyword field "module" to "module_"
        trace("createPipeline: stages array");
        let stages: VkPipelineShaderStageCreateInfo[2];
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].pNext = nullptr;
        stages[0].flags = 0;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module_ = vert;
        stages[0].pName = "main";
        stages[0].pSpecializationInfo = nullptr;
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].pNext = nullptr;
        stages[1].flags = 0;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module_ = frag;
        stages[1].pName = "main";
        stages[1].pSpecializationInfo = nullptr;

        trace("createPipeline: fixed-function state");
        let vertexInput: VkPipelineVertexInputStateCreateInfo;
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        let inputAsm: VkPipelineInputAssemblyStateCreateInfo;
        inputAsm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAsm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        let viewportState: VkPipelineViewportStateCreateInfo;
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        let raster: VkPipelineRasterizationStateCreateInfo;
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.lineWidth = 1.0;

        let ms: VkPipelineMultisampleStateCreateInfo;
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        let blendAtt: VkPipelineColorBlendAttachmentState;
        // R | G | B | A (the real header has no VK_COLOR_COMPONENT_RGBA shortcut)
        blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        let blend: VkPipelineColorBlendStateCreateInfo;
        blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blend.attachmentCount = 1;
        blend.pAttachments = &blendAtt;

        trace("createPipeline: dynamic states array");
        let dynStates: uint[2] = [VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR];
        let dyn: VkPipelineDynamicStateCreateInfo;
        dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dyn.dynamicStateCount = 2;
        dyn.pDynamicStates = &dynStates;

        let pcRange: VkPushConstantRange;
        pcRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pcRange.offset = 0;
        pcRange.size = 12;

        let plci: VkPipelineLayoutCreateInfo;
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges = &pcRange;
        trace("createPipeline: vkCreatePipelineLayout");
        if (!vkOk(vkCreatePipelineLayout(this.device, &plci, nullptr, &this.pipelineLayout), "vkCreatePipelineLayout")) return 0;

        trace("createPipeline: fill graphics pipeline info");
        let gpci: VkGraphicsPipelineCreateInfo;
        gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gpci.stageCount = 2;
        gpci.pStages = &stages;
        gpci.pVertexInputState = &vertexInput;
        gpci.pInputAssemblyState = &inputAsm;
        gpci.pViewportState = &viewportState;
        gpci.pRasterizationState = &raster;
        gpci.pMultisampleState = &ms;
        gpci.pColorBlendState = &blend;
        gpci.pDynamicState = &dyn;
        gpci.layout = this.pipelineLayout;
        gpci.renderPass = this.renderPass;
        gpci.subpass = 0;
        gpci.basePipelineIndex = -1;

        trace("createPipeline: vkCreateGraphicsPipelines");
        let ok = vkOk(vkCreateGraphicsPipelines(this.device, nullptr, 1, &gpci, nullptr, &this.pipeline), "vkCreateGraphicsPipelines");

        trace("createPipeline: destroy shader modules");
        vkDestroyShaderModule(this.device, vert, nullptr);
        vkDestroyShaderModule(this.device, frag, nullptr);
        if (ok) printf("Created graphics pipeline\n");
        return ok;
    }

    createCommands() => int {
        trace("createCommands: command pool");
        let cpci: VkCommandPoolCreateInfo;
        cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cpci.queueFamilyIndex = this.queueFamily;
        if (!vkOk(vkCreateCommandPool(this.device, &cpci, nullptr, &this.commandPool), "vkCreateCommandPool")) return 0;

        trace("createCommands: command buffer");
        let cbai: VkCommandBufferAllocateInfo;
        cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.commandPool = this.commandPool;
        cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbai.commandBufferCount = 1;
        if (!vkOk(vkAllocateCommandBuffers(this.device, &cbai, &this.commandBuffer), "vkAllocateCommandBuffers")) return 0;

        trace("createCommands: semaphore");
        let semci: VkSemaphoreCreateInfo;
        semci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (!vkOk(vkCreateSemaphore(this.device, &semci, nullptr, &this.imageAvailable), "imageAvailable semaphore")) return 0;

        trace("createCommands: fence");
        let fci: VkFenceCreateInfo;
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (!vkOk(vkCreateFence(this.device, &fci, nullptr, &this.inFlight), "vkCreateFence")) return 0;
        return 1;
    }

    init(window: void*) => int {
        trace("init: start");
        this.window = window;
        trace("init: createInstance");
        if (!this.createInstance()) return 0;
        trace("init: createSurface");
        if (!this.createSurface()) return 0;
        trace("init: pickDevice");
        if (!this.pickDevice()) return 0;
        trace("init: createDevice");
        if (!this.createDevice()) return 0;
        trace("init: createRenderPass");
        if (!this.createRenderPass()) return 0;
        trace("init: createSwapchain");
        let r = this.createSwapchain();
        if (r == 0) return 0;
        trace("init: createPipeline");
        if (!this.createPipeline()) return 0;
        trace("init: createCommands");
        if (!this.createCommands()) return 0;
        trace("init: done");
        return 1;
    }

    drawFrame() => int {
        g_frame = g_frame + 1;
        traceFrame("begin");
        traceFrame("vkWaitForFences");
        vkWaitForFences(this.device, 1, &this.inFlight, VK_TRUE, U64_MAX);

        // Minimized or no swapchain yet: try to (re)build, otherwise skip
        if (this.swapchain == nullptr) {
            let rr = this.createSwapchain();
            if (rr == 0) return 0;
            if (rr == 2) { glfwWaitEvents(); return 1; }
        }

        traceFrame("vkAcquireNextImageKHR");
        let imageIndex: uint = 0;
        let r = vkAcquireNextImageKHR(this.device, this.swapchain, U64_MAX, this.imageAvailable, nullptr, &imageIndex);
        if (r == VK_ERROR_OUT_OF_DATE_KHR) {
            this.recreateSwapchain();
            return 1;
        }
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
            printf("vkAcquireNextImageKHR failed: %d\n", r);
            return 0;
        }

        traceFrame("reset fence + command buffer");
        vkResetFences(this.device, 1, &this.inFlight);
        vkResetCommandBuffer(this.commandBuffer, 0);

        // Record
        traceFrame("record: begin");
        let bi: VkCommandBufferBeginInfo;
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(this.commandBuffer, &bi);

        traceFrame("record: render pass");
        let clear: float[4] = [0.0, 0.0, 0.0, 1.0];
        let rpbi: VkRenderPassBeginInfo;
        rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpbi.renderPass = this.renderPass;
        rpbi.framebuffer = this.framebuffers[imageIndex];
        rpbi.renderArea.extent.width = this.extentW;
        rpbi.renderArea.extent.height = this.extentH;
        rpbi.clearValueCount = 1;
        rpbi.pClearValues = &clear;
        vkCmdBeginRenderPass(this.commandBuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(this.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this.pipeline);

        traceFrame("record: viewport/scissor");
        let vp: VkViewport;
        vp.width = this.extentW as float;
        vp.height = this.extentH as float;
        vp.maxDepth = 1.0;
        vkCmdSetViewport(this.commandBuffer, 0, 1, &vp);

        let sc: VkRect2D;
        sc.extent.width = this.extentW;
        sc.extent.height = this.extentH;
        vkCmdSetScissor(this.commandBuffer, 0, 1, &sc);

        traceFrame("record: push constants");
        let pc: PushConstants;
        pc.resX = this.extentW as float;
        pc.resY = this.extentH as float;
        pc.time = glfwGetTime() as float;
        vkCmdPushConstants(this.commandBuffer, this.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 12, &pc);

        traceFrame("record: draw + end");
        vkCmdDraw(this.commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(this.commandBuffer);
        vkEndCommandBuffer(this.commandBuffer);

        // Submit
        traceFrame("submit");
        let waitStage: uint = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        let si: VkSubmitInfo;
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = &this.imageAvailable;
        si.pWaitDstStageMask = &waitStage;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &this.commandBuffer;
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &this.renderFinished[imageIndex];
        if (!vkOk(vkQueueSubmit(this.queue, 1, &si, this.inFlight), "vkQueueSubmit")) return 0;

        // Present
        traceFrame("present");
        let presentInfo: VkPresentInfoKHR;
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &this.renderFinished[imageIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &this.swapchain;
        presentInfo.pImageIndices = &imageIndex;
        let pr = vkQueuePresentKHR(this.queue, &presentInfo);
        if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) {
            this.recreateSwapchain();
        } else if (pr != VK_SUCCESS) {
            printf("vkQueuePresentKHR failed: %d\n", pr);
            return 0;
        }
        traceFrame("end");
        return 1;
    }

    cleanup() => void {
        trace("cleanup");
        if (this.device != nullptr) {
            vkDeviceWaitIdle(this.device);
            this.destroySwapchain();
            vkDestroyFence(this.device, this.inFlight, nullptr);
            vkDestroySemaphore(this.device, this.imageAvailable, nullptr);
            vkDestroyCommandPool(this.device, this.commandPool, nullptr);
            vkDestroyPipeline(this.device, this.pipeline, nullptr);
            vkDestroyPipelineLayout(this.device, this.pipelineLayout, nullptr);
            vkDestroyRenderPass(this.device, this.renderPass, nullptr);
            vkDestroyDevice(this.device, nullptr);
        }
        if (this.surface != nullptr) vkDestroySurfaceKHR(this.instance, this.surface, nullptr);
        if (this.instance != nullptr) vkDestroyInstance(this.instance, nullptr);
        trace("cleanup done");
    }
}

let g_app: VulkanApp;

function main() => void {
    trace("main entered");
    glfwSetErrorCallback(glfwErrorCallback);
    trace("glfwInit");
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return;
    }

    // No OpenGL context, Vulkan manages the surface
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    trace("glfwCreateWindow");
    let window = glfwCreateWindow(800, 600, "Vulkan Star Field", nullptr, nullptr);
    if (window == nullptr) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return;
    }
    printf("Successfully created the window\n");
    fflush(nullptr);

    trace("g_app.init");
    if (!g_app.init(window)) {
        printf("Vulkan initialization failed\n");
        fflush(nullptr);
        g_app.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
        return;
    }

    trace("Starting main loop");
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (!g_app.drawFrame()) break;
    }

    g_app.cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    printf("Done\n");
    fflush(nullptr);
}

// standard/1/Vulkan.os
// Tier 6: complete Vulkan 1.1 renderer layer on top of c/vulkan.os + c/glfw.os.
//   instance (+ optional validation layer), GLFW surface, device, swapchain (+ recreate on resize),
//   render pass, framebuffers, frames in flight, SPIR-V shader modules (.spv files), graphics pipelines
//   (dynamic viewport/scissor, alpha blending, push constants), buffers + device memory (staging uploads),
//   RGBA8 textures (sampler + descriptor set), and a begin/draw/end frame loop.
// Multi-window: initCore() once + attach() per window (see below); init()/initEx() single-window use is unchanged.
// Not included: depth buffer, MSAA, mipmaps, uniform buffers, compute.
// Structs assume C layout (natural alignment, x86_64). No `return` inside loop bodies (compiler quirk).
module V = import "standard/1/c/vulkan.os";
module G = import "standard/1/c/glfw.os";
module S = import "standard/1/c/stdio.os";
module W = import "standard/1/Window.os";
module L = import "standard/1/List.os";
module CMem = import "standard/1/c/mem.os";

let VK_SUCCESS: int = 0;
let VK_SUBOPTIMAL_KHR: int = 1000001003;
let VK_ERROR_OUT_OF_DATE_KHR: int = -1000001004;
let MAX_FRAMES: u32 = 2;

// sType values
let VK_STRUCTURE_TYPE_APPLICATION_INFO: u32 = 0;
let VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO: u32 = 1;
let VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO: u32 = 2;
let VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO: u32 = 3;
let VK_STRUCTURE_TYPE_SUBMIT_INFO: u32 = 4;
let VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO: u32 = 5;
let VK_STRUCTURE_TYPE_FENCE_CREATE_INFO: u32 = 8;
let VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO: u32 = 9;
let VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO: u32 = 12;
let VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO: u32 = 14;
let VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO: u32 = 15;
let VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO: u32 = 16;
let VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO: u32 = 18;
let VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO: u32 = 19;
let VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO: u32 = 20;
let VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO: u32 = 22;
let VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO: u32 = 23;
let VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO: u32 = 24;
let VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO: u32 = 26;
let VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO: u32 = 27;
let VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO: u32 = 28;
let VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO: u32 = 30;
let VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO: u32 = 31;
let VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO: u32 = 32;
let VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO: u32 = 33;
let VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO: u32 = 34;
let VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET: u32 = 35;
let VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO: u32 = 37;
let VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO: u32 = 38;
let VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO: u32 = 39;
let VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO: u32 = 40;
let VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO: u32 = 42;
let VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO: u32 = 43;
let VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER: u32 = 45;
let VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR: u32 = 1000001000;
let VK_STRUCTURE_TYPE_PRESENT_INFO_KHR: u32 = 1000001001;

// enums / flags used by the helpers (public so callers can build their own buffers)
let VK_FORMAT_R32_SFLOAT: u32 = 100;
let VK_FORMAT_R32G32_SFLOAT: u32 = 103;
let VK_FORMAT_R32G32B32_SFLOAT: u32 = 106;
let VK_FORMAT_R32G32B32A32_SFLOAT: u32 = 109;
let VK_FORMAT_R8G8B8A8_SRGB: u32 = 43;
let VK_FORMAT_B8G8R8A8_SRGB: u32 = 50;
let VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: u32 = 0;
let VK_PRESENT_MODE_MAILBOX_KHR: u32 = 1;
let VK_PRESENT_MODE_FIFO_KHR: u32 = 2;
let VK_QUEUE_GRAPHICS_BIT: u32 = 1;
let VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT: u32 = 16;
let VK_FENCE_CREATE_SIGNALED_BIT: u32 = 1;
let VK_API_VERSION_1_1: u32 = 4198400;
let VK_BUFFER_USAGE_TRANSFER_SRC_BIT: u32 = 1;
let VK_BUFFER_USAGE_TRANSFER_DST_BIT: u32 = 2;
let VK_BUFFER_USAGE_INDEX_BUFFER_BIT: u32 = 64;
let VK_BUFFER_USAGE_VERTEX_BUFFER_BIT: u32 = 128;
let VK_MEMORY_DEVICE_LOCAL: u32 = 1;
let VK_MEMORY_HOST_VISIBLE_COHERENT: u32 = 6;
let VK_STAGE_VERTEX: u32 = 1;
let VK_STAGE_FRAGMENT: u32 = 16;
let VK_INDEX_U16: u32 = 0;
let VK_INDEX_U32: u32 = 1;

let LAYER_VALIDATION: char* = "VK_LAYER_KHRONOS_validation";

// ---------------------------------------------------------------------------------------------
// C structs
// ---------------------------------------------------------------------------------------------
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
struct VkExtent3D { width: u32; height: u32; depth: u32; }
struct VkOffset2D { x: int; y: int; }
struct VkOffset3D { x: int; y: int; z: int; }
struct VkRect2D { offset: VkOffset2D; extent: VkExtent2D; }
struct VkViewport { x: float; y: float; width: float; height: float; minDepth: float; maxDepth: float; }
struct VkClearColor { r: float; g: float; b: float; a: float; }
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
struct VkImageSubresourceLayers { aspectMask: u32; mipLevel: u32; baseArrayLayer: u32; layerCount: u32; }
struct VkImageViewCreateInfo {
    sType: u32; pNext: void*; flags: u32; image: u64; viewType: u32; format: u32;
    components: VkComponentMapping; subresourceRange: VkImageSubresourceRange;
}
struct VkSemaphoreCreateInfo { sType: u32; pNext: void*; flags: u32; }
struct VkFenceCreateInfo { sType: u32; pNext: void*; flags: u32; }
struct VkCommandPoolCreateInfo { sType: u32; pNext: void*; flags: u32; queueFamilyIndex: u32; }
struct VkCommandBufferAllocateInfo { sType: u32; pNext: void*; commandPool: u64; level: u32; commandBufferCount: u32; }
struct VkCommandBufferBeginInfo { sType: u32; pNext: void*; flags: u32; pInheritanceInfo: void*; }
struct VkAttachmentDescription {
    flags: u32; format: u32; samples: u32; loadOp: u32; storeOp: u32;
    stencilLoadOp: u32; stencilStoreOp: u32; initialLayout: u32; finalLayout: u32;
}
struct VkAttachmentReference { attachment: u32; layout: u32; }
struct VkSubpassDescription {
    flags: u32; pipelineBindPoint: u32; inputAttachmentCount: u32; pInputAttachments: void*;
    colorAttachmentCount: u32; pColorAttachments: VkAttachmentReference*; pResolveAttachments: void*;
    pDepthStencilAttachment: void*; preserveAttachmentCount: u32; pPreserveAttachments: u32*;
}
struct VkSubpassDependency {
    srcSubpass: u32; dstSubpass: u32; srcStageMask: u32; dstStageMask: u32;
    srcAccessMask: u32; dstAccessMask: u32; dependencyFlags: u32;
}
struct VkRenderPassCreateInfo {
    sType: u32; pNext: void*; flags: u32; attachmentCount: u32; pAttachments: VkAttachmentDescription*;
    subpassCount: u32; pSubpasses: VkSubpassDescription*; dependencyCount: u32; pDependencies: VkSubpassDependency*;
}
struct VkFramebufferCreateInfo {
    sType: u32; pNext: void*; flags: u32; renderPass: u64; attachmentCount: u32; pAttachments: u64*;
    width: u32; height: u32; layers: u32;
}
struct VkRenderPassBeginInfo {
    sType: u32; pNext: void*; renderPass: u64; framebuffer: u64; renderArea: VkRect2D;
    clearValueCount: u32; pClearValues: VkClearColor*;
}
struct VkShaderModuleCreateInfo { sType: u32; pNext: void*; flags: u32; codeSize: u64; pCode: u32*; }
struct VkPipelineShaderStageCreateInfo {
    sType: u32; pNext: void*; flags: u32; stage: u32; module: u64; pName: char*; pSpecializationInfo: void*;
}
struct VkVertexInputBindingDescription { binding: u32; stride: u32; inputRate: u32; }
struct VkVertexInputAttributeDescription { location: u32; binding: u32; format: u32; offset: u32; }
struct VkPipelineVertexInputStateCreateInfo {
    sType: u32; pNext: void*; flags: u32;
    vertexBindingDescriptionCount: u32; pVertexBindingDescriptions: VkVertexInputBindingDescription*;
    vertexAttributeDescriptionCount: u32; pVertexAttributeDescriptions: VkVertexInputAttributeDescription*;
}
struct VkPipelineInputAssemblyStateCreateInfo { sType: u32; pNext: void*; flags: u32; topology: u32; primitiveRestartEnable: u32; }
struct VkPipelineViewportStateCreateInfo {
    sType: u32; pNext: void*; flags: u32; viewportCount: u32; pViewports: VkViewport*; scissorCount: u32; pScissors: VkRect2D*;
}
struct VkPipelineRasterizationStateCreateInfo {
    sType: u32; pNext: void*; flags: u32; depthClampEnable: u32; rasterizerDiscardEnable: u32; polygonMode: u32;
    cullMode: u32; frontFace: u32; depthBiasEnable: u32; depthBiasConstantFactor: float; depthBiasClamp: float;
    depthBiasSlopeFactor: float; lineWidth: float;
}
struct VkPipelineMultisampleStateCreateInfo {
    sType: u32; pNext: void*; flags: u32; rasterizationSamples: u32; sampleShadingEnable: u32; minSampleShading: float;
    pSampleMask: u32*; alphaToCoverageEnable: u32; alphaToOneEnable: u32;
}
struct VkPipelineColorBlendAttachmentState {
    blendEnable: u32; srcColorBlendFactor: u32; dstColorBlendFactor: u32; colorBlendOp: u32;
    srcAlphaBlendFactor: u32; dstAlphaBlendFactor: u32; alphaBlendOp: u32; colorWriteMask: u32;
}
struct VkPipelineColorBlendStateCreateInfo {
    sType: u32; pNext: void*; flags: u32; logicOpEnable: u32; logicOp: u32; attachmentCount: u32;
    pAttachments: VkPipelineColorBlendAttachmentState*; bc0: float; bc1: float; bc2: float; bc3: float;
}
struct VkPipelineDynamicStateCreateInfo { sType: u32; pNext: void*; flags: u32; dynamicStateCount: u32; pDynamicStates: u32*; }
struct VkPushConstantRange { stageFlags: u32; offset: u32; size: u32; }
struct VkPipelineLayoutCreateInfo {
    sType: u32; pNext: void*; flags: u32; setLayoutCount: u32; pSetLayouts: u64*;
    pushConstantRangeCount: u32; pPushConstantRanges: VkPushConstantRange*;
}
struct VkGraphicsPipelineCreateInfo {
    sType: u32; pNext: void*; flags: u32; stageCount: u32; pStages: VkPipelineShaderStageCreateInfo*;
    pVertexInputState: void*; pInputAssemblyState: void*; pTessellationState: void*; pViewportState: void*;
    pRasterizationState: void*; pMultisampleState: void*; pDepthStencilState: void*; pColorBlendState: void*;
    pDynamicState: void*; layout: u64; renderPass: u64; subpass: u32; basePipelineHandle: u64; basePipelineIndex: int;
}
struct VkBufferCreateInfo {
    sType: u32; pNext: void*; flags: u32; size: u64; usage: u32; sharingMode: u32;
    queueFamilyIndexCount: u32; pQueueFamilyIndices: u32*;
}
struct VkMemoryRequirements { size: u64; alignment: u64; memoryTypeBits: u32; }
struct VkMemoryAllocateInfo { sType: u32; pNext: void*; allocationSize: u64; memoryTypeIndex: u32; }
struct VkBufferCopy { srcOffset: u64; dstOffset: u64; size: u64; }
struct VkImageCreateInfo {
    sType: u32; pNext: void*; flags: u32; imageType: u32; format: u32; extent: VkExtent3D; mipLevels: u32;
    arrayLayers: u32; samples: u32; tiling: u32; usage: u32; sharingMode: u32; queueFamilyIndexCount: u32;
    pQueueFamilyIndices: u32*; initialLayout: u32;
}
struct VkImageMemoryBarrier {
    sType: u32; pNext: void*; srcAccessMask: u32; dstAccessMask: u32; oldLayout: u32; newLayout: u32;
    srcQueueFamilyIndex: u32; dstQueueFamilyIndex: u32; image: u64; subresourceRange: VkImageSubresourceRange;
}
struct VkBufferImageCopy {
    bufferOffset: u64; bufferRowLength: u32; bufferImageHeight: u32; imageSubresource: VkImageSubresourceLayers;
    imageOffset: VkOffset3D; imageExtent: VkExtent3D;
}
struct VkSamplerCreateInfo {
    sType: u32; pNext: void*; flags: u32; magFilter: u32; minFilter: u32; mipmapMode: u32;
    addressModeU: u32; addressModeV: u32; addressModeW: u32; mipLodBias: float; anisotropyEnable: u32;
    maxAnisotropy: float; compareEnable: u32; compareOp: u32; minLod: float; maxLod: float;
    borderColor: u32; unnormalizedCoordinates: u32;
}
struct VkDescriptorSetLayoutBinding {
    binding: u32; descriptorType: u32; descriptorCount: u32; stageFlags: u32; pImmutableSamplers: void*;
}
struct VkDescriptorSetLayoutCreateInfo { sType: u32; pNext: void*; flags: u32; bindingCount: u32; pBindings: VkDescriptorSetLayoutBinding*; }
struct VkDescriptorPoolSize { kind: u32; descriptorCount: u32; }
struct VkDescriptorPoolCreateInfo {
    sType: u32; pNext: void*; flags: u32; maxSets: u32; poolSizeCount: u32; pPoolSizes: VkDescriptorPoolSize*;
}
struct VkDescriptorSetAllocateInfo { sType: u32; pNext: void*; descriptorPool: u64; descriptorSetCount: u32; pSetLayouts: u64*; }
struct VkDescriptorImageInfo { sampler: u64; imageView: u64; imageLayout: u32; }
struct VkWriteDescriptorSet {
    sType: u32; pNext: void*; dstSet: u64; dstBinding: u32; dstArrayElement: u32; descriptorCount: u32;
    descriptorType: u32; pImageInfo: VkDescriptorImageInfo*; pBufferInfo: void*; pTexelBufferView: void*;
}
struct VkSubmitInfo {
    sType: u32; pNext: void*; waitSemaphoreCount: u32; pWaitSemaphores: u64*; pWaitDstStageMask: u32*;
    commandBufferCount: u32; pCommandBuffers: void**; signalSemaphoreCount: u32; pSignalSemaphores: u64*;
}
struct VkPresentInfoKHR {
    sType: u32; pNext: void*; waitSemaphoreCount: u32; pWaitSemaphores: u64*; swapchainCount: u32;
    pSwapchains: u64*; pImageIndices: u32*; pResults: int*;
}

// ---------------------------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------------------------
function zero(p: void*, n: u64) => void { CMem.memset(p, 0, n); }

function forever() => u64 { let m: u64 = 0; m = m - 1; return m; }

function copyBytes(dst: u8*, src: u8*, n: u64) => void {
    let i: u64 = 0;
    while (i < n) { dst[i] = src[i]; i = i + 1; }
}

// Reads a whole file into malloc'd memory. Returns nullptr on failure; size goes to outSize[0].
function readFile(path: char*, outSize: u64*) => u8* {
    let f: void* = S.fopen(path, "rb");
    if (f == nullptr) return nullptr;
    S.fseek(f, 0, 2);
    let n: int = S.ftell(f);
    S.fseek(f, 0, 0);
    if (n <= 0) { S.fclose(f); return nullptr; }
    let buf = CMem.malloc(n as u64) as u8*;
    let got: u64 = S.fread(buf as void*, 1, n as u64, f);
    S.fclose(f);
    if (got != (n as u64)) { CMem.free(buf as void*); return nullptr; }
    outSize[0] = n as u64;
    return buf;
}

function layerPresent(name: char*) => bool {
    let n: u32 = 0;
    V.vkEnumerateInstanceLayerProperties(&n, nullptr);
    if (n == 0) return false;
    let props = CMem.malloc((n as u64) * 520) as u8*;     // VkLayerProperties = 520 bytes
    V.vkEnumerateInstanceLayerProperties(&n, props as void*);
    let found: bool = false;
    let i: u32 = 0;
    while (i < n && !found) {
        let base: u64 = (i as u64) * 520;
        let same: bool = true;
        let more: bool = true;
        let k: u64 = 0;
        while (more) {
            let a: u8 = props[base + k];
            let b: u8 = name[k] as u8;
            if (a != b) { same = false; more = false; }
            if (a == 0) { more = false; }
            k = k + 1;
        }
        if (same) found = true;
        i = i + 1;
    }
    CMem.free(props as void*);
    return found;
}

// ---------------------------------------------------------------------------------------------
// resources
// ---------------------------------------------------------------------------------------------
struct Buffer { buf: u64 = 0; mem: u64 = 0; size: u64 = 0; }
struct Texture { image: u64 = 0; mem: u64 = 0; view: u64 = 0; set: u64 = 0; width: int = 0; height: int = 0; }
struct Pipeline { handle: u64 = 0; layout: u64 = 0; pushSize: u32 = 0; textured: bool = false; }

struct Vulkan {
    winHandle: void* = nullptr;
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
    fbs: L.List<u64>;
    renderPass: u64 = 0;
    pool: u64 = 0;
    cmds: void** = nullptr;
    imageAvailable: u64* = nullptr;      // MAX_FRAMES
    inFlight: u64* = nullptr;            // MAX_FRAMES
    renderFinished: u64* = nullptr;      // one per swapchain image
    rfCount: u32 = 0;
    frame: u32 = 0;
    curImage: u32 = 0;
    curCmd: void* = nullptr;
    inFrame: bool = false;
    vsync: bool = true;
    validation: bool = false;
    clearR: float = 0.05; clearG: float = 0.05; clearB: float = 0.08; clearA: float = 1.0;
    memProps: u32* = nullptr;            // raw VkPhysicalDeviceMemoryProperties (520 bytes) as u32s
    descPool: u64 = 0;
    texLayout: u64 = 0;
    sampler: u64 = 0;
    owner: bool = true;                  // false for targets made by attach(): they borrow the core's device

    // ---- shared core + per-window targets (multi-window) -------------------------------------
    // initCore: instance + device + queue + memory props + descriptor pool/sampler + command pool, NO swapchain.
    // `probe` is any GLFW no-API window; it is only used to pick a queue family that can present.
    // Then every window (the first too) calls attach(core, handle). Free targets first, core last.
    initCore(probe: void*, appName: char*, validate: bool) => bool {
        this.winHandle = probe;
        if (G.glfwVulkanSupported() == 0) return false;
        let useLayer: bool = false;
        if (validate) useLayer = layerPresent(LAYER_VALIDATION);
        this.validation = useLayer;
        let app: VkApplicationInfo;
        zero(&app as void*, sizeof(VkApplicationInfo));
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; app.pApplicationName = appName; app.pEngineName = "omniscript"; app.apiVersion = VK_API_VERSION_1_1;
        let extCount: u32 = 0;
        let exts: char** = G.glfwGetRequiredInstanceExtensions(&extCount);
        let layer: char* = LAYER_VALIDATION;
        let ici: VkInstanceCreateInfo;
        zero(&ici as void*, sizeof(VkInstanceCreateInfo));
        ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; ici.pApplicationInfo = &app;
        ici.enabledExtensionCount = extCount; ici.ppEnabledExtensionNames = exts;
        if (useLayer) { ici.enabledLayerCount = 1; ici.ppEnabledLayerNames = &layer; }
        if (V.vkCreateInstance(&ici as void*, nullptr, &this.instance) != VK_SUCCESS) return false;
        if (G.glfwCreateWindowSurface(this.instance, probe, nullptr, &this.surface) != VK_SUCCESS) return false;
        if (!this.pickDevice()) return false;
        if (!this.createDevice(useLayer)) return false;
        V.vkDestroySurfaceKHR(this.instance, this.surface, nullptr);     // probe surface not needed any more
        this.surface = 0;
        this.memProps = CMem.malloc(520) as u32*;
        V.vkGetPhysicalDeviceMemoryProperties(this.phys, this.memProps as void*);
        if (!this.createDescriptors()) return false;
        let pci: VkCommandPoolCreateInfo; zero(&pci as void*, sizeof(VkCommandPoolCreateInfo));
        pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; pci.flags = 2; pci.queueFamilyIndex = this.queueFamily;
        return V.vkCreateCommandPool(this.device, &pci as void*, nullptr, &this.pool) == VK_SUCCESS;
    }

    // Makes this struct a presentation target for `win` on core's device. After true, use it exactly like a
    // Vulkan from init(): pipelines, beginFrame/endFrame, createTexture (descriptor sets come from the core).
    attach(core: Vulkan*, win: void*) => bool {
        this.owner = false;
        this.winHandle = win;
        this.instance = core.instance; this.phys = core.phys; this.device = core.device;
        this.queue = core.queue; this.queueFamily = core.queueFamily; this.memProps = core.memProps;
        this.descPool = core.descPool; this.texLayout = core.texLayout; this.sampler = core.sampler;
        this.vsync = core.vsync;
        if (G.glfwCreateWindowSurface(this.instance, win, nullptr, &this.surface) != VK_SUCCESS) return false;
        let present: u32 = 0;
        V.vkGetPhysicalDeviceSurfaceSupportKHR(this.phys, this.queueFamily, this.surface, &present);
        if (present == 0) return false;
        if (!this.createSwapchain()) return false;
        if (!this.createRenderPass()) return false;
        if (!this.createFramebuffers()) return false;
        if (!this.createSync()) return false;
        return this.allocCommandBuffers();
    }

    // ---- bring-up ----------------------------------------------------------------------------
    // Window must be opened with opengl=false. Returns false on first failure.
    init(win: W.Window*, appName: char*) => bool { return this.initRaw(win.h, appName, false); }
    initEx(win: W.Window*, appName: char*, validate: bool) => bool { return this.initRaw(win.h, appName, validate); }

    // win = raw GLFWwindow* created with GLFW_CLIENT_API = GLFW_NO_API
    initRaw(win: void*, appName: char*, validate: bool) => bool {
        this.winHandle = win;
        if (G.glfwVulkanSupported() == 0) return false;
        let useLayer: bool = false;
        if (validate) useLayer = layerPresent(LAYER_VALIDATION);
        this.validation = useLayer;

        let app: VkApplicationInfo;
        zero(&app as void*, sizeof(VkApplicationInfo));
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; app.pApplicationName = appName; app.pEngineName = "omniscript"; app.apiVersion = VK_API_VERSION_1_1;
        let extCount: u32 = 0;
        let exts: char** = G.glfwGetRequiredInstanceExtensions(&extCount);
        let layer: char* = LAYER_VALIDATION;
        let ici: VkInstanceCreateInfo;
        zero(&ici as void*, sizeof(VkInstanceCreateInfo));
        ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; ici.pApplicationInfo = &app;
        ici.enabledExtensionCount = extCount; ici.ppEnabledExtensionNames = exts;
        if (useLayer) { ici.enabledLayerCount = 1; ici.ppEnabledLayerNames = &layer; }
        if (V.vkCreateInstance(&ici as void*, nullptr, &this.instance) != VK_SUCCESS) return false;

        if (G.glfwCreateWindowSurface(this.instance, win, nullptr, &this.surface) != VK_SUCCESS) return false;
        if (!this.pickDevice()) return false;
        if (!this.createDevice(useLayer)) return false;
        this.memProps = CMem.malloc(520) as u32*;
        V.vkGetPhysicalDeviceMemoryProperties(this.phys, this.memProps as void*);
        if (!this.createSwapchain()) return false;
        if (!this.createRenderPass()) return false;
        if (!this.createFramebuffers()) return false;
        if (!this.createSync()) return false;
        if (!this.createDescriptors()) return false;
        return this.allocCommandBuffers();
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

    createDevice(useLayer: bool) => bool {
        let prio: float = 1.0;
        let qci: VkDeviceQueueCreateInfo;
        zero(&qci as void*, sizeof(VkDeviceQueueCreateInfo));
        qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; qci.queueFamilyIndex = this.queueFamily; qci.queueCount = 1; qci.pQueuePriorities = &prio;
        let ext: char* = "VK_KHR_swapchain";
        let layer: char* = LAYER_VALIDATION;
        let dci: VkDeviceCreateInfo;
        zero(&dci as void*, sizeof(VkDeviceCreateInfo));
        dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; dci.queueCreateInfoCount = 1; dci.pQueueCreateInfos = &qci;
        dci.enabledExtensionCount = 1; dci.ppEnabledExtensionNames = &ext;
        if (useLayer) { dci.enabledLayerCount = 1; dci.ppEnabledLayerNames = &layer; }
        if (V.vkCreateDevice(this.phys, &dci as void*, nullptr, &this.device) != VK_SUCCESS) return false;
        V.vkGetDeviceQueue(this.device, this.queueFamily, 0, &this.queue);
        return true;
    }

    // ---- swapchain ---------------------------------------------------------------------------
    // Creates (or re-creates, using the old handle) the swapchain, its image views and the
    // per-image renderFinished semaphores. Framebuffers are built by createFramebuffers().
    createSwapchain() => bool {
        let caps: VkSurfaceCapabilitiesKHR;
        V.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this.phys, this.surface, &caps as void*);

        let fmtCount: u32 = 0;
        V.vkGetPhysicalDeviceSurfaceFormatsKHR(this.phys, this.surface, &fmtCount, nullptr);
        let fmts = CMem.malloc((fmtCount as u64) * sizeof(VkSurfaceFormatKHR)) as VkSurfaceFormatKHR*;
        V.vkGetPhysicalDeviceSurfaceFormatsKHR(this.phys, this.surface, &fmtCount, fmts as void*);
        let chosen: VkSurfaceFormatKHR = fmts[0];
        let i: u32 = 0;
        while (i < fmtCount) { if (fmts[i].format == VK_FORMAT_B8G8R8A8_SRGB && fmts[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) chosen = fmts[i]; i = i + 1; }
        CMem.free(fmts as void*);
        this.format = chosen.format;

        let mode: u32 = VK_PRESENT_MODE_FIFO_KHR;
        if (!this.vsync) {
            let pmCount: u32 = 0;
            V.vkGetPhysicalDeviceSurfacePresentModesKHR(this.phys, this.surface, &pmCount, nullptr);
            let pms = CMem.malloc((pmCount as u64) * 4) as u32*;
            V.vkGetPhysicalDeviceSurfacePresentModesKHR(this.phys, this.surface, &pmCount, pms);
            let p: u32 = 0;
            while (p < pmCount) { if (pms[p] == VK_PRESENT_MODE_MAILBOX_KHR) mode = VK_PRESENT_MODE_MAILBOX_KHR; p = p + 1; }
            CMem.free(pms as void*);
        }

        this.extent = caps.currentExtent;
        if (this.extent.width == 0xFFFFFFFF) {
            let w: int = 0; let h: int = 0;
            G.glfwGetFramebufferSize(this.winHandle, &w, &h);
            this.extent.width = w as u32; this.extent.height = h as u32;
        }
        let count: u32 = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && count > caps.maxImageCount) count = caps.maxImageCount;

        let sci: VkSwapchainCreateInfoKHR;
        zero(&sci as void*, sizeof(VkSwapchainCreateInfoKHR));
        sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; sci.surface = this.surface; sci.minImageCount = count;
        sci.imageFormat = chosen.format; sci.imageColorSpace = chosen.colorSpace; sci.imageExtent = this.extent;
        sci.imageArrayLayers = 1; sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; sci.imageSharingMode = 0;
        sci.preTransform = caps.currentTransform; sci.compositeAlpha = 1; sci.presentMode = mode; sci.clipped = 1;
        sci.oldSwapchain = this.swapchain;
        let fresh: u64 = 0;
        if (V.vkCreateSwapchainKHR(this.device, &sci as void*, nullptr, &fresh) != VK_SUCCESS) return false;
        if (this.swapchain != 0) V.vkDestroySwapchainKHR(this.device, this.swapchain, nullptr);
        this.swapchain = fresh;

        let n: u32 = 0;
        V.vkGetSwapchainImagesKHR(this.device, this.swapchain, &n, nullptr);
        this.images.len = 0;
        this.images.reserve(n as u64);
        V.vkGetSwapchainImagesKHR(this.device, this.swapchain, &n, this.images.data);
        this.images.len = n as u64;

        this.views.len = 0;
        let ok: bool = true;
        let k: u32 = 0;
        while (k < n && ok) {
            let vci: VkImageViewCreateInfo;
            zero(&vci as void*, sizeof(VkImageViewCreateInfo));
            vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; vci.image = this.images.data[k]; vci.viewType = 1; vci.format = this.format;
            vci.subresourceRange.aspectMask = 1; vci.subresourceRange.levelCount = 1; vci.subresourceRange.layerCount = 1;
            let view: u64 = 0;
            if (V.vkCreateImageView(this.device, &vci as void*, nullptr, &view) != VK_SUCCESS) { ok = false; }
            if (ok) this.views.push(view);
            k = k + 1;
        }
        if (!ok) return false;

        // renderFinished semaphores: one per image
        this.freeRenderSemaphores();
        this.rfCount = n;
        this.renderFinished = CMem.malloc((n as u64) * 8) as u64*;
        let sem: VkSemaphoreCreateInfo; zero(&sem as void*, sizeof(VkSemaphoreCreateInfo));
        sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        let s: u32 = 0;
        while (s < n && ok) {
            let h: u64 = 0;
            if (V.vkCreateSemaphore(this.device, &sem as void*, nullptr, &h) != VK_SUCCESS) { ok = false; }
            this.renderFinished[s] = h;
            s = s + 1;
        }
        return ok;
    }

    freeRenderSemaphores() => void {
        if (this.renderFinished == nullptr) return;
        let i: u32 = 0;
        while (i < this.rfCount) {
            if (this.renderFinished[i] != 0) V.vkDestroySemaphore(this.device, this.renderFinished[i], nullptr);
            i = i + 1;
        }
        CMem.free(this.renderFinished as void*);
        this.renderFinished = nullptr;
        this.rfCount = 0;
    }

    createRenderPass() => bool {
        let att: VkAttachmentDescription;
        zero(&att as void*, sizeof(VkAttachmentDescription));
        att.format = this.format; att.samples = 1; att.loadOp = 1; att.storeOp = 0;
        att.stencilLoadOp = 2; att.stencilStoreOp = 1; att.initialLayout = 0; att.finalLayout = 1000001002;
        let ref: VkAttachmentReference; ref.attachment = 0; ref.layout = 2;
        let sub: VkSubpassDescription;
        zero(&sub as void*, sizeof(VkSubpassDescription));
        sub.pipelineBindPoint = 0; sub.colorAttachmentCount = 1; sub.pColorAttachments = &ref;
        let dep: VkSubpassDependency;
        zero(&dep as void*, sizeof(VkSubpassDependency));
        dep.srcSubpass = 0xFFFFFFFF; dep.dstSubpass = 0; dep.srcStageMask = 1024; dep.dstStageMask = 1024;
        dep.srcAccessMask = 0; dep.dstAccessMask = 256;
        let rci: VkRenderPassCreateInfo;
        zero(&rci as void*, sizeof(VkRenderPassCreateInfo));
        rci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; rci.attachmentCount = 1; rci.pAttachments = &att;
        rci.subpassCount = 1; rci.pSubpasses = &sub; rci.dependencyCount = 1; rci.pDependencies = &dep;
        return V.vkCreateRenderPass(this.device, &rci as void*, nullptr, &this.renderPass) == VK_SUCCESS;
    }

    createFramebuffers() => bool {
        this.fbs.len = 0;
        let ok: bool = true;
        let i: u64 = 0;
        while (i < this.views.len && ok) {
            let view: u64 = this.views.data[i];
            let fci: VkFramebufferCreateInfo;
            zero(&fci as void*, sizeof(VkFramebufferCreateInfo));
            fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO; fci.renderPass = this.renderPass;
            fci.attachmentCount = 1; fci.pAttachments = &view;
            fci.width = this.extent.width; fci.height = this.extent.height; fci.layers = 1;
            let fb: u64 = 0;
            if (V.vkCreateFramebuffer(this.device, &fci as void*, nullptr, &fb) != VK_SUCCESS) { ok = false; }
            if (ok) this.fbs.push(fb);
            i = i + 1;
        }
        return ok;
    }

    destroyFramebuffersAndViews() => void {
        let i: u64 = 0;
        while (i < this.fbs.len) { V.vkDestroyFramebuffer(this.device, this.fbs.data[i], nullptr); i = i + 1; }
        this.fbs.len = 0;
        i = 0;
        while (i < this.views.len) { V.vkDestroyImageView(this.device, this.views.data[i], nullptr); i = i + 1; }
        this.views.len = 0;
    }

    // Called automatically on resize / out-of-date. Pipelines stay valid (dynamic viewport/scissor).
    recreateSwapchain() => bool {
        V.vkDeviceWaitIdle(this.device);
        this.destroyFramebuffersAndViews();
        if (!this.createSwapchain()) return false;
        return this.createFramebuffers();
    }

    // ---- sync + commands + descriptors -------------------------------------------------------
    createSync() => bool {
        let sci: VkSemaphoreCreateInfo; zero(&sci as void*, sizeof(VkSemaphoreCreateInfo));
        sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        let fci: VkFenceCreateInfo; zero(&fci as void*, sizeof(VkFenceCreateInfo));
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO; fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        let pci: VkCommandPoolCreateInfo; zero(&pci as void*, sizeof(VkCommandPoolCreateInfo));
        pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; pci.flags = 2; pci.queueFamilyIndex = this.queueFamily;
        this.imageAvailable = CMem.malloc((MAX_FRAMES as u64) * 8) as u64*;
        this.inFlight = CMem.malloc((MAX_FRAMES as u64) * 8) as u64*;
        let ok: bool = true;
        let i: u32 = 0;
        while (i < MAX_FRAMES && ok) {
            let s: u64 = 0; let f: u64 = 0;
            if (V.vkCreateSemaphore(this.device, &sci as void*, nullptr, &s) != VK_SUCCESS) { ok = false; }
            if (V.vkCreateFence(this.device, &fci as void*, nullptr, &f) != VK_SUCCESS) { ok = false; }
            this.imageAvailable[i] = s; this.inFlight[i] = f;
            i = i + 1;
        }
        if (!ok) return false;
        return V.vkCreateCommandPool(this.device, &pci as void*, nullptr, &this.pool) == VK_SUCCESS;
    }

    allocCommandBuffers() => bool {
        this.cmds = CMem.malloc((MAX_FRAMES as u64) * 8) as void**;
        let ai: VkCommandBufferAllocateInfo; zero(&ai as void*, sizeof(VkCommandBufferAllocateInfo));
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; ai.commandPool = this.pool; ai.level = 0; ai.commandBufferCount = MAX_FRAMES;
        return V.vkAllocateCommandBuffers(this.device, &ai as void*, this.cmds) == VK_SUCCESS;
    }

    // One combined-image-sampler set layout (set 0, binding 0, fragment stage) + a linear/repeat sampler.
    createDescriptors() => bool {
        let b: VkDescriptorSetLayoutBinding; zero(&b as void*, sizeof(VkDescriptorSetLayoutBinding));
        b.binding = 0; b.descriptorType = 1; b.descriptorCount = 1; b.stageFlags = VK_STAGE_FRAGMENT;
        let lci: VkDescriptorSetLayoutCreateInfo; zero(&lci as void*, sizeof(VkDescriptorSetLayoutCreateInfo));
        lci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO; lci.bindingCount = 1; lci.pBindings = &b;
        if (V.vkCreateDescriptorSetLayout(this.device, &lci as void*, nullptr, &this.texLayout) != VK_SUCCESS) return false;
        let ps: VkDescriptorPoolSize; ps.kind = 1; ps.descriptorCount = 256;
        let pci: VkDescriptorPoolCreateInfo; zero(&pci as void*, sizeof(VkDescriptorPoolCreateInfo));
        pci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO; pci.maxSets = 256; pci.poolSizeCount = 1; pci.pPoolSizes = &ps;
        if (V.vkCreateDescriptorPool(this.device, &pci as void*, nullptr, &this.descPool) != VK_SUCCESS) return false;
        let sa: VkSamplerCreateInfo; zero(&sa as void*, sizeof(VkSamplerCreateInfo));
        sa.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO; sa.magFilter = 1; sa.minFilter = 1; sa.mipmapMode = 1;
        sa.addressModeU = 0; sa.addressModeV = 0; sa.addressModeW = 0; sa.maxAnisotropy = 1.0; sa.maxLod = 0.0; sa.borderColor = 0;
        return V.vkCreateSampler(this.device, &sa as void*, nullptr, &this.sampler) == VK_SUCCESS;
    }

    // ---- shaders + pipelines -----------------------------------------------------------------
    shaderFromWords(code: u32*, bytes: u64) => u64 {
        let ci: VkShaderModuleCreateInfo; zero(&ci as void*, sizeof(VkShaderModuleCreateInfo));
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO; ci.codeSize = bytes; ci.pCode = code;
        let m: u64 = 0;
        if (V.vkCreateShaderModule(this.device, &ci as void*, nullptr, &m) != VK_SUCCESS) return 0;
        return m;
    }

    // Loads compiled SPIR-V (glslangValidator -V x.vert -o x.vert.spv). Returns 0 on failure.
    loadShader(path: char*) => u64 {
        let size: u64 = 0;
        let data: u8* = readFile(path, &size);
        if (data == nullptr) return 0;
        let m: u64 = this.shaderFromWords(data as u32*, size);
        CMem.free(data as void*);
        return m;
    }

    destroyShader(m: u64) => void { if (m != 0) V.vkDestroyShaderModule(this.device, m, nullptr); }

    // attrFormats/attrOffsets: one entry per vertex attribute (location = index), all from binding 0.
    // stride 0 / attrCount 0 = no vertex buffer (vertices generated in the shader).
    // textured: pipeline layout gets the sampler set (set 0). pushBytes > 0: push constants (vertex|fragment).
    createPipeline(out: Pipeline*, vs: u64, fs: u64, stride: u32, attrCount: u32, attrFormats: u32*, attrOffsets: u32*, textured: bool, pushBytes: u32) => bool {
        let stages = CMem.malloc(2 * sizeof(VkPipelineShaderStageCreateInfo)) as VkPipelineShaderStageCreateInfo*;
        zero(stages as void*, 2 * sizeof(VkPipelineShaderStageCreateInfo));
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; stages[0].stage = VK_STAGE_VERTEX; stages[0].module = vs; stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; stages[1].stage = VK_STAGE_FRAGMENT; stages[1].module = fs; stages[1].pName = "main";

        let bind: VkVertexInputBindingDescription; bind.binding = 0; bind.stride = stride; bind.inputRate = 0;
        let attrs = CMem.malloc(((attrCount + 1) as u64) * sizeof(VkVertexInputAttributeDescription)) as VkVertexInputAttributeDescription*;
        let a: u32 = 0;
        while (a < attrCount) {
            attrs[a].location = a; attrs[a].binding = 0; attrs[a].format = attrFormats[a]; attrs[a].offset = attrOffsets[a];
            a = a + 1;
        }
        let vi: VkPipelineVertexInputStateCreateInfo; zero(&vi as void*, sizeof(VkPipelineVertexInputStateCreateInfo));
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        if (attrCount > 0) {
            vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &bind;
            vi.vertexAttributeDescriptionCount = attrCount; vi.pVertexAttributeDescriptions = attrs;
        }
        let ia: VkPipelineInputAssemblyStateCreateInfo; zero(&ia as void*, sizeof(VkPipelineInputAssemblyStateCreateInfo));
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; ia.topology = 3;
        let vp: VkPipelineViewportStateCreateInfo; zero(&vp as void*, sizeof(VkPipelineViewportStateCreateInfo));
        vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; vp.viewportCount = 1; vp.scissorCount = 1;
        let rs: VkPipelineRasterizationStateCreateInfo; zero(&rs as void*, sizeof(VkPipelineRasterizationStateCreateInfo));
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO; rs.polygonMode = 0; rs.cullMode = 0; rs.frontFace = 1; rs.lineWidth = 1.0;
        let ms: VkPipelineMultisampleStateCreateInfo; zero(&ms as void*, sizeof(VkPipelineMultisampleStateCreateInfo));
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO; ms.rasterizationSamples = 1;
        let cba: VkPipelineColorBlendAttachmentState;
        cba.blendEnable = 1; cba.srcColorBlendFactor = 6; cba.dstColorBlendFactor = 7; cba.colorBlendOp = 0;
        cba.srcAlphaBlendFactor = 1; cba.dstAlphaBlendFactor = 7; cba.alphaBlendOp = 0; cba.colorWriteMask = 15;
        let cb: VkPipelineColorBlendStateCreateInfo; zero(&cb as void*, sizeof(VkPipelineColorBlendStateCreateInfo));
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; cb.attachmentCount = 1; cb.pAttachments = &cba;
        let dyn = CMem.malloc(8) as u32*; dyn[0] = 0; dyn[1] = 1;      // VIEWPORT, SCISSOR
        let ds: VkPipelineDynamicStateCreateInfo; zero(&ds as void*, sizeof(VkPipelineDynamicStateCreateInfo));
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO; ds.dynamicStateCount = 2; ds.pDynamicStates = dyn;

        let pcr: VkPushConstantRange; pcr.stageFlags = 17; pcr.offset = 0; pcr.size = pushBytes;
        let plci: VkPipelineLayoutCreateInfo; zero(&plci as void*, sizeof(VkPipelineLayoutCreateInfo));
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        if (textured) { plci.setLayoutCount = 1; plci.pSetLayouts = &this.texLayout; }
        if (pushBytes > 0) { plci.pushConstantRangeCount = 1; plci.pPushConstantRanges = &pcr; }
        let layout: u64 = 0;
        let ok: bool = V.vkCreatePipelineLayout(this.device, &plci as void*, nullptr, &layout) == VK_SUCCESS;

        let handle: u64 = 0;
        if (ok) {
            let gpci: VkGraphicsPipelineCreateInfo; zero(&gpci as void*, sizeof(VkGraphicsPipelineCreateInfo));
            gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; gpci.stageCount = 2; gpci.pStages = stages;
            gpci.pVertexInputState = &vi as void*; gpci.pInputAssemblyState = &ia as void*; gpci.pViewportState = &vp as void*;
            gpci.pRasterizationState = &rs as void*; gpci.pMultisampleState = &ms as void*; gpci.pColorBlendState = &cb as void*;
            gpci.pDynamicState = &ds as void*; gpci.layout = layout; gpci.renderPass = this.renderPass; gpci.subpass = 0;
            gpci.basePipelineIndex = -1;
            ok = V.vkCreateGraphicsPipelines(this.device, 0, 1, &gpci as void*, nullptr, &handle) == VK_SUCCESS;
        }
        CMem.free(stages as void*); CMem.free(attrs as void*); CMem.free(dyn as void*);
        if (!ok) { if (layout != 0) V.vkDestroyPipelineLayout(this.device, layout, nullptr); return false; }
        out.handle = handle; out.layout = layout; out.pushSize = pushBytes; out.textured = textured;
        return true;
    }

    destroyPipeline(p: Pipeline*) => void {
        if (p.handle != 0) V.vkDestroyPipeline(this.device, p.handle, nullptr);
        if (p.layout != 0) V.vkDestroyPipelineLayout(this.device, p.layout, nullptr);
        p.handle = 0; p.layout = 0;
    }

    // ---- memory + buffers --------------------------------------------------------------------
    findMemoryType(typeBits: u32, want: u32) => u32 {
        let n: u32 = this.memProps[0];
        let r: u32 = 0xFFFFFFFF;
        let i: u32 = 0;
        while (i < n && r == 0xFFFFFFFF) {
            let bit: u32 = 1;
            bit = bit << i;
            if ((typeBits & bit) != 0 && (this.memProps[1 + i * 2] & want) == want) r = i;
            i = i + 1;
        }
        return r;
    }

    createBuffer(out: Buffer*, size: u64, usage: u32, memFlags: u32) => bool {
        let bci: VkBufferCreateInfo; zero(&bci as void*, sizeof(VkBufferCreateInfo));
        bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; bci.size = size; bci.usage = usage; bci.sharingMode = 0;
        let b: u64 = 0;
        if (V.vkCreateBuffer(this.device, &bci as void*, nullptr, &b) != VK_SUCCESS) return false;
        let mr: VkMemoryRequirements;
        V.vkGetBufferMemoryRequirements(this.device, b, &mr as void*);
        let t: u32 = this.findMemoryType(mr.memoryTypeBits, memFlags);
        if (t == 0xFFFFFFFF) { V.vkDestroyBuffer(this.device, b, nullptr); return false; }
        let mai: VkMemoryAllocateInfo; zero(&mai as void*, sizeof(VkMemoryAllocateInfo));
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; mai.allocationSize = mr.size; mai.memoryTypeIndex = t;
        let mem: u64 = 0;
        if (V.vkAllocateMemory(this.device, &mai as void*, nullptr, &mem) != VK_SUCCESS) { V.vkDestroyBuffer(this.device, b, nullptr); return false; }
        V.vkBindBufferMemory(this.device, b, mem, 0);
        out.buf = b; out.mem = mem; out.size = size;
        return true;
    }

    destroyBuffer(b: Buffer*) => void {
        if (b.buf != 0) V.vkDestroyBuffer(this.device, b.buf, nullptr);
        if (b.mem != 0) V.vkFreeMemory(this.device, b.mem, nullptr);
        b.buf = 0; b.mem = 0; b.size = 0;
    }

    // Copy into a host-visible buffer (createBuffer with VK_MEMORY_HOST_VISIBLE_COHERENT).
    writeBuffer(b: Buffer*, data: void*, size: u64) => bool {
        let p: void* = nullptr;
        if (V.vkMapMemory(this.device, b.mem, 0, size, 0, &p) != VK_SUCCESS) return false;
        copyBytes(p as u8*, data as u8*, size);
        V.vkUnmapMemory(this.device, b.mem);
        return true;
    }

    beginOneTime() => void* {
        let ai: VkCommandBufferAllocateInfo; zero(&ai as void*, sizeof(VkCommandBufferAllocateInfo));
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; ai.commandPool = this.pool; ai.level = 0; ai.commandBufferCount = 1;
        let cb: void* = nullptr;
        if (V.vkAllocateCommandBuffers(this.device, &ai as void*, &cb) != VK_SUCCESS) return nullptr;
        let bi: VkCommandBufferBeginInfo; zero(&bi as void*, sizeof(VkCommandBufferBeginInfo));
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; bi.flags = 1;
        V.vkBeginCommandBuffer(cb, &bi as void*);
        return cb;
    }

    endOneTime(cb: void*) => void {
        V.vkEndCommandBuffer(cb);
        let si: VkSubmitInfo; zero(&si as void*, sizeof(VkSubmitInfo));
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; si.commandBufferCount = 1; si.pCommandBuffers = &cb;
        V.vkQueueSubmit(this.queue, 1, &si as void*, 0);
        V.vkQueueWaitIdle(this.queue);
        V.vkFreeCommandBuffers(this.device, this.pool, 1, &cb);
    }

    // Static GPU buffer: staged upload into device-local memory. usage = VERTEX/INDEX bit.
    createDeviceBuffer(out: Buffer*, data: void*, size: u64, usage: u32) => bool {
        let staging: Buffer;
        if (!this.createBuffer(&staging, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_HOST_VISIBLE_COHERENT)) return false;
        if (!this.writeBuffer(&staging, data, size)) { this.destroyBuffer(&staging); return false; }
        if (!this.createBuffer(out, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_DEVICE_LOCAL)) { this.destroyBuffer(&staging); return false; }
        let cb: void* = this.beginOneTime();
        if (cb == nullptr) { this.destroyBuffer(&staging); this.destroyBuffer(out); return false; }
        let region: VkBufferCopy; region.srcOffset = 0; region.dstOffset = 0; region.size = size;
        V.vkCmdCopyBuffer(cb, staging.buf, out.buf, 1, &region as void*);
        this.endOneTime(cb);
        this.destroyBuffer(&staging);
        return true;
    }

    // ---- textures ----------------------------------------------------------------------------
    // RGBA8 sRGB texture, sampler + descriptor set ready for bindTexture().
    createTexture(out: Texture*, px: u8*, w: int, h: int) => bool {
        let size: u64 = (w as u64) * (h as u64) * 4;
        let staging: Buffer;
        if (!this.createBuffer(&staging, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_HOST_VISIBLE_COHERENT)) return false;
        if (!this.writeBuffer(&staging, px as void*, size)) { this.destroyBuffer(&staging); return false; }

        let ici: VkImageCreateInfo; zero(&ici as void*, sizeof(VkImageCreateInfo));
        ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO; ici.imageType = 1; ici.format = VK_FORMAT_R8G8B8A8_SRGB;
        ici.extent.width = w as u32; ici.extent.height = h as u32; ici.extent.depth = 1;
        ici.mipLevels = 1; ici.arrayLayers = 1; ici.samples = 1; ici.tiling = 0; ici.usage = 6; ici.sharingMode = 0; ici.initialLayout = 0;
        let img: u64 = 0;
        if (V.vkCreateImage(this.device, &ici as void*, nullptr, &img) != VK_SUCCESS) { this.destroyBuffer(&staging); return false; }
        let mr: VkMemoryRequirements;
        V.vkGetImageMemoryRequirements(this.device, img, &mr as void*);
        let t: u32 = this.findMemoryType(mr.memoryTypeBits, VK_MEMORY_DEVICE_LOCAL);
        if (t == 0xFFFFFFFF) { V.vkDestroyImage(this.device, img, nullptr); this.destroyBuffer(&staging); return false; }
        let mai: VkMemoryAllocateInfo; zero(&mai as void*, sizeof(VkMemoryAllocateInfo));
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; mai.allocationSize = mr.size; mai.memoryTypeIndex = t;
        let mem: u64 = 0;
        if (V.vkAllocateMemory(this.device, &mai as void*, nullptr, &mem) != VK_SUCCESS) { V.vkDestroyImage(this.device, img, nullptr); this.destroyBuffer(&staging); return false; }
        V.vkBindImageMemory(this.device, img, mem, 0);

        let cb: void* = this.beginOneTime();
        if (cb == nullptr) { V.vkDestroyImage(this.device, img, nullptr); V.vkFreeMemory(this.device, mem, nullptr); this.destroyBuffer(&staging); return false; }
        // UNDEFINED -> TRANSFER_DST
        let bar: VkImageMemoryBarrier; zero(&bar as void*, sizeof(VkImageMemoryBarrier));
        bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; bar.oldLayout = 0; bar.newLayout = 7;
        bar.srcQueueFamilyIndex = 0xFFFFFFFF; bar.dstQueueFamilyIndex = 0xFFFFFFFF; bar.image = img;
        bar.subresourceRange.aspectMask = 1; bar.subresourceRange.levelCount = 1; bar.subresourceRange.layerCount = 1;
        bar.srcAccessMask = 0; bar.dstAccessMask = 4096;
        V.vkCmdPipelineBarrier(cb, 1, 4096, 0, 0, nullptr, 0, nullptr, 1, &bar as void*);
        // copy
        let reg: VkBufferImageCopy; zero(&reg as void*, sizeof(VkBufferImageCopy));
        reg.imageSubresource.aspectMask = 1; reg.imageSubresource.layerCount = 1;
        reg.imageExtent.width = w as u32; reg.imageExtent.height = h as u32; reg.imageExtent.depth = 1;
        V.vkCmdCopyBufferToImage(cb, staging.buf, img, 7, 1, &reg as void*);
        // TRANSFER_DST -> SHADER_READ_ONLY
        bar.oldLayout = 7; bar.newLayout = 5; bar.srcAccessMask = 4096; bar.dstAccessMask = 32;
        V.vkCmdPipelineBarrier(cb, 4096, 128, 0, 0, nullptr, 0, nullptr, 1, &bar as void*);
        this.endOneTime(cb);
        this.destroyBuffer(&staging);

        let vci: VkImageViewCreateInfo; zero(&vci as void*, sizeof(VkImageViewCreateInfo));
        vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; vci.image = img; vci.viewType = 1; vci.format = VK_FORMAT_R8G8B8A8_SRGB;
        vci.subresourceRange.aspectMask = 1; vci.subresourceRange.levelCount = 1; vci.subresourceRange.layerCount = 1;
        let view: u64 = 0;
        if (V.vkCreateImageView(this.device, &vci as void*, nullptr, &view) != VK_SUCCESS) { V.vkDestroyImage(this.device, img, nullptr); V.vkFreeMemory(this.device, mem, nullptr); return false; }

        let dsai: VkDescriptorSetAllocateInfo; zero(&dsai as void*, sizeof(VkDescriptorSetAllocateInfo));
        dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO; dsai.descriptorPool = this.descPool; dsai.descriptorSetCount = 1; dsai.pSetLayouts = &this.texLayout;
        let set: u64 = 0;
        if (V.vkAllocateDescriptorSets(this.device, &dsai as void*, &set) != VK_SUCCESS) { V.vkDestroyImageView(this.device, view, nullptr); V.vkDestroyImage(this.device, img, nullptr); V.vkFreeMemory(this.device, mem, nullptr); return false; }
        let dii: VkDescriptorImageInfo; dii.sampler = this.sampler; dii.imageView = view; dii.imageLayout = 5;
        let wr: VkWriteDescriptorSet; zero(&wr as void*, sizeof(VkWriteDescriptorSet));
        wr.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; wr.dstSet = set; wr.dstBinding = 0; wr.descriptorCount = 1; wr.descriptorType = 1; wr.pImageInfo = &dii;
        V.vkUpdateDescriptorSets(this.device, 1, &wr as void*, 0, nullptr);

        out.image = img; out.mem = mem; out.view = view; out.set = set; out.width = w; out.height = h;
        return true;
    }

    // Descriptor sets come from a fixed pool (256) and are released with it in free().
    destroyTexture(t: Texture*) => void {
        V.vkDeviceWaitIdle(this.device);
        if (t.view != 0) V.vkDestroyImageView(this.device, t.view, nullptr);
        if (t.image != 0) V.vkDestroyImage(this.device, t.image, nullptr);
        if (t.mem != 0) V.vkFreeMemory(this.device, t.mem, nullptr);
        t.view = 0; t.image = 0; t.mem = 0; t.set = 0;
    }

    // ---- frame loop --------------------------------------------------------------------------
    setClear(r: float, g: float, b: float, a: float) => void { this.clearR = r; this.clearG = g; this.clearB = b; this.clearA = a; }

    // Returns false when the frame must be skipped (minimised window, swapchain rebuilt, error).
    // On true, a command buffer is recording inside the render pass with viewport/scissor set.
    beginFrame() => bool {
        if (this.device == nullptr || this.inFrame) return false;
        let w: int = 0; let h: int = 0;
        G.glfwGetFramebufferSize(this.winHandle, &w, &h);
        if (w == 0 || h == 0) return false;
        if ((w as u32) != this.extent.width || (h as u32) != this.extent.height) {
            if (!this.recreateSwapchain()) return false;
        }
        let fence: u64 = this.inFlight[this.frame];
        V.vkWaitForFences(this.device, 1, &fence, 1, forever());
        let idx: u32 = 0;
        let r: int = V.vkAcquireNextImageKHR(this.device, this.swapchain, forever(), this.imageAvailable[this.frame], 0, &idx);
        if (r == VK_ERROR_OUT_OF_DATE_KHR) { this.recreateSwapchain(); return false; }
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) return false;
        V.vkResetFences(this.device, 1, &fence);

        this.curImage = idx;
        this.curCmd = this.cmds[this.frame];
        V.vkResetCommandBuffer(this.curCmd, 0);
        let bi: VkCommandBufferBeginInfo; zero(&bi as void*, sizeof(VkCommandBufferBeginInfo));
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        V.vkBeginCommandBuffer(this.curCmd, &bi as void*);

        let cc: VkClearColor; cc.r = this.clearR; cc.g = this.clearG; cc.b = this.clearB; cc.a = this.clearA;
        let rp: VkRenderPassBeginInfo; zero(&rp as void*, sizeof(VkRenderPassBeginInfo));
        rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; rp.renderPass = this.renderPass; rp.framebuffer = this.fbs.data[idx];
        rp.renderArea.extent = this.extent; rp.clearValueCount = 1; rp.pClearValues = &cc;
        V.vkCmdBeginRenderPass(this.curCmd, &rp as void*, 0);

        let vp: VkViewport; vp.x = 0.0; vp.y = 0.0; vp.width = this.extent.width as float; vp.height = this.extent.height as float; vp.minDepth = 0.0; vp.maxDepth = 1.0;
        V.vkCmdSetViewport(this.curCmd, 0, 1, &vp as void*);
        let sc: VkRect2D; zero(&sc as void*, sizeof(VkRect2D)); sc.extent = this.extent;
        V.vkCmdSetScissor(this.curCmd, 0, 1, &sc as void*);
        this.inFrame = true;
        return true;
    }

    // Ends the pass, submits, presents. Handles out-of-date/suboptimal by rebuilding the swapchain.
    endFrame() => bool {
        if (!this.inFrame) return false;
        this.inFrame = false;
        V.vkCmdEndRenderPass(this.curCmd);
        V.vkEndCommandBuffer(this.curCmd);

        let waitSem: u64 = this.imageAvailable[this.frame];
        let sigSem: u64 = this.renderFinished[this.curImage];
        let stage: u32 = 1024;
        let cb: void* = this.curCmd;
        let si: VkSubmitInfo; zero(&si as void*, sizeof(VkSubmitInfo));
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; si.waitSemaphoreCount = 1; si.pWaitSemaphores = &waitSem; si.pWaitDstStageMask = &stage;
        si.commandBufferCount = 1; si.pCommandBuffers = &cb; si.signalSemaphoreCount = 1; si.pSignalSemaphores = &sigSem;
        if (V.vkQueueSubmit(this.queue, 1, &si as void*, this.inFlight[this.frame]) != VK_SUCCESS) return false;

        let sc: u64 = this.swapchain;
        let idx: u32 = this.curImage;
        let pi: VkPresentInfoKHR; zero(&pi as void*, sizeof(VkPresentInfoKHR));
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &sigSem;
        pi.swapchainCount = 1; pi.pSwapchains = &sc; pi.pImageIndices = &idx;
        let r: int = V.vkQueuePresentKHR(this.queue, &pi as void*);
        this.frame = (this.frame + 1) % MAX_FRAMES;
        if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) { this.recreateSwapchain(); return true; }
        return r == VK_SUCCESS;
    }

    // ---- draw helpers (valid between beginFrame and endFrame) ----------------------------------
    bindPipeline(p: Pipeline*) => void { V.vkCmdBindPipeline(this.curCmd, 0, p.handle); }

    bindTexture(p: Pipeline*, t: Texture*) => void {
        let set: u64 = t.set;
        V.vkCmdBindDescriptorSets(this.curCmd, 0, p.layout, 0, 1, &set, 0, nullptr);
    }

    pushConstants(p: Pipeline*, data: void*, size: u32) => void {
        V.vkCmdPushConstants(this.curCmd, p.layout, 17, 0, size, data);
    }

    bindVertexBuffer(b: Buffer*) => void {
        let buf: u64 = b.buf; let off: u64 = 0;
        V.vkCmdBindVertexBuffers(this.curCmd, 0, 1, &buf, &off);
    }

    bindIndexBuffer(b: Buffer*, kind: u32) => void { V.vkCmdBindIndexBuffer(this.curCmd, b.buf, 0, kind); }

    draw(vertexCount: u32) => void { V.vkCmdDraw(this.curCmd, vertexCount, 1, 0, 0); }

    drawIndexed(indexCount: u32) => void { V.vkCmdDrawIndexed(this.curCmd, indexCount, 1, 0, 0, 0); }

    waitIdle() => void { if (this.device != nullptr) V.vkDeviceWaitIdle(this.device); }

    // ---- teardown ----------------------------------------------------------------------------
    // Destroy your own buffers/textures/pipelines/shaders first (after waitIdle), then call this.
    free() => void {
        if (this.device == nullptr) return;
        V.vkDeviceWaitIdle(this.device);
        this.destroyFramebuffersAndViews();
        this.fbs.free(); this.views.free(); this.images.free();
        this.freeRenderSemaphores();
        if (this.imageAvailable != nullptr) {
            let i: u32 = 0;
            while (i < MAX_FRAMES) {
                if (this.imageAvailable[i] != 0) V.vkDestroySemaphore(this.device, this.imageAvailable[i], nullptr);
                if (this.inFlight[i] != 0) V.vkDestroyFence(this.device, this.inFlight[i], nullptr);
                i = i + 1;
            }
            CMem.free(this.imageAvailable as void*); CMem.free(this.inFlight as void*);
            this.imageAvailable = nullptr; this.inFlight = nullptr;
        }
        if (this.cmds != nullptr) { CMem.free(this.cmds as void*); this.cmds = nullptr; }
        if (this.owner) {
            if (this.memProps != nullptr) { CMem.free(this.memProps as void*); this.memProps = nullptr; }
            if (this.sampler != 0) V.vkDestroySampler(this.device, this.sampler, nullptr);
            if (this.descPool != 0) V.vkDestroyDescriptorPool(this.device, this.descPool, nullptr);
            if (this.texLayout != 0) V.vkDestroyDescriptorSetLayout(this.device, this.texLayout, nullptr);
        }
        if (this.pool != 0) V.vkDestroyCommandPool(this.device, this.pool, nullptr);
        if (this.renderPass != 0) V.vkDestroyRenderPass(this.device, this.renderPass, nullptr);
        if (this.swapchain != 0) V.vkDestroySwapchainKHR(this.device, this.swapchain, nullptr);
        if (this.surface != 0) V.vkDestroySurfaceKHR(this.instance, this.surface, nullptr);
        if (this.owner) {
            V.vkDestroyDevice(this.device, nullptr);
            V.vkDestroyInstance(this.instance, nullptr);
        }
        this.device = nullptr;
    }
}

/*
Vulkan version of starlikeglad.os (single threaded, FIFO/vsync present).

Files that must sit next to this one:
  std/vulkan.os + dependencies/vk/vulkan_full.h   Vulkan/GLFW declarations (header importer, no SDK needed)
  starlikeglad_vk.vert.spv     precompiled shaders (sources: starlikeglad_vk.vert / .frag)
  starlikeglad_vk.frag.spv
Runtime requirement: a GPU driver that provides vulkan-1.dll (every normal Windows install has it).
Shaders are loaded relative to the working directory, which must be the project root
(the folder containing docs/ and std/); the paths below point into docs/examples/vk/.

Run:
./path/to/Osengine.exe docs/examples/vk/starlikeglad_vk.os --execute              // JIT
./path/to/Osengine.exe docs/examples/vk/starlikeglad_vk.os --make -o app.exe      // AOT

Rewritten with the documented syntax: bool, sizeof, for loops, struct literals, [N]T arrays.
Syntax under test (patch back to the old form if the compiler rejects it):
  1. struct literals on imported Vulkan structs:  VkFoo{ field: v }
  2. [N]T array types instead of T[N]
  3. typed for-loop counters:  for (let i: uint = 0; ...; i++)
  4. bool returns and true/false
  5. sizeof(T) for imported and local types
  6. main() => i32
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

function vkOk(result: int, what: char*) => bool {
    if (result != VK_SUCCESS) {
        printf("Vulkan error %d in %s\n", result, what);
        fflush(nullptr);
        return false;
    }
    return true;
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

    createInstance() => bool {
        if (!glfwVulkanSupported()) {
            printf("GLFW: Vulkan not supported (is vulkan-1.dll installed?)\n");
            return false;
        }

        let extCount: uint = 0;
        let exts = glfwGetRequiredInstanceExtensions(&extCount);

        // VK_API_VERSION_1_0 is a function-like macro in the real header, so it is not generated
        let app = VkApplicationInfo{
            sType: VK_STRUCTURE_TYPE_APPLICATION_INFO,
            pApplicationName: "Vulkan Star Field",
            pEngineName: "Osengine",
            apiVersion: 0x00400000
        };

        let ci = VkInstanceCreateInfo{
            sType: VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            pApplicationInfo: &app,
            enabledExtensionCount: extCount,
            ppEnabledExtensionNames: exts
        };

        if (!vkOk(vkCreateInstance(&ci, nullptr, &this.instance), "vkCreateInstance")) return false;
        printf("Created Vulkan instance\n");
        return true;
    }

    createSurface() => bool {
        if (!vkOk(glfwCreateWindowSurface(this.instance, this.window, nullptr, &this.surface), "glfwCreateWindowSurface")) return false;
        printf("Created surface\n");
        return true;
    }

    pickDevice() => bool {
        let count: uint = 1;
        let r = vkEnumeratePhysicalDevices(this.instance, &count, &this.physicalDevice);
        if (r != VK_SUCCESS && r != VK_INCOMPLETE) { // INCOMPLETE: more than one GPU, we take the first
            printf("vkEnumeratePhysicalDevices failed: %d\n", r);
            return false;
        }
        if (count == 0 || this.physicalDevice == nullptr) {
            printf("No Vulkan capable GPU found\n");
            return false;
        }

        // Find a queue family that supports graphics AND presenting
        let famCount: uint = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(this.physicalDevice, &famCount, nullptr);

        let props = malloc((famCount * sizeof(VkQueueFamilyProperties)) as u64) as VkQueueFamilyProperties*;
        vkGetPhysicalDeviceQueueFamilyProperties(this.physicalDevice, &famCount, props);

        let found: bool = false;
        for (let i: uint = 0; i < famCount && !found; i++) {
            let supported: uint = 0;
            vkGetPhysicalDeviceSurfaceSupportKHR(this.physicalDevice, i, this.surface, &supported);
            if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && supported != 0) {
                this.queueFamily = i;
                found = true;
            }
        }
        free(props as void*);

        if (!found) {
            printf("No graphics+present queue family\n");
            return false;
        }
        printf("Using queue family %u\n", this.queueFamily);
        return true;
    }

    createDevice() => bool {
        let priority: float = 1.0;
        let qci = VkDeviceQueueCreateInfo{
            sType: VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            queueFamilyIndex: this.queueFamily,
            queueCount: 1,
            pQueuePriorities: &priority
        };

        let swapchainExt: char* = "VK_KHR_swapchain";

        let dci = VkDeviceCreateInfo{
            sType: VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            queueCreateInfoCount: 1,
            pQueueCreateInfos: &qci,
            enabledExtensionCount: 1,
            ppEnabledExtensionNames: &swapchainExt
        };

        if (!vkOk(vkCreateDevice(this.physicalDevice, &dci, nullptr, &this.device), "vkCreateDevice")) return false;
        vkGetDeviceQueue(this.device, this.queueFamily, 0, &this.queue);
        printf("Created logical device\n");
        return true;
    }

    // Returns 0 on failure, 1 on success, 2 when the window is minimized (nothing to build yet)
    createSwapchain() => int {
        let caps: VkSurfaceCapabilitiesKHR;
        if (!vkOk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this.physicalDevice, this.surface, &caps), "surface caps")) return 0;

        let w: int;
        let h: int;
        glfwGetFramebufferSize(this.window, &w, &h);
        if (w <= 0 || h <= 0) return 2;

        this.extentW = w as uint;
        this.extentH = h as uint;
        if (this.extentW < caps.minImageExtent.width) this.extentW = caps.minImageExtent.width;
        if (this.extentH < caps.minImageExtent.height) this.extentH = caps.minImageExtent.height;
        if (this.extentW > caps.maxImageExtent.width) this.extentW = caps.maxImageExtent.width;
        if (this.extentH > caps.maxImageExtent.height) this.extentH = caps.maxImageExtent.height;

        let minImages = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && minImages > caps.maxImageCount) minImages = caps.maxImageCount;

        let sci = VkSwapchainCreateInfoKHR{
            sType: VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            surface: this.surface,
            minImageCount: minImages,
            imageFormat: VK_FORMAT_B8G8R8A8_UNORM,
            imageColorSpace: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            imageArrayLayers: 1,
            imageUsage: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            imageSharingMode: VK_SHARING_MODE_EXCLUSIVE,
            preTransform: caps.currentTransform,
            compositeAlpha: VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            presentMode: VK_PRESENT_MODE_FIFO_KHR,
            clipped: VK_TRUE
        };
        sci.imageExtent.width = this.extentW;
        sci.imageExtent.height = this.extentH;

        if (!vkOk(vkCreateSwapchainKHR(this.device, &sci, nullptr, &this.swapchain), "vkCreateSwapchainKHR")) return 0;

        vkGetSwapchainImagesKHR(this.device, this.swapchain, &this.imageCount, nullptr);
        this.images = malloc((this.imageCount * sizeof(void*)) as u64) as void**;
        this.views = malloc((this.imageCount * sizeof(void*)) as u64) as void**;
        this.framebuffers = malloc((this.imageCount * sizeof(void*)) as u64) as void**;
        this.renderFinished = malloc((this.imageCount * sizeof(void*)) as u64) as void**;
        vkGetSwapchainImagesKHR(this.device, this.swapchain, &this.imageCount, this.images);

        // Workaround: a `return` inside a loop body is typed 'Void' by the compiler, so failures
        // inside the loop clear `ok` and the return happens after the loop.
        let ok: bool = true;
        for (let i: uint = 0; i < this.imageCount && ok; i++) {
            let vci = VkImageViewCreateInfo{
                sType: VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                image: this.images[i],
                viewType: VK_IMAGE_VIEW_TYPE_2D,
                format: VK_FORMAT_B8G8R8A8_UNORM
            };
            vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vci.subresourceRange.levelCount = 1;
            vci.subresourceRange.layerCount = 1;
            if (!vkOk(vkCreateImageView(this.device, &vci, nullptr, &this.views[i]), "vkCreateImageView")) ok = false;

            if (ok) {
                let fci = VkFramebufferCreateInfo{
                    sType: VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    renderPass: this.renderPass,
                    attachmentCount: 1,
                    pAttachments: &this.views[i],
                    width: this.extentW,
                    height: this.extentH,
                    layers: 1
                };
                if (!vkOk(vkCreateFramebuffer(this.device, &fci, nullptr, &this.framebuffers[i]), "vkCreateFramebuffer")) ok = false;
            }

            if (ok) {
                let semci = VkSemaphoreCreateInfo{ sType: VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
                if (!vkOk(vkCreateSemaphore(this.device, &semci, nullptr, &this.renderFinished[i]), "renderFinished semaphore")) ok = false;
            }
        }
        if (!ok) return 0;
        return 1;
    }

    destroySwapchain() => void {
        if (this.swapchain == nullptr) return;
        for (let i: uint = 0; i < this.imageCount; i++) {
            vkDestroySemaphore(this.device, this.renderFinished[i], nullptr);
            vkDestroyFramebuffer(this.device, this.framebuffers[i], nullptr);
            vkDestroyImageView(this.device, this.views[i], nullptr);
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
        vkDeviceWaitIdle(this.device);
        this.destroySwapchain();
        return this.createSwapchain();
    }

    createRenderPass() => bool {
        let att = VkAttachmentDescription{
            format: VK_FORMAT_B8G8R8A8_UNORM,
            samples: VK_SAMPLE_COUNT_1_BIT,
            loadOp: VK_ATTACHMENT_LOAD_OP_CLEAR,
            storeOp: VK_ATTACHMENT_STORE_OP_STORE,
            stencilLoadOp: VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            stencilStoreOp: VK_ATTACHMENT_STORE_OP_DONT_CARE,
            initialLayout: VK_IMAGE_LAYOUT_UNDEFINED,
            finalLayout: VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };

        let colorRef = VkAttachmentReference{
            attachment: 0,
            layout: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        let sub = VkSubpassDescription{
            pipelineBindPoint: VK_PIPELINE_BIND_POINT_GRAPHICS,
            colorAttachmentCount: 1,
            pColorAttachments: &colorRef
        };

        let dep = VkSubpassDependency{
            srcSubpass: VK_SUBPASS_EXTERNAL,
            dstSubpass: 0,
            srcStageMask: VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            dstStageMask: VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            srcAccessMask: 0,
            dstAccessMask: VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        };

        let rpci = VkRenderPassCreateInfo{
            sType: VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            attachmentCount: 1,
            pAttachments: &att,
            subpassCount: 1,
            pSubpasses: &sub,
            dependencyCount: 1,
            pDependencies: &dep
        };

        return vkOk(vkCreateRenderPass(this.device, &rpci, nullptr, &this.renderPass), "vkCreateRenderPass");
    }

    createShaderModule(path: char*, outModule: void**) => bool {
        let code = loadFile(path);
        if (code == nullptr) return false;
        let smci = VkShaderModuleCreateInfo{
            sType: VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            codeSize: g_fileSize,
            pCode: code
        };
        let ok = vkOk(vkCreateShaderModule(this.device, &smci, nullptr, outModule), "vkCreateShaderModule");
        free(code);
        return ok;
    }

    createPipeline() => bool {
        let vert: void*;
        let frag: void*;
        if (!this.createShaderModule("docs/examples/vk/starlikeglad_vk.vert.spv", &vert)) return false;
        if (!this.createShaderModule("docs/examples/vk/starlikeglad_vk.frag.spv", &frag)) return false;

        // The importer renames the keyword field "module" to "module_"
        let stages: [2]VkPipelineShaderStageCreateInfo;
        stages[0] = VkPipelineShaderStageCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            stage: VK_SHADER_STAGE_VERTEX_BIT,
            module_: vert,
            pName: "main"
        };
        stages[1] = VkPipelineShaderStageCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            stage: VK_SHADER_STAGE_FRAGMENT_BIT,
            module_: frag,
            pName: "main"
        };

        let vertexInput = VkPipelineVertexInputStateCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        };

        let inputAsm = VkPipelineInputAssemblyStateCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            topology: VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        };

        let viewportState = VkPipelineViewportStateCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            viewportCount: 1,
            scissorCount: 1
        };

        let raster = VkPipelineRasterizationStateCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            polygonMode: VK_POLYGON_MODE_FILL,
            cullMode: VK_CULL_MODE_NONE,
            frontFace: VK_FRONT_FACE_COUNTER_CLOCKWISE,
            lineWidth: 1.0
        };

        let ms = VkPipelineMultisampleStateCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            rasterizationSamples: VK_SAMPLE_COUNT_1_BIT
        };

        // R | G | B | A (the real header has no VK_COLOR_COMPONENT_RGBA shortcut)
        let blendAtt = VkPipelineColorBlendAttachmentState{
            colorWriteMask: VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };

        let blend = VkPipelineColorBlendStateCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            attachmentCount: 1,
            pAttachments: &blendAtt
        };

        let dynStates: [2]uint = [VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR];
        let dyn = VkPipelineDynamicStateCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            dynamicStateCount: 2,
            pDynamicStates: &dynStates
        };

        let pcRange = VkPushConstantRange{
            stageFlags: VK_SHADER_STAGE_FRAGMENT_BIT,
            offset: 0,
            size: sizeof(PushConstants)
        };

        let plci = VkPipelineLayoutCreateInfo{
            sType: VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            pushConstantRangeCount: 1,
            pPushConstantRanges: &pcRange
        };
        if (!vkOk(vkCreatePipelineLayout(this.device, &plci, nullptr, &this.pipelineLayout), "vkCreatePipelineLayout")) return false;

        let gpci = VkGraphicsPipelineCreateInfo{
            sType: VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            stageCount: 2,
            pStages: &stages,
            pVertexInputState: &vertexInput,
            pInputAssemblyState: &inputAsm,
            pViewportState: &viewportState,
            pRasterizationState: &raster,
            pMultisampleState: &ms,
            pColorBlendState: &blend,
            pDynamicState: &dyn,
            layout: this.pipelineLayout,
            renderPass: this.renderPass,
            subpass: 0,
            basePipelineIndex: -1
        };

        let ok = vkOk(vkCreateGraphicsPipelines(this.device, nullptr, 1, &gpci, nullptr, &this.pipeline), "vkCreateGraphicsPipelines");

        vkDestroyShaderModule(this.device, vert, nullptr);
        vkDestroyShaderModule(this.device, frag, nullptr);
        if (ok) printf("Created graphics pipeline\n");
        return ok;
    }

    createCommands() => bool {
        let cpci = VkCommandPoolCreateInfo{
            sType: VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            flags: VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            queueFamilyIndex: this.queueFamily
        };
        if (!vkOk(vkCreateCommandPool(this.device, &cpci, nullptr, &this.commandPool), "vkCreateCommandPool")) return false;

        let cbai = VkCommandBufferAllocateInfo{
            sType: VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            commandPool: this.commandPool,
            level: VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            commandBufferCount: 1
        };
        if (!vkOk(vkAllocateCommandBuffers(this.device, &cbai, &this.commandBuffer), "vkAllocateCommandBuffers")) return false;

        let semci = VkSemaphoreCreateInfo{ sType: VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        if (!vkOk(vkCreateSemaphore(this.device, &semci, nullptr, &this.imageAvailable), "imageAvailable semaphore")) return false;

        let fci = VkFenceCreateInfo{
            sType: VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            flags: VK_FENCE_CREATE_SIGNALED_BIT
        };
        if (!vkOk(vkCreateFence(this.device, &fci, nullptr, &this.inFlight), "vkCreateFence")) return false;
        return true;
    }

    init(window: void*) => bool {
        this.window = window;
        if (!this.createInstance()) return false;
        if (!this.createSurface()) return false;
        if (!this.pickDevice()) return false;
        if (!this.createDevice()) return false;
        if (!this.createRenderPass()) return false;
        if (this.createSwapchain() == 0) return false;
        if (!this.createPipeline()) return false;
        if (!this.createCommands()) return false;
        return true;
    }

    drawFrame() => bool {
        vkWaitForFences(this.device, 1, &this.inFlight, VK_TRUE, U64_MAX);

        // Minimized or no swapchain yet: try to (re)build, otherwise skip
        if (this.swapchain == nullptr) {
            let rr = this.createSwapchain();
            if (rr == 0) return false;
            if (rr == 2) { glfwWaitEvents(); return true; }
        }

        let imageIndex: uint = 0;
        let r = vkAcquireNextImageKHR(this.device, this.swapchain, U64_MAX, this.imageAvailable, nullptr, &imageIndex);
        if (r == VK_ERROR_OUT_OF_DATE_KHR) {
            this.recreateSwapchain();
            return true;
        }
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
            printf("vkAcquireNextImageKHR failed: %d\n", r);
            return false;
        }

        vkResetFences(this.device, 1, &this.inFlight);
        vkResetCommandBuffer(this.commandBuffer, 0);

        // Record
        let bi = VkCommandBufferBeginInfo{ sType: VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(this.commandBuffer, &bi);

        let clear: [4]float = [0.0, 0.0, 0.0, 1.0];
        let rpbi = VkRenderPassBeginInfo{
            sType: VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            renderPass: this.renderPass,
            framebuffer: this.framebuffers[imageIndex],
            clearValueCount: 1,
            pClearValues: &clear
        };
        rpbi.renderArea.extent.width = this.extentW;
        rpbi.renderArea.extent.height = this.extentH;
        vkCmdBeginRenderPass(this.commandBuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(this.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this.pipeline);

        let vp = VkViewport{
            width: this.extentW as float,
            height: this.extentH as float,
            maxDepth: 1.0
        };
        vkCmdSetViewport(this.commandBuffer, 0, 1, &vp);

        let sc: VkRect2D;
        sc.extent.width = this.extentW;
        sc.extent.height = this.extentH;
        vkCmdSetScissor(this.commandBuffer, 0, 1, &sc);

        let pc = PushConstants{
            resX: this.extentW as float,
            resY: this.extentH as float,
            time: glfwGetTime() as float
        };
        vkCmdPushConstants(this.commandBuffer, this.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

        vkCmdDraw(this.commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(this.commandBuffer);
        vkEndCommandBuffer(this.commandBuffer);

        // Submit
        let waitStage: uint = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        let si = VkSubmitInfo{
            sType: VK_STRUCTURE_TYPE_SUBMIT_INFO,
            waitSemaphoreCount: 1,
            pWaitSemaphores: &this.imageAvailable,
            pWaitDstStageMask: &waitStage,
            commandBufferCount: 1,
            pCommandBuffers: &this.commandBuffer,
            signalSemaphoreCount: 1,
            pSignalSemaphores: &this.renderFinished[imageIndex]
        };
        if (!vkOk(vkQueueSubmit(this.queue, 1, &si, this.inFlight), "vkQueueSubmit")) return false;

        // Present
        let presentInfo = VkPresentInfoKHR{
            sType: VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            waitSemaphoreCount: 1,
            pWaitSemaphores: &this.renderFinished[imageIndex],
            swapchainCount: 1,
            pSwapchains: &this.swapchain,
            pImageIndices: &imageIndex
        };
        let pr = vkQueuePresentKHR(this.queue, &presentInfo);
        if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) {
            this.recreateSwapchain();
        } else if (pr != VK_SUCCESS) {
            printf("vkQueuePresentKHR failed: %d\n", pr);
            return false;
        }
        return true;
    }

    cleanup() => void {
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
    }
}

let g_app: VulkanApp;

function main() => i32 {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return 1;
    }

    // No OpenGL context, Vulkan manages the surface
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    let window = glfwCreateWindow(800, 600, "Vulkan Star Field", nullptr, nullptr);
    if (window == nullptr) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    if (!g_app.init(window)) {
        printf("Vulkan initialization failed\n");
        fflush(nullptr);
        g_app.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (!g_app.drawFrame()) break;
    }

    g_app.cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    printf("Done\n");
    fflush(nullptr);
    return 0;
}
